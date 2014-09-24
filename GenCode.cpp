/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2014, Justin Madru (justin.jdm64@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <set>
#include <fstream>
#include <llvm/IR/Constants.h>
#include <llvm/PassManager.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Assembly/PrintModulePass.h>
#include "parserbase.h"
#include "AST.h"
#include "Instructions.h"
#include "Pass.h"

NStatementList* programBlock;

void CodeContext::genCode(NStatementList stms)
{
	stms.genCode(*this);

	PassManager clean;
	clean.add(new SimpleBlockClean());
	clean.run(*module);

	if (!errors.empty()) {
		returncode = 2;
		for (auto& error : errors)
			cout << "error: " << error << endl;
		cout << "found " << errors.size() << " errors" << endl;
		return;
	}
	verifyModule(*module);

	fstream file(filename.substr(0, filename.rfind('.')) + ".ll", fstream::out);
	raw_os_ostream stream(file);

	PassManager pm;
	pm.add(createPrintModulePass(&stream));
	pm.run(*module);
}

SType* NBaseType::getType(CodeContext& context)
{
	switch (type) {
	case ParserBase::TT_VOID:
		return SType::getVoid(context);
	case ParserBase::TT_BOOL:
		return SType::getBool(context);
	case ParserBase::TT_INT8:
		return SType::getInt(context, 8);
	case ParserBase::TT_INT16:
		return SType::getInt(context, 16);
	case ParserBase::TT_INT:
	case ParserBase::TT_INT32:
		return SType::getInt(context, 32);
	case ParserBase::TT_INT64:
		return SType::getInt(context, 64);
	case ParserBase::TT_UINT8:
		return SType::getInt(context, 8, true);
	case ParserBase::TT_UINT16:
		return SType::getInt(context, 16, true);
	case ParserBase::TT_UINT:
	case ParserBase::TT_UINT32:
		return SType::getInt(context, 32, true);
	case ParserBase::TT_UINT64:
		return SType::getInt(context, 64, true);
	case ParserBase::TT_FLOAT:
		return SType::getFloat(context);
	case ParserBase::TT_DOUBLE:
		return SType::getFloat(context, true);
	case ParserBase::TT_AUTO:
	default:
		return nullptr;
	}
}

SType* NArrayType::getType(CodeContext& context)
{
	auto arrSize = size->getInt(context);
	if (arrSize < 0) {
		context.addError("Array size must be non-negative");
		return nullptr;
	}
	auto btype = baseType->getType(context);
	return btype? SType::getArray(context, btype, arrSize) : nullptr;
}

SType* NVecType::getType(CodeContext& context)
{
	auto arrSize = size->getInt(context);
	if (arrSize <= 0) {
		context.addError("vec size must be greater than 0");
		return nullptr;
	}
	auto btype = baseType->getType(context);
	if (!btype) {
		context.addError("vec type can not be auto");
		return nullptr;
	}
	if (!btype->isNumeric()) {
		context.addError("vec type only supports basic numeric types");
		return nullptr;
	}
	return SType::getVec(context, btype, arrSize);
}

SType* NUserType::getType(CodeContext& context)
{
	auto type = SUserType::lookup(context, name);
	if (!type)
		context.addError(*name + " type not declared");
	return type;
}

SType* NPointerType::getType(CodeContext& context)
{
	auto btype = baseType->getType(context);
	return btype? SType::getPointer(context, btype) : nullptr;
}

RValue NVariable::genValue(CodeContext& context, RValue var)
{
	if (!var)
		return context.errValue();
	return Inst::Load(context, var);
}

RValue NBaseVariable::loadVar(CodeContext& context)
{
	auto var = context.loadSymbol(name);
	if (!var)
		context.addError("variable " + *name + " not declared");
	return var;
}

RValue NArrayVariable::loadVar(CodeContext& context)
{
	auto indexVal = index->genValue(context);

	if (!indexVal) {
		return indexVal;
	} else if (!indexVal.stype()->isNumeric()) {
		context.addError("array index is not able to be cast to an int");
		return RValue();
	}

	auto var = arrVar->loadVar(context);
	if (!var)
		return var;
	else if (var.stype()->isPointer())
		var = Inst::Deref(context, var, true);

	if (!var.stype()->isSequence()) {
		context.addError("variable " + *getName() + " is not an array or vec");
		return RValue();
	}
	Inst::CastTo(indexVal, SType::getInt(context, 64), context);

	vector<Value*> indexes;
	indexes.push_back(RValue::getZero(context, SType::getInt(context, 32)));
	indexes.push_back(indexVal);

	auto getEl = GetElementPtrInst::Create(var, indexes, "", context);
	return RValue(getEl, var.stype()->subType());
}

RValue NMemberVariable::loadVar(CodeContext& context)
{
	auto var = baseVar->loadVar(context);
	auto varType = var.stype();

	if (!varType) {
		goto fail;
	} else if (var.stype()->isPointer()) {
		var = Inst::Deref(context, var, true);
		varType = var.stype();
	}

	if (varType->isStruct())
		return loadStruct(context, var, static_cast<SStructType*>(varType));
	else if (varType->isUnion())
		return loadUnion(context, var, static_cast<SUnionType*>(varType));
fail:
	context.addError(*getName() + " is not a struct or union");
	return RValue();
}

RValue NMemberVariable::loadStruct(CodeContext& context, RValue& baseValue, SStructType* structType)
{
	auto item = structType->getItem(memberName);
	if (!item) {
		context.addError(*getName() +" doesn't have member " + *memberName);
		return RValue();
	}

	vector<Value*> indexes;
	indexes.push_back(RValue::getZero(context, SType::getInt(context, 32)));
	indexes.push_back(ConstantInt::get(*SType::getInt(context, 32), item->first));

	auto getEl = GetElementPtrInst::Create(baseValue, indexes, "", context);
	return RValue(getEl, item->second);
}

RValue NMemberVariable::loadUnion(CodeContext& context, RValue& baseValue, SUnionType* unionType)
{
	auto item = unionType->getItem(memberName);
	if (!item) {
		context.addError(*getName() +" doesn't have member " + *memberName);
		return RValue();
	}

	auto ptr = PointerType::get(*item, 0);
	auto castEl = new BitCastInst(baseValue, ptr, "", context);
	return RValue(castEl, item);
}

RValue NDereference::loadVar(CodeContext& context)
{
	auto var = derefVar->loadVar(context);
	if (!var) {
		return var;
	} else if (!var.stype()->isPointer()) {
		context.addError("variable " + *getName() + " can not be dereferenced");
		return RValue();
	}
	return Inst::Deref(context, var);
}

RValue NAddressOf::loadVar(CodeContext& context)
{
	return addVar->loadVar(context);
}

void NParameter::genCode(CodeContext& context)
{
	auto stype = type->getType(context);
	auto stackAlloc = new AllocaInst(*stype, "", context);
	new StoreInst(arg, stackAlloc, context);
	context.storeLocalSymbol(LValue(stackAlloc, stype), name);
}

void NVariableDecl::genCode(CodeContext& context)
{
	auto initValue = initExp? initExp->genValue(context) : RValue();
	auto varType = type->getType(context);

	if (!varType) { // auto type
		if (!initValue) { // auto type requires initialization
			context.addError("auto variable type requires initialization");
			return;
		}
		varType = initValue.stype();
	}

	auto name = getName();
	if (context.loadSymbolCurr(name)) {
		context.addError("variable " + *name + " already defined");
		return;
	}

	auto var = LValue(new AllocaInst(*varType, *name, context), varType);
	context.storeLocalSymbol(var, name);

	if (initValue) {
		Inst::CastTo(initValue, varType, context);
		new StoreInst(initValue, var, context);
	}
}

void NGlobalVariableDecl::genCode(CodeContext& context)
{
	if (initExp && !initExp->isConstant()) {
		context.addError("global variables only support constant value initializer");
		return;
	}
	auto initValue = initExp? initExp->genValue(context) : RValue();
	auto varType = type->getType(context);

	if (!varType) { // auto type
		if (!initValue) { // auto type requires initialization
			context.addError("auto variable type requires initialization");
			return;
		}
		varType = initValue.stype();
	}
	if (initValue && varType != initValue.stype()) {
		context.addError("global variable initialization requires exact type matching");
		return;
	}

	auto name = getName();
	if (context.loadSymbolCurr(name)) {
		context.addError("variable " + *name + " already defined");
		return;
	}

	auto var = new GlobalVariable(*context.getModule(), *varType, false, GlobalValue::ExternalLinkage, (Constant*) initValue.value(), *name);
	context.storeGlobalSymbol(LValue(var, varType), name);
}

bool NVariableDeclGroup::addMembers(vector<pair<string, SType*> >& structVector, set<string>& memberNames, CodeContext& context)
{
	auto stype = type->getType(context);
	if (!stype) {
		context.addError("struct members must not have auto type");
		return false;
	}
	bool valid = true;
	for (auto var : *variables) {
		if (var->hasInit())
			context.addError("structs don't support variable initialization");
		auto name = var->getName();
		auto res = memberNames.insert(*name);
		if (!res.second) {
			context.addError("member name " + *name + " already declared");
			valid = false;
			continue;
		}
		structVector.push_back(make_pair(*name, stype));
	}
	return valid;
}

void NStructDeclaration::genCode(CodeContext& context)
{
	auto utype = SUserType::lookup(context, name);
	if (utype) {
		context.addError(*name + " type already declared");
		return;
	}
	vector<pair<string, SType*> > structVars;
	set<string> memberNames;
	bool valid = true;
	for (auto item : *list)
		valid &= item->addMembers(structVars, memberNames, context);
	if (valid)
		createUserType(structVars, context);
}

void NFunctionPrototype::genCode(CodeContext& context)
{
	context.addError("Called NFunctionPrototype::genCode; use genFunction instead.");
}

SFunction NFunctionPrototype::genFunction(CodeContext& context)
{
	auto sym = context.loadSymbol(name);
	if (sym) {
		if (sym.isFunction())
			return static_cast<SFunction&>(sym);
		context.addError("variable " + *name + " already defined");
		return SFunction();
	}

	auto funcType = getFunctionType(context);
	return funcType? SFunction::create(context, name, funcType) : SFunction();
}

void NFunctionPrototype::genCodeParams(SFunction function, CodeContext& context) const
{
	int i = 0;
	for (auto arg = function.arg_begin(); arg != function.arg_end(); arg++, i++) {
		auto param = params->at(i);
		arg->setName(*param->getName());
		param->setArgument(RValue(arg, function.getParam(i)));
		param->genCode(context);
	}
}

void NFunctionDeclaration::genCode(CodeContext& context)
{
	auto function = prototype->genFunction(context);
	if (!function || !body) { // no body means only function prototype
		return;
	} else if (function.size()) {
		context.addError("function " + *prototype->getName() + " already declared");
		return;
	}

	if (body->empty() || !body->back()->isTerminator()) {
		auto returnType = function.returnTy();
		if (returnType->isVoid())
			body->addItem(new NReturnStatement);
		else
			context.addError("no return for a non-void function");
	}

	if (prototype->getFunctionType(context) != function.stype()) {
		context.addError("function type for " + *prototype->getName() + " doesn't match definition");
		return;
	}

	context.startFuncBlock(function);
	prototype->genCodeParams(function, context);
	body->genCode(context);
	context.endFuncBlock();
}

void NReturnStatement::genCode(CodeContext& context)
{
	auto func = context.currFunction();
	auto funcReturn = func.returnTy();

	if (funcReturn->isVoid()) {
		if (value) {
			context.addError("function " + func.name().str() + " declared void, but non-void return found");
			return;
		}
	} else if (!value) {
		context.addError("function " + func.name().str() + " declared non-void, but void return found");
		return;
	}
	auto returnVal = value? value->genValue(context) : RValue();
	if (returnVal)
		Inst::CastTo(returnVal, funcReturn, context);
	ReturnInst::Create(context, returnVal, context);
	context.pushBlock(context.createBlock());
}

void NWhileStatement::genCode(CodeContext& context)
{
	auto condBlock = context.createContinueBlock();
	auto bodyBlock = context.createRedoBlock();
	auto endBlock = context.createBreakBlock();

	auto startBlock = isDoWhile? bodyBlock : condBlock;
	auto trueBlock = isUntil? endBlock : bodyBlock;
	auto falseBlock = isUntil? bodyBlock : endBlock;

	context.pushLocalTable();

	BranchInst::Create(startBlock, context);

	context.pushBlock(condBlock);
	Inst::Branch(trueBlock, falseBlock, condition, context);

	context.pushBlock(bodyBlock);
	body->genCode(context);
	BranchInst::Create(condBlock, context);

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popLoopBranchBlocks();
}

void NSwitchStatement::genCode(CodeContext& context)
{
	auto switchValue = value->genValue(context);
	Inst::CastTo(switchValue, SType::getInt(context, 32), context);

	auto caseBlock = context.createBlock();
	auto endBlock = context.createBreakBlock(), defaultBlock = endBlock;
	auto switchInst = SwitchInst::Create(switchValue, defaultBlock, cases->size(), context);

	context.pushLocalTable();

	set<int64_t> unique;
	bool hasDefault = false;
	for (auto caseItem : *cases) {
		if (caseItem->isValueCase()) {
			auto val = caseItem->getValue(context);
			if (!unique.insert(val->getSExtValue()).second)
				context.addError("switch case values are not unique");
			switchInst->addCase(val, caseBlock);
		} else {
			if (hasDefault)
				context.addError("switch statement has more than one default");
			hasDefault = true;
			defaultBlock = caseBlock;
		}

		context.pushBlock(caseBlock);
		caseItem->genCode(context);

		if (caseItem->isLastStmBranch()) {
			caseBlock = context.currBlock();
		} else {
			caseBlock = context.createBlock();
			BranchInst::Create(caseBlock, context);
			context.pushBlock(caseBlock);
		}
	}
	switchInst->setDefaultDest(defaultBlock);

	// NOTE: the last case will create a dangling block which needs a terminator.
	BranchInst::Create(endBlock, context);

	context.popLocalTable();
	context.popBreakBlock();
	context.pushBlock(endBlock);
}

void NForStatement::genCode(CodeContext& context)
{
	auto condBlock = context.createBlock();
	auto bodyBlock = context.createRedoBlock();
	auto postBlock = context.createContinueBlock();
	auto endBlock = context.createBreakBlock();

	context.pushLocalTable();

	preStm->genCode(context);
	BranchInst::Create(condBlock, context);

	context.pushBlock(condBlock);
	Inst::Branch(bodyBlock, endBlock, condition, context);

	context.pushBlock(bodyBlock);
	body->genCode(context);
	BranchInst::Create(postBlock, context);

	context.pushBlock(postBlock);
	postExp->genCode(context);
	BranchInst::Create(condBlock, context);

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popLoopBranchBlocks();
}

void NIfStatement::genCode(CodeContext& context)
{
	auto ifBlock = context.createBlock();
	auto elseBlock = context.createBlock();
	auto endBlock = elseBody? context.createBlock() : elseBlock;

	context.pushLocalTable();

	Inst::Branch(ifBlock, elseBlock, condition, context);

	context.pushBlock(ifBlock);
	body->genCode(context);
	BranchInst::Create(endBlock, context);

	context.popLocalTable();
	context.pushLocalTable();

	context.pushBlock(elseBlock);
	if (elseBody) {
		elseBody->genCode(context);
		BranchInst::Create(endBlock, context);
	}
	context.pushBlock(endBlock);
	context.popLocalTable();
}

void NLabelStatement::genCode(CodeContext& context)
{
	// check if label already declared
	auto label = context.getLabelBlock(name);
	if (label) {
		if (!label->isPlaceholder) {
			context.addError("label " + *name + " already defined");
			return;
		}
		// a used label is no longer a placeholder
		label->isPlaceholder = false;
	} else {
		label = context.createLabelBlock(name, false);
	}
	BranchInst::Create(label->block, context);
	context.pushBlock(label->block);
}

void NGotoStatement::genCode(CodeContext& context)
{
	auto skip = context.createBlock();
	auto label = context.getLabelBlock(name);
	if (!label) {
		// trying to jump to a non-existant label. create place holder and
		// later check if it's used at the end of the function.
		label = context.createLabelBlock(name, true);
	}
	BranchInst::Create(label->block, context);
	context.pushBlock(skip);
}

void NLoopBranch::genCode(CodeContext& context)
{
	BasicBlock* block;
	string typeName;
	auto brLevel = level? level->getInt(context) : 1;

	switch (type) {
	case ParserBase::TT_CONTINUE:
		block = context.getContinueBlock(brLevel);
		if (!block) {
			typeName = "continue";
			goto error;
		}
		break;
	case ParserBase::TT_REDO:
		block = context.getRedoBlock(brLevel);
		if (!block) {
			typeName = "redo";
			goto error;
		}
		break;
	case ParserBase::TT_BREAK:
		block = context.getBreakBlock(brLevel);
		if (!block) {
			typeName = "break";
			goto error;
		}
		break;
	default:
		context.addError("undefined loop branch type: " + to_string(type));
		return;
	}
	BranchInst::Create(block, context);
	context.pushBlock(context.createBlock());
	return;
error:
	context.addError(typeName + " invalid outside a loop/switch block");
}

RValue NAssignment::genValue(CodeContext& context)
{
	auto lhsVar = lhs->loadVar(context);
	auto rhsExp = rhs->genValue(context);

	if (!lhsVar || !rhsExp)
		return context.errValue();

	if (oper != '=') {
		auto lhsLocal = Inst::Load(context, lhsVar);
		rhsExp = Inst::BinaryOp(oper, lhsLocal, rhsExp, context);
	}
	Inst::CastTo(rhsExp, lhsVar.stype(), context);
	new StoreInst(rhsExp, lhsVar, context);

	return rhsExp;
}

RValue NTernaryOperator::genValue(CodeContext& context)
{
	auto condExp = condition->genValue(context);
	Inst::CastTo(condExp, SType::getBool(context), context);

	RValue trueExp, falseExp, retVal;
	if (trueVal->isComplex() || falseVal->isComplex()) {
		auto trueBlock = context.createBlock();
		auto falseBlock = context.createBlock();
		auto endBlock = context.createBlock();

		BranchInst::Create(trueBlock, falseBlock, condExp, context);

		context.pushBlock(trueBlock);
		trueExp = trueVal->genValue(context);
		BranchInst::Create(endBlock, context);

		context.pushBlock(falseBlock);
		falseExp = falseVal->genValue(context);
		BranchInst::Create(endBlock, context);

		context.pushBlock(endBlock);
		auto result = PHINode::Create(trueExp.type(), 2, "", context);
		result->addIncoming(trueExp, trueBlock);
		result->addIncoming(falseExp, falseBlock);
		retVal = RValue(result, trueExp.stype());
	} else {
		trueExp = trueVal->genValue(context);
		falseExp = falseVal->genValue(context);
		auto select = SelectInst::Create(condExp, trueExp, falseExp, "", context);
		retVal = RValue(select, trueExp.stype());
	}

	if (trueExp.stype() != falseExp.stype())
		context.addError("return types of ternary must match");
	return retVal;
}

RValue NLogicalOperator::genValue(CodeContext& context)
{
	auto saveBlock = context.currBlock();
	auto firstBlock = context.createBlock();
	auto secondBlock = context.createBlock();
	auto trueBlock = (oper == ParserBase::TT_LOG_AND)? firstBlock : secondBlock;
	auto falseBlock = (oper == ParserBase::TT_LOG_AND)? secondBlock : firstBlock;

	auto lhsExp = Inst::Branch(trueBlock, falseBlock, lhs, context);

	context.pushBlock(firstBlock);
	auto rhsExp = rhs->genValue(context);
	Inst::CastTo(rhsExp, SType::getBool(context), context);
	BranchInst::Create(secondBlock, context);

	context.pushBlock(secondBlock);
	auto result = PHINode::Create(Type::getInt1Ty(context), 2, "", context);
	result->addIncoming(lhsExp, saveBlock);
	result->addIncoming(rhsExp, firstBlock);

	return RValue(result, SType::getBool(context));
}

RValue NCompareOperator::genValue(CodeContext& context)
{
	auto lhsExp = lhs->genValue(context);
	auto rhsExp = rhs->genValue(context);

	return Inst::Cmp(oper, lhsExp, rhsExp, context);
}

RValue NBinaryMathOperator::genValue(CodeContext& context)
{
	auto lhsExp = lhs->genValue(context);
	auto rhsExp = rhs->genValue(context);

	return Inst::BinaryOp(oper, lhsExp, rhsExp, context);
}

RValue NNullCoalescing::genValue(CodeContext& context)
{
	RValue rhsExp, retVal;
	auto lhsExp = lhs->genValue(context);
	auto condition = lhsExp;

	Inst::CastTo(condition, SType::getBool(context), context);
	if (rhs->isComplex()) {
		auto trueBlock = context.currBlock();
		auto falseBlock = context.createBlock();
		auto endBlock = context.createBlock();

		BranchInst::Create(endBlock, falseBlock, condition, context);

		context.pushBlock(falseBlock);
		rhsExp = rhs->genValue(context);
		BranchInst::Create(endBlock, context);

		context.pushBlock(endBlock);
		auto result = PHINode::Create(lhsExp.type(), 2, "", context);
		result->addIncoming(lhsExp, trueBlock);
		result->addIncoming(rhsExp, falseBlock);

		retVal = RValue(result, lhsExp.stype());
	} else {
		rhsExp = rhs->genValue(context);
		auto select = SelectInst::Create(condition, lhsExp, rhsExp, "", context);
		retVal = RValue(select, lhsExp.stype());
	}

	if (lhsExp.stype() != rhsExp.stype())
		context.addError("return types of null coalescing operator must match");
	return retVal;
}

RValue NSizeOfOperator::genValue(CodeContext& context)
{
	SType* stype = nullptr;
	switch (type) {
	case DATA:
		stype = dtype->getType(context);
		break;
	case EXP:
		stype = exp->genValue(context).stype();
		break;
	case NAME:
		auto isType = SUserType::lookup(context, name);
		auto isVar = context.loadSymbol(name);

		if (isType && isVar) {
			context.addError(*name + " is ambigious, both a type and a variable");
			return context.errValue();
		} else if (isType) {
			stype = isType;
		} else if (isVar) {
			stype = isVar.stype();
		} else {
			context.addError("type " + *name + " is not declared");
			return context.errValue();
		}
		break;
	}
	if (!stype) {
		return context.errValue();
	} else if (stype->isVoid()) {
		context.addError("size of void is invalid");
		return context.errValue();
	}
	auto itype = SType::getInt(context, 64, true);
	auto size = ConstantInt::get(*itype, SType::allocSize(context, stype));
	return RValue(size, itype);
}

RValue NUnaryMathOperator::genValue(CodeContext& context)
{
	auto unaryExp = unary->genValue(context);
	auto type = unaryExp.stype();

	switch (oper) {
	case '+':
	case '-':
		return Inst::BinaryOp(oper, RValue::getZero(context, type), unaryExp, context);
	case '!':
		return Inst::Cmp(ParserBase::TT_EQ, RValue::getZero(context, type), unaryExp, context);
	case '~':
		return Inst::BinaryOp('^', RValue::getAllOne(context, type), unaryExp, context);
	default:
		context.addError("invalid unary operator " + to_string(oper));
		return context.errValue();
	}
}

RValue NFunctionCall::genValue(CodeContext& context)
{
	auto sym = context.loadSymbol(name);
	if (!sym || !sym.isFunction()) {
		context.addError("function " + *name + " not defined");
		return context.errValue();
	}
	auto func = static_cast<SFunction&>(sym);
	auto argCount = arguments->size();
	auto paramCount = func.numParams();
	if (argCount != paramCount) {
		context.addError("argument count for " + func.name().str() + " function invalid, "
			+ to_string(argCount) + " arguments given, but " + to_string(paramCount) + " required.");
		return context.errValue();
	}
	vector<Value*> exp_list;
	int i = 0;
	for (auto arg : *arguments) {
		auto argExp = arg->genValue(context);
		Inst::CastTo(argExp, func.getParam(i++), context);
		exp_list.push_back(argExp);
	}
	auto call = CallInst::Create(func, exp_list, "", context);
	return RValue(call, func.returnTy());
}

RValue NFunctionCall::loadVar(CodeContext& context)
{
	auto value = genValue(context);
	auto stackAlloc = new AllocaInst(value.type(), "", context);
	new StoreInst(value, stackAlloc, context);
	return RValue(stackAlloc, value.stype());
}

RValue NIncrement::genValue(CodeContext& context)
{
	auto varPtr = variable->loadVar(context);
	auto varVal = variable->genValue(context, varPtr);
	auto incType = varVal.stype()->isPointer()? SType::getInt(context, 32) : varVal.stype();

	auto result = Inst::BinaryOp(type, varVal, RValue::getNumVal(context, incType, type == ParserBase::TT_INC? 1:-1), context);
	new StoreInst(result, varPtr, context);

	return isPostfix? varVal : RValue(result, varVal.stype());
}

RValue NBoolConst::genValue(CodeContext& context)
{
	auto val = value? ConstantInt::getTrue(context) : ConstantInt::getFalse(context);
	return RValue(val, SType::getBool(context));
}

RValue NIntConst::genValue(CodeContext& context)
{
	static const map<string, SType*> suffix = {
		{"i8", SType::getInt(context, 8)},
		{"u8", SType::getInt(context, 8, true)},
		{"i16", SType::getInt(context, 16)},
		{"u16", SType::getInt(context, 16, true)},
		{"i32", SType::getInt(context, 32)},
		{"u32", SType::getInt(context, 32, true)},
		{"i64", SType::getInt(context, 64)},
		{"u64", SType::getInt(context, 64, true)} };
	auto type = SType::getInt(context, 32); // default is int32

	auto data = getValueAndSuffix();
	if (data.size() > 1) {
		auto suf = suffix.find(data[1]);
		if (suf == suffix.end())
			context.addError("invalid integer suffix: " + data[1]);
		else
			type = suf->second;
	}
	string intVal(data[0], base == 10? 0:2);
	auto val = ConstantInt::get((IntegerType*) type->type(), intVal, base);
	return RValue(val, type);
}

RValue NFloatConst::genValue(CodeContext& context)
{
	static const map<string, SType*> suffix = {
		{"f", SType::getFloat(context)},
		{"d", SType::getFloat(context, true)} };
	auto type = SType::getFloat(context, true);

	auto data = getValueAndSuffix();
	if (data.size() > 1) {
		auto suf = suffix.find(data[1]);
		if (suf == suffix.end())
			context.addError("invalid float suffix: " + data[1]);
		else
			type = suf->second;
	}
	auto fp = ConstantFP::get(*type, data[0]);
	return RValue(fp, type);
}

RValue NCharConst::genValue(CodeContext& context)
{
	char cVal = '\0';
	if (value->at(0) == '\\' && value->length() > 1) {
		switch (value->at(1)) {
		case '0': cVal = '\0'; break;
		case 'a': cVal = '\a'; break;
		case 'b': cVal = '\b'; break;
		case 'e': cVal =   27; break;
		case 'f': cVal = '\f'; break;
		case 'n': cVal = '\n'; break;
		case 'r': cVal = '\r'; break;
		case 't': cVal = '\t'; break;
		case 'v': cVal = '\v'; break;
		default: cVal = value->at(1);
		}
	} else {
		cVal = value->at(0);
	}
	auto type = SType::getInt(context, 8, true);
	auto val = ConstantInt::get((IntegerType*) type->type(), cVal);
	return RValue(val, type);
}

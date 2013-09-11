/*      Saphyr, a C++ style compiler using LLVM
        Copyright (C) 2012, Justin Madru (justin.jdm64@gmail.com)

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <set>
#include <fstream>
#include <llvm/Constants.h>
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
	auto size = ConstantInt::get(Type::getIntNTy(context, 64), *strSize, 10);
	auto arrSize = size->getSExtValue();

	if (arrSize < 0) {
		context.addError("Array size must be non-negative");
		return nullptr;
	}
	auto btype = baseType->getType(context);
	return btype? SType::getArray(context, btype, arrSize) : nullptr;
}

RValue NVariable::genValue(CodeContext& context)
{
	auto var = loadVar(context);
	if (!var)
		return context.errValue();
	auto load = new LoadInst(var, "", context);
	return RValue(load, var.stype());
}

RValue NBaseVariable::loadVar(CodeContext& context)
{
	auto var = context.loadVar(name);
	if (!var)
		context.addError("variable " + *name + " not declared");
	return var;
}

RValue NArrayVariable::loadVar(CodeContext& context)
{
	auto zero = RValue::getZero(SType::getInt(context, 32));
	auto indexVal = index->genValue(context);

	if (!indexVal) {
		return indexVal;
	} else if (indexVal.stype()->isArray()) {
		context.addError("array index is not able to be cast to an int");
		return RValue();
	}
	Inst::CastMatch(indexVal, SType::getInt(context, 64), context);

	vector<Value*> indexes;
	indexes.push_back(zero);
	indexes.push_back(indexVal);

	auto var = arrVar->loadVar(context);
	if (!var) {
		return var;
	} else if (!var.stype()->isArray()) {
		context.addError("variable " + *getName() + " is not an array");
		return RValue();
	}
	auto getEl = GetElementPtrInst::Create(var, indexes, "", context);
	return RValue(getEl, var.stype()->subType());
}

void NParameter::genCode(CodeContext& context)
{
	auto stype = type->getType(context);
	auto stackAlloc = new AllocaInst(*stype, "", context);
	new StoreInst(arg, stackAlloc, context);
	context.storeLocalVar(LValue(stackAlloc, stype), name);
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
	if (context.loadVarCurr(name)) {
		context.addError("variable " + *name + " already defined");
		return;
	}

	auto var = LValue(new AllocaInst(*varType, *name, context), varType);
	context.storeLocalVar(var, name);

	if (initValue) {
		Inst::CastMatch(initValue, varType, context);
		new StoreInst(initValue, var, context);
	}
}

void NGlobalVariableDecl::genCode(CodeContext& context)
{
	if (initExp && initExp->getNodeType() != NodeType::IntConst && initExp->getNodeType() != NodeType::FloatConst) {
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
	if (context.loadVarCurr(name)) {
		context.addError("variable " + *name + " already defined");
		return;
	}

	auto var = new GlobalVariable(*context.getModule(), *varType, false, GlobalValue::ExternalLinkage, (Constant*) initValue.value(), *name);
	context.storeGlobalVar(LValue(var, varType), name);
}

void NFunctionPrototype::genCode(CodeContext& context)
{
	context.addError("Called NFunctionPrototype::genCode; use genFunction instead.");
}

SFunction* NFunctionPrototype::genFunction(CodeContext& context)
{
	auto function = context.getFunction(name);
	if (function)
		return function;

	auto funcType = getFunctionType(context);
	return SFunction::create(context, name, funcType);
}

void NFunctionPrototype::genCodeParams(SFunction* function, CodeContext& context)
{
	int i = 0;
	for (auto arg = function->arg_begin(); arg != function->arg_end(); arg++, i++) {
		auto param = params->at(i);
		arg->setName(*param->getName());
		param->setArgument(RValue(arg, function->stype()->getParam(i)));
		param->genCode(context);
	}
}

void NFunctionDeclaration::genCode(CodeContext& context)
{
	auto function = prototype->genFunction(context);
	if (!function || !body) { // no body means only function prototype
		return;
	} else if (function->size()) {
		context.addError("function " + *prototype->getName() + " already declared");
		return;
	}

	if (body->back()->getNodeType() != NodeType::ReturnStm) {
		auto returnType = function->returnTy();
		if (returnType->isVoid())
			body->addItem(new NReturnStatement);
		else
			context.addError("no return for a non-void function");
	}

	if (prototype->getFunctionType(context) != function->stype()) {
		context.addError("function type for " + *prototype->getName() + " doesn't match definition");
		return;
	}

	context.startFuncBlock(function);

	// pad stack with 4 bytes
	// also fixes issue with function with one instruction not being declared
	new AllocaInst(SType::getInt(context, 32)->type(), "", context);

	prototype->genCodeParams(function, context);
	body->genCode(context);
	context.endFuncBlock();
}

void NReturnStatement::genCode(CodeContext& context)
{
	auto func = context.currFunction();
	auto funcReturn = func->returnTy();

	if (funcReturn->isVoid()) {
		if (value) {
			context.addError(func->name().str() + " function declared void, but non-void return found");
			return;
		}
	} else if (!value) {
		context.addError(func->name().str() + " function declared non-void, but void return found");
		return;
	}
	auto returnVal = value? value->genValue(context) : RValue();
	if (returnVal)
		Inst::CastMatch(returnVal, funcReturn, context);
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
	Inst::CastMatch(switchValue, SType::getInt(context, 32), context);

	auto caseBlock = context.createBlock();
	auto endBlock = context.createBreakBlock(), defaultBlock = endBlock;
	auto switchInst = SwitchInst::Create(switchValue, defaultBlock, cases->size(), context);

	context.pushLocalTable();

	set<int64_t> unique;
	bool hasDefault = false;
	for (auto caseItem : *cases) {
		if (caseItem->isValueCase()) {
			auto val = static_cast<ConstantInt*>(caseItem->genValue(context).value());
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
			context.addError("label \"" + *name + "\" already defined");
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

	switch (type) {
	case ParserBase::TT_CONTINUE:
		block = context.getContinueBlock();
		if (!block) {
			typeName = "continue";
			goto error;
		}
		break;
	case ParserBase::TT_REDO:
		block = context.getRedoBlock();
		if (!block) {
			typeName = "redo";
			goto error;
		}
		break;
	case ParserBase::TT_BREAK:
		block = context.getBreakBlock();
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
	context.addError("no valid context for " + typeName + " statement");
}

RValue NAssignment::genValue(CodeContext& context)
{
	auto lhsVar = lhs->loadVar(context);
	auto rhsExp = rhs->genValue(context);

	if (!lhsVar)
		return context.errValue();

	if (oper != '=') {
		auto lhsLocal = RValue(new LoadInst(lhsVar, "", context), lhsVar.stype());
		rhsExp = Inst::BinaryOp(oper, lhsLocal, rhsExp, context);
	}
	Inst::CastMatch(rhsExp, lhsVar.stype(), context);
	new StoreInst(rhsExp, lhsVar, context);

	return rhsExp;
}

RValue NTernaryOperator::genValue(CodeContext& context)
{
	auto condExp = condition->genValue(context);
	Inst::CastMatch(condExp, SType::getBool(context), context);

	RValue trueExp, falseExp, retVal;
	if (Inst::isComplexExp(trueVal->getNodeType()) || Inst::isComplexExp(falseVal->getNodeType())) {
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
	Inst::CastMatch(rhsExp, SType::getBool(context), context);
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

	Inst::CastMatch(condition, SType::getBool(context), context);
	if (Inst::isComplexExp(rhs->getNodeType())) {
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
	auto stype = dtype? dtype->getType(context) : exp->genValue(context).stype();
	if (!stype)
		return context.errValue();
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
		return Inst::BinaryOp(oper, RValue::getZero(type), unaryExp, context);
	case '!':
		return Inst::Cmp(ParserBase::TT_EQ, RValue::getZero(type), unaryExp, context);
	case '~':
		return Inst::BinaryOp('^', RValue::getAllOne(type), unaryExp, context);
	default:
		context.addError("invalid unary operator " + to_string(oper));
		return context.errValue();
	}
}

RValue NFunctionCall::genValue(CodeContext& context)
{
	auto func = context.getFunction(name);
	if (!func) {
		context.addError("function " + *name + " not defined");
		return context.errValue();
	}
	auto argCount = arguments->size();
	auto paramCount = func->stype()->numParams();
	if (argCount != paramCount)
		context.addError("argument count for " + func->name().str() + " function invalid, "
			+ to_string(argCount) + " arguments given, but " + to_string(paramCount) + " required.");
	auto funcType = func->stype();
	vector<Value*> exp_list;
	int i = 0;
	for (auto arg : *arguments) {
		auto argExp = arg->genValue(context);
		Inst::CastMatch(argExp, funcType->getParam(i++), context);
		exp_list.push_back(argExp);
	}
	auto call = CallInst::Create(*func, exp_list, "", context);
	return RValue(call, func->returnTy());
}

RValue NIncrement::genValue(CodeContext& context)
{
	auto varVal = variable->genValue(context);

	auto result = Inst::BinaryOp(type, varVal, RValue::getOne(varVal.stype()), context);
	new StoreInst(result, context.loadVar(variable->getName()), context);

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

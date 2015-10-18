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
#include "LLVM_Defines.h"

#include "parserbase.h"
#include "AST.h"
#include "Instructions.h"

NStatementList* programBlock;

const string NExprVariable::STR_TMP_EXP = "temp expression";

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
		return SType::getAuto(context);
	}
}

SType* NArrayType::getType(CodeContext& context)
{
	auto btype = baseType->getType(context);
	if (!btype) {
		return nullptr;
	} else if (btype->isAuto()) {
		auto token = static_cast<NNamedType*>(baseType)->getToken();
		context.addError("can't create array of auto types", token);
		return nullptr;
	}
	if (size) {
		auto arrSize = size->getIntVal(context).getSExtValue();
		if (arrSize <= 0) {
			context.addError("Array size must be positive", size->getToken());
			return nullptr;
		}
		return SType::getArray(context, btype, arrSize);
	} else {
		return SType::getArray(context, btype, 0);
	}
}

SType* NVecType::getType(CodeContext& context)
{
	auto arrSize = size->getIntVal(context).getSExtValue();
	if (arrSize <= 0) {
		context.addError("vec size must be greater than 0", size->getToken());
		return nullptr;
	}
	auto btype = baseType->getType(context);
	if (!btype) {
		return nullptr;
	} else if (btype->isAuto()) {
		auto token = static_cast<NNamedType*>(baseType)->getToken();
		context.addError("vec type can not be auto", token);
		return nullptr;
	}
	if (!btype->isNumeric()) {
		auto token = static_cast<NNamedType*>(baseType)->getToken();
		context.addError("vec type only supports basic numeric types", token);
		return nullptr;
	}
	return SType::getVec(context, btype, arrSize);
}

SType* NUserType::getType(CodeContext& context)
{
	auto typeName = getName();
	auto type = SUserType::lookup(context, typeName);
	if (!type) {
		context.addError(typeName + " type not declared", token);
		return nullptr;
	}
	return type->isAlias()? type->subType() : type;
}

SType* NPointerType::getType(CodeContext& context)
{
	auto btype = baseType->getType(context);
	if (!btype) {
		return nullptr;
	} else if (btype->isAuto()) {
		auto token = static_cast<NNamedType*>(baseType)->getToken();
		context.addError("can't create pointer to auto type", token);
		return nullptr;
	}
	return SType::getPointer(context, btype);
}

SFunctionType* NFuncPointerType::getType(CodeContext& context, Token* atToken, NDataType* retType, NDataTypeList* params)
{
	bool valid = true;
	vector<SType*> args;
	for (auto item : *params) {
		auto param = item->getType(context);
		if (!param) {
			valid = false;
		} else if (param->isAuto()) {
			auto token = static_cast<NNamedType*>(item)->getToken();
			context.addError("parameter can not be auto type", token);
			valid = false;
		} else if (SType::validate(context, atToken, param)) {
			args.push_back(param);
		}
	}

	auto returnType = retType->getType(context);
	if (!returnType) {
		return nullptr;
	} else if (returnType->isAuto()) {
		auto token = static_cast<NNamedType*>(retType)->getToken();
		context.addError("function return type can not be auto", token);
		return nullptr;
	}

	return (returnType && valid)? SType::getFunction(context, returnType, args) : nullptr;
}

RValue NVariable::genValue(CodeContext& context, RValue var)
{
	if (!var)
		return var;

	auto type = var.stype();
	if (type->isFunction())
		// don't require address-of operator for converting
		// a function into a function pointer
		return RValue(var.value(), SType::getPointer(context, var.stype()));
	else if (type->isEnum() && var.isConst())
		// an enum value, not a variable, don't load it
		return var;

	return Inst::Load(context, var);
}

RValue NBaseVariable::loadVar(CodeContext& context)
{
	auto varName = getName();
	auto var = context.loadSymbol(varName);
	if (var)
		return var;
	auto userVar = SUserType::lookup(context, varName);
	if (!userVar) {
		context.addError("variable " + varName + " not declared", name);
		return var;
	}
	return RValue(ConstantInt::getFalse(context), userVar);
}

RValue NArrayVariable::loadVar(CodeContext& context)
{
	auto indexVal = index->genValue(context);

	if (!indexVal) {
		return indexVal;
	} else if (!indexVal.stype()->isNumeric()) {
		context.addError("array index is not able to be cast to an int", brackTok);
		return RValue();
	}

	auto var = arrVar->loadVar(context);
	if (!var)
		return var;
	var = Inst::Deref(context, var, true);

	if (!var.stype()->isSequence()) {
		context.addError("variable " + getName() + " is not an array or vec", brackTok);
		return RValue();
	}
	Inst::CastTo(context, brackTok, indexVal, SType::getInt(context, 64));

	vector<Value*> indexes;
	indexes.push_back(RValue::getZero(context, SType::getInt(context, 32)));
	indexes.push_back(indexVal);

	auto getEl = GetElementPtrInst::Create(var, indexes, "", context);
	return RValue(getEl, var.stype()->subType());
}

RValue NMemberVariable::loadVar(CodeContext& context)
{
	auto var = baseVar->loadVar(context);
	if (!var)
		return RValue();

	var = Inst::Deref(context, var, true);
	auto varType = var.stype();

	if (varType->isStruct())
		return loadStruct(context, var, static_cast<SStructType*>(varType));
	else if (varType->isUnion())
		return loadUnion(context, var, static_cast<SUnionType*>(varType));
	else if (varType->isEnum())
		return loadEnum(context, static_cast<SEnumType*>(varType));

	context.addError(getName() + " is not a struct/union/enum", dotToken);
	return RValue();
}

RValue NMemberVariable::loadStruct(CodeContext& context, RValue& baseValue, SStructType* structType) const
{
	auto member = getMemberName();
	auto item = structType->getItem(member);
	if (!item) {
		context.addError(getName() +" doesn't have member " + member, memberName);
		return RValue();
	}

	vector<Value*> indexes;
	indexes.push_back(RValue::getZero(context, SType::getInt(context, 32)));
	indexes.push_back(ConstantInt::get(*SType::getInt(context, 32), item->first));

	auto getEl = GetElementPtrInst::Create(baseValue, indexes, "", context);
	return RValue(getEl, item->second);
}

RValue NMemberVariable::loadUnion(CodeContext& context, RValue& baseValue, SUnionType* unionType) const
{
	auto member = getMemberName();
	auto item = unionType->getItem(member);
	if (!item) {
		context.addError(getName() +" doesn't have member " + member, memberName);
		return RValue();
	}

	auto ptr = SType::getPointer(context, item);
	auto castEl = new BitCastInst(baseValue, *ptr, "", context);
	return RValue(castEl, item);
}

RValue NMemberVariable::loadEnum(CodeContext& context, SEnumType* enumType) const
{
	auto member = getMemberName();
	auto item = enumType->getItem(member);
	if (!item) {
		context.addError(getName() +" doesn't have member " + member, memberName);
		return RValue();
	}
	auto val = RValue::getValue(context, *item);
	return RValue(val.value(), enumType);
}

RValue NDereference::loadVar(CodeContext& context)
{
	auto var = derefVar->loadVar(context);
	if (!var) {
		return var;
	} else if (!var.stype()->isPointer()) {
		context.addError("variable " + getName() + " can not be dereferenced", atTok);
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
	context.storeLocalSymbol(LValue(stackAlloc, stype), getName());
}

void NVariableDecl::genCode(CodeContext& context)
{
	auto initValue = initExp? initExp->genValue(context) : RValue();
	auto varType = type->getType(context);

	if (!varType) {
		return;
	} else if (varType->isAuto()) {
		if (!initExp) { // auto type requires initialization
			auto token = static_cast<NNamedType*>(type)->getToken();
			context.addError("auto variable type requires initialization", token);
			return;
		} else if (!initValue) {
			return;
		}
		varType = initValue.stype();
	} else if (!SType::validate(context, getNameToken(), varType)) {
		return;
	}

	auto name = getName();
	if (context.loadSymbolCurr(name)) {
		context.addError("variable " + name + " already defined", getNameToken());
		return;
	}

	auto var = LValue(new AllocaInst(*varType, name, context), varType);
	context.storeLocalSymbol(var, name);

	if (initValue) {
		Inst::CastTo(context, eqToken, initValue, varType);
		new StoreInst(initValue, var, context);
	}
}

void NGlobalVariableDecl::genCode(CodeContext& context)
{
	if (initExp && !initExp->isConstant()) {
		context.addError("global variables only support constant value initializer", eqToken);
		return;
	}
	auto initValue = initExp? initExp->genValue(context) : RValue();
	auto varType = type->getType(context);

	if (!varType) {
		return;
	} else if (varType->isAuto()) {
		if (!initExp) { // auto type requires initialization
			auto token = static_cast<NNamedType*>(type)->getToken();
			context.addError("auto variable type requires initialization", token);
			return;
		} else if (!initValue) {
			return;
		}
		varType = initValue.stype();
	} else if (!SType::validate(context, getNameToken(), varType)) {
		return;
	}

	if (initValue) {
		if (initValue.isNullPtr()) {
			Inst::CastTo(context, eqToken, initValue, varType);
		} else if (varType != initValue.stype()) {
			context.addError("global variable initialization requires exact type matching", eqToken);
			return;
		}
	}

	auto name = getName();
	if (context.loadSymbolCurr(name)) {
		context.addError("variable " + name + " already defined", getNameToken());
		return;
	}

	auto var = new GlobalVariable(*context.getModule(), *varType, false, GlobalValue::ExternalLinkage, (Constant*) initValue.value(), name);
	context.storeGlobalSymbol(LValue(var, varType), name);
}

bool NVariableDeclGroup::addMembers(vector<pair<string, SType*> >& structVector, set<string>& memberNames, CodeContext& context)
{
	auto stype = type->getType(context);
	if (!stype) {
		return false;
	} else if (stype->isAuto()) {
		auto token = static_cast<NNamedType*>(type)->getToken();
		context.addError("struct members must not have auto type", token);
		return false;
	}
	bool valid = true;
	for (auto var : *variables) {
		if (var->hasInit())
			context.addError("structs don't support variable initialization", var->getEqToken());
		auto name = var->getName();
		auto res = memberNames.insert(name);
		if (!res.second) {
			auto token = static_cast<NDeclaration*>(var)->getNameToken();
			context.addError("member name " + name + " already declared", token);
			valid = false;
			continue;
		}
		structVector.push_back(make_pair(name, stype));
	}
	return valid;
}

void NAliasDeclaration::genCode(CodeContext& context)
{
	auto realType = type->getType(context);
	if (!realType) {
		return;
	} else if (realType->isAuto()) {
		auto token = static_cast<NNamedType*>(type)->getToken();
		context.addError("can not create alias to auto type", token);
		return;
	}

	SAliasType::createAlias(context, getName(), realType);
}

void NStructDeclaration::genCode(CodeContext& context)
{
	auto structName = getName();
	auto utype = SUserType::lookup(context, structName);
	if (utype) {
		context.addError(structName + " type already declared", getNameToken());
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

void NEnumDeclaration::genCode(CodeContext& context)
{
	int64_t val = 0;
	set<string> names;
	vector<pair<string,int64_t>> structure;

	for (auto item : *variables) {
		auto name = item->getName();
		auto res = names.insert(name);
		if (!res.second) {
			auto token = static_cast<NDeclaration*>(item)->getNameToken();
			context.addError("enum member name " + name + " already declared", token);
			continue;
		}
		if (item->hasInit()) {
			auto initExp = item->getInitExp();
			if (!initExp->isConstant()) {
				context.addError("enum initializer must be a constant", item->getEqToken());
				continue;
			}
			auto constVal = static_cast<NConstant*>(initExp);
			if (!constVal->isIntConst()) {
				context.addError("enum initializer must be an int-like constant", constVal->getToken());
				continue;
			}
			auto intVal = static_cast<NIntLikeConst*>(constVal);
			val = intVal->getIntVal(context).getSExtValue();
		}
		structure.push_back(make_pair(name, val++));
	}
	SUserType::createEnum(context, getName(), structure);
}

void NFunctionPrototype::genCode(CodeContext& context)
{
	context.addError("Called NFunctionPrototype::genCode; use genFunction instead.", nullptr);
}

SFunction NFunctionPrototype::genFunction(CodeContext& context)
{
	auto funcName = getName();
	auto sym = context.loadSymbol(funcName);
	if (sym) {
		if (sym.isFunction())
			return static_cast<SFunction&>(sym);
		context.addError("variable " + funcName + " already defined", getNameToken());
		return SFunction();
	}

	auto funcType = getFunctionType(context, getNameToken());
	return funcType? SFunction::create(context, funcName, funcType) : SFunction();
}

void NFunctionPrototype::genCodeParams(SFunction function, CodeContext& context) const
{
	int i = 0;
	for (auto arg = function.arg_begin(); arg != function.arg_end(); arg++, i++) {
		auto param = params->at(i);
		arg->setName(param->getName());
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
		context.addError("function " + prototype->getName() + " already declared", prototype->getNameToken());
		return;
	}

	if (body->empty() || !body->back()->isTerminator()) {
		auto returnType = function.returnTy();
		if (returnType->isVoid())
			body->addItem(new NReturnStatement);
		else
			context.addError("no return for a non-void function", prototype->getNameToken());
	}

	if (prototype->getFunctionType(context, prototype->getNameToken()) != function.stype()) {
		context.addError("function type for " + prototype->getName() + " doesn't match definition", prototype->getNameToken());
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
			context.addError("function " + func.name().str() + " declared void, but non-void return found", retToken);
			return;
		}
	} else if (!value) {
		context.addError("function " + func.name().str() + " declared non-void, but void return found", retToken);
		return;
	}
	auto returnVal = value? value->genValue(context) : RValue();
	if (returnVal)
		Inst::CastTo(context, retToken, returnVal, funcReturn);
	ReturnInst::Create(context, returnVal, context);
	context.pushBlock(context.createBlock());
}

void NLoopStatement::genCode(CodeContext& context)
{
	auto bodyBlock = context.createContinueBlock();
	auto endBlock = context.createBreakBlock();

	context.pushLocalTable();

	BranchInst::Create(bodyBlock, context);
	context.pushBlock(bodyBlock);
	body->genCode(context);
	BranchInst::Create(bodyBlock, context);

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popLoopBranchBlocks();
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
	Inst::Branch(trueBlock, falseBlock, condition, lparen, context);

	context.pushBlock(bodyBlock);
	body->genCode(context);
	BranchInst::Create(condBlock, context);

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popLoopBranchBlocks();
}

ConstantInt* NSwitchCase::getValue(CodeContext& context)
{
	return ConstantInt::get(context, value->getIntVal(context));
}

void NSwitchStatement::genCode(CodeContext& context)
{
	auto switchValue = value->genValue(context);
	Inst::CastTo(context, lparen, switchValue, SType::getInt(context, 32));

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
				context.addError("switch case values are not unique", caseItem->getToken());
			switchInst->addCase(val, caseBlock);
		} else {
			if (hasDefault)
				context.addError("switch statement has more than one default", caseItem->getToken());
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
	Inst::Branch(bodyBlock, endBlock, condition, semiCol2, context);

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

	Inst::Branch(ifBlock, elseBlock, condition, lparen, context);

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
	auto labelName = getName();
	auto label = context.getLabelBlock(labelName);
	if (label) {
		if (!label->isPlaceholder) {
			context.addError("label " + labelName + " already defined", getNameToken());
			return;
		}
		// a used label is no longer a placeholder
		label->isPlaceholder = false;
	} else {
		label = context.createLabelBlock(getNameToken(), false);
	}
	BranchInst::Create(label->block, context);
	context.pushBlock(label->block);
}

void NGotoStatement::genCode(CodeContext& context)
{
	auto labelName = getName();
	auto skip = context.createBlock();
	auto label = context.getLabelBlock(labelName);
	if (!label) {
		// trying to jump to a non-existant label. create place holder and
		// later check if it's used at the end of the function.
		label = context.createLabelBlock(getNameToken(), true);
	}
	BranchInst::Create(label->block, context);
	context.pushBlock(skip);
}

void NLoopBranch::genCode(CodeContext& context)
{
	BasicBlock* block;
	string typeName;
	auto brLevel = level? level->getIntVal(context).getSExtValue() : 1;

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
		context.addError("undefined loop branch type: " + to_string(type), token);
		return;
	}
	BranchInst::Create(block, context);
	context.pushBlock(context.createBlock());
	return;
error:
	context.addError(typeName + " invalid outside a loop/switch block", token);
}

void NDeleteStatement::genCode(CodeContext& context)
{
	static string freeName = "free";

	auto bytePtr = SType::getPointer(context, SType::getInt(context, 8));
	auto func = context.loadSymbol(freeName);
	if (!func) {
		vector<SType*> args;
		args.push_back(bytePtr);
		auto retType = SType::getVoid(context);
		auto funcType = SType::getFunction(context, retType, args);

		func = SFunction::create(context, freeName, funcType);
	} else if (!func.isFunction()) {
		context.addError("Compiler Error: free not function", token);
		return;
	}

	auto ptr = variable->genValue(context);
	if (!ptr) {
		return;
	} else if (!ptr.stype()->isPointer()) {
		context.addError("delete requires pointer type", token);
		return;
	}

	vector<Value*> exp_list;
	exp_list.push_back(new BitCastInst(ptr, *bytePtr, "", context));

	CallInst::Create(static_cast<SFunction&>(func), exp_list, "", context);
}

RValue NAssignment::genValue(CodeContext& context)
{
	auto lhsVar = lhs->loadVar(context);

	if (!lhsVar)
		return RValue();

	BasicBlock* endBlock = nullptr;

	if (oper == ParserBase::TT_DQ_MARK) {
		auto condExp = Inst::Load(context, lhsVar);
		Inst::CastTo(context, opTok, condExp, SType::getBool(context));

		auto trueBlock = context.createBlock();
		endBlock = context.createBlock();

		BranchInst::Create(endBlock, trueBlock, condExp, context);
		context.pushBlock(trueBlock);
	}

	auto rhsExp = rhs->genValue(context);
	if (!rhsExp)
		return RValue();

	if (oper != '=' && oper != ParserBase::TT_DQ_MARK) {
		auto lhsLocal = Inst::Load(context, lhsVar);
		rhsExp = Inst::BinaryOp(oper, opTok, lhsLocal, rhsExp, context);
	}
	Inst::CastTo(context, opTok, rhsExp, lhsVar.stype());
	new StoreInst(rhsExp, lhsVar, context);

	if (oper == ParserBase::TT_DQ_MARK) {
		BranchInst::Create(endBlock, context);
		context.pushBlock(endBlock);
	}

	return rhsExp;
}

RValue NTernaryOperator::genValue(CodeContext& context)
{
	auto condExp = condition->genValue(context);
	Inst::CastTo(context, colTok, condExp, SType::getBool(context));

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
		context.addError("return types of ternary must match", colTok);
	return retVal;
}

RValue NNewExpression::genValue(CodeContext& context)
{
	static string mallocName = "malloc";

	auto funcVal = context.loadSymbol(mallocName);
	if (!funcVal) {
		vector<SType*> args;
		args.push_back(SType::getInt(context, 64));
		auto retType = SType::getPointer(context, SType::getInt(context, 8));
		auto funcType = SType::getFunction(context, retType, args);

		funcVal = SFunction::create(context, mallocName, funcType);
	} else if (!funcVal.isFunction()) {
		context.addError("Compiler Error: malloc not function", token);
		return RValue();
	}

	auto nType = type->getType(context);
	if (!nType) {
		return RValue();
	} else if (nType->isAuto()) {
		context.addError("can't call new on auto type", token);
		return RValue();
	} else if (nType->isVoid()) {
		context.addError("can't call new on void type", token);
		return RValue();
	}

	vector<Value*> exp_list;
	exp_list.push_back(Inst::SizeOf(context, token, nType));

	auto func = static_cast<SFunction&>(funcVal);
	auto call = CallInst::Create(func, exp_list, "", context);
	auto ptr = RValue(call, func.returnTy());
	auto ptrType = SType::getPointer(context, nType);

	return RValue(new BitCastInst(ptr, *ptrType, "", context), ptrType);
}

RValue NLogicalOperator::genValue(CodeContext& context)
{
	auto saveBlock = context.currBlock();
	auto firstBlock = context.createBlock();
	auto secondBlock = context.createBlock();
	auto trueBlock = (oper == ParserBase::TT_LOG_AND)? firstBlock : secondBlock;
	auto falseBlock = (oper == ParserBase::TT_LOG_AND)? secondBlock : firstBlock;

	auto lhsExp = Inst::Branch(trueBlock, falseBlock, lhs, opTok, context);

	context.pushBlock(firstBlock);
	auto rhsExp = rhs->genValue(context);
	Inst::CastTo(context, opTok, rhsExp, SType::getBool(context));
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

	return Inst::Cmp(oper, opTok, lhsExp, rhsExp, context);
}

RValue NBinaryMathOperator::genValue(CodeContext& context)
{
	auto lhsExp = lhs->genValue(context);
	auto rhsExp = rhs->genValue(context);

	return Inst::BinaryOp(oper, opTok, lhsExp, rhsExp, context);
}

RValue NNullCoalescing::genValue(CodeContext& context)
{
	RValue rhsExp, retVal;
	auto lhsExp = lhs->genValue(context);
	auto condition = lhsExp;

	Inst::CastTo(context, opTok, condition, SType::getBool(context), false);
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
		context.addError("return types of null coalescing operator must match", opTok);
	return retVal;
}

RValue NSizeOfOperator::genValue(CodeContext& context)
{
	switch (type) {
	case DATA:
		return Inst::SizeOf(context, sizeTok, dtype);
	case EXP:
		return Inst::SizeOf(context, sizeTok, exp);
	case NAME:
		return Inst::SizeOf(context, sizeTok, name->str);
	default:
		// shouldn't happen
		return RValue();
	}
}

RValue NUnaryMathOperator::genValue(CodeContext& context)
{
	auto unaryExp = unary->genValue(context);
	auto type = unaryExp.stype();

	switch (oper) {
	case '+':
	case '-':
		return Inst::BinaryOp(oper, opTok, RValue::getZero(context, type), unaryExp, context);
	case '!':
		return Inst::Cmp(ParserBase::TT_EQ, opTok, RValue::getZero(context, type), unaryExp, context);
	case '~':
		return Inst::BinaryOp('^', opTok, RValue::getAllOne(context, type), unaryExp, context);
	default:
		context.addError("invalid unary operator " + to_string(oper), opTok);
		return RValue();
	}
}

RValue NFunctionCall::genValue(CodeContext& context)
{
	auto funcName = getName();
	auto sym = context.loadSymbol(funcName);
	if (!sym) {
		context.addError("symbol " + funcName + " not defined", name);
		return sym;
	}
	auto deSym = Inst::Deref(context, sym, true);
	if (!deSym.isFunction()) {
		context.addError("symbol " + funcName + " doesn't reference a function", name);
		return RValue();
	}

	auto func = static_cast<SFunction&>(deSym);
	auto argCount = arguments->size();
	auto paramCount = func.numParams();
	if (argCount != paramCount) {
		context.addError("argument count for " + func.name().str() + " function invalid, "
			+ to_string(argCount) + " arguments given, but " + to_string(paramCount) + " required.", name);
		return RValue();
	}
	vector<Value*> exp_list;
	int i = 0;
	for (auto arg : *arguments) {
		auto argExp = arg->genValue(context);
		Inst::CastTo(context, name, argExp, func.getParam(i++));
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
	if (!varPtr || !varVal)
		return RValue();
	auto incType = varVal.stype()->isPointer()? SType::getInt(context, 32) : varVal.stype();

	auto result = Inst::BinaryOp(oper, opTok, varVal, RValue::getNumVal(context, incType, oper == ParserBase::TT_INC? 1:-1), context);
	new StoreInst(result, varPtr, context);

	return isPostfix? varVal : RValue(result, varVal.stype());
}

APSInt NBoolConst::getIntVal(CodeContext& context)
{
	return APSInt(APInt(1, bvalue));
}

RValue NIntLikeConst::genValue(CodeContext& context)
{
	return RValue::getValue(context, getIntVal(context));
}

RValue NNullPointer::genValue(CodeContext& context)
{
	return RValue::getNullPtr(context, SType::getInt(context, 8));
}

RValue NStringLiteral::genValue(CodeContext& context)
{
	auto strVal = getStrVal();
	auto arrData = ConstantDataArray::getString(context, strVal, true);
	auto arrTy = SType::getArray(context, SType::getInt(context, 8), strVal.size() + 1);
	auto arrTyPtr = SType::getPointer(context, arrTy);
	auto gVar = new GlobalVariable(*context.getModule(), *arrTy, true, GlobalValue::PrivateLinkage, arrData);
	auto zero = ConstantInt::get(*SType::getInt(context, 32), 0);

	std::vector<Constant*> idxs;
	idxs.push_back(zero);

	auto strPtr = ConstantExpr::getGetElementPtr(gVar, idxs);

	return RValue(strPtr, arrTyPtr);
}

APSInt NIntConst::getIntVal(CodeContext& context)
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

	auto data = NConstant::getValueAndSuffix(getStrVal());
	if (data.size() > 1) {
		auto suf = suffix.find(data[1]);
		if (suf == suffix.end())
			context.addError("invalid integer suffix: " + data[1], value);
		else
			type = suf->second;
	}
	string intVal(data[0], base == 10? 0:2);
	return APSInt(APInt(type->size(), intVal, base), type->isUnsigned());
}

RValue NFloatConst::genValue(CodeContext& context)
{
	static const map<string, SType*> suffix = {
		{"f", SType::getFloat(context)},
		{"d", SType::getFloat(context, true)} };
	auto type = SType::getFloat(context, true);

	auto data = NConstant::getValueAndSuffix(getStrVal());
	if (data.size() > 1) {
		auto suf = suffix.find(data[1]);
		if (suf == suffix.end())
			context.addError("invalid float suffix: " + data[1], value);
		else
			type = suf->second;
	}
	auto fp = ConstantFP::get(*type, data[0]);
	return RValue(fp, type);
}

APSInt NCharConst::getIntVal(CodeContext& context)
{
	auto strVal = getStrVal();
	char cVal = strVal.at(0);
	if (cVal == '\\' && strVal.length() > 1) {
		switch (strVal.at(1)) {
		case '0': cVal = '\0'; break;
		case 'a': cVal = '\a'; break;
		case 'b': cVal = '\b'; break;
		case 'e': cVal =   27; break;
		case 'f': cVal = '\f'; break;
		case 'n': cVal = '\n'; break;
		case 'r': cVal = '\r'; break;
		case 't': cVal = '\t'; break;
		case 'v': cVal = '\v'; break;
		default: cVal = strVal.at(1);
		}
	}
	return APSInt(APInt(8, cVal, true));
}

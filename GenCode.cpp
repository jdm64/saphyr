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
#include "Util.h"

NStatementList* programBlock;

void CodeContext::genCode(NStatementList stms)
{
	stms.genCode(*this);

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

Type* NBaseType::getType(CodeContext& context)
{
	switch (type) {
	case BaseDataType::VOID:
		return Type::getVoidTy(context.getContext());
	case BaseDataType::BOOL:
		return Type::getInt1Ty(context.getContext());
	case BaseDataType::INT8:
		return Type::getInt8Ty(context.getContext());
	case BaseDataType::INT16:
		return Type::getInt16Ty(context.getContext());
	case BaseDataType::INT:
	case BaseDataType::INT32:
		return Type::getInt32Ty(context.getContext());
	case BaseDataType::INT64:
		return Type::getInt64Ty(context.getContext());
	case BaseDataType::FLOAT:
		return Type::getFloatTy(context.getContext());
	case BaseDataType::DOUBLE:
		return Type::getDoubleTy(context.getContext());
	case BaseDataType::AUTO:
	default:
		return nullptr;
	}
}

Type* NArrayType::getType(CodeContext& context)
{
	auto size = ConstantInt::get(Type::getIntNTy(context.getContext(), 64), *strSize, 10);
	auto arrSize = size->getSExtValue();

	if (arrSize < 0) {
		context.addError("Array size must be non-negative");
		return nullptr;
	}
	auto btype = baseType->getType(context);
	return btype? ArrayType::get(btype, arrSize) : nullptr;
}

Value* NVariable::genValue(CodeContext& context)
{
	auto var = loadVar(context);
	return var? new LoadInst(var, "", context.currBlock()) : nullptr;
}

Value* NVariable::loadVar(CodeContext& context)
{
	auto var = context.loadVar(name);
	if (!var)
		context.addError("variable " + *name + " not declared");
	return var;
}

Value* NArrayVariable::loadVar(CodeContext& context)
{
	auto zero = ConstantInt::getNullValue(IntegerType::get(context.getContext(), 32));
	auto indexVal = index->genValue(context);

	if (!indexVal) {
		return nullptr;
	} else if (indexVal->getType()->isArrayTy()) {
		context.addError("array index is not able to be cast to an int");
		return nullptr;
	}
	typeCastMatch(indexVal, IntegerType::get(context.getContext(), 64), context);

	vector<Value*> indexes;
	indexes.push_back(zero);
	indexes.push_back(indexVal);

	auto var = context.loadVar(name);
	if (!var) {
		context.addError("variable " + *name + " not declared");
		return nullptr;
	}
	// temporarly load variable to determin type
	auto load = new LoadInst(var, "");
	auto isArray = load->getType()->isArrayTy();
	delete load;
	if (!isArray) {
		context.addError("variable " + *name + " is not an array");
		return nullptr;
	}
	return GetElementPtrInst::Create(var, indexes, "", context.currBlock());
}

void NParameter::genCode(CodeContext& context)
{
	auto stackAlloc = new AllocaInst(type->getType(context), "", context.currBlock());
	new StoreInst(arg, stackAlloc, context.currBlock());
	context.storeLocalVar(stackAlloc, name);
}

void NVariableDecl::genCode(CodeContext& context)
{
	auto initValue = initExp? initExp->genValue(context) : nullptr;
	auto varType = type->getType(context);

	if (!varType) { // auto type
		if (!initValue) { // auto type requires initialization
			context.addError("auto variable type requires initialization");
			return;
		}
		varType = initValue->getType();
	}

	auto name = getName();
	if (context.loadVarCurr(name)) {
		context.addError("variable " + *name + " already defined");
		return;
	}

	auto var = new AllocaInst(varType, *name, context.currBlock());
	context.storeLocalVar(var, name);

	if (initValue) {
		typeCastMatch(initValue, var->getType()->getPointerElementType(), context);
		new StoreInst(initValue, var, context.currBlock());
	}
}

void NGlobalVariableDecl::genCode(CodeContext& context)
{
	if (initExp && initExp->getNodeType() != NodeType::IntConst && initExp->getNodeType() != NodeType::FloatConst) {
		context.addError("global variables only support constant value initializer");
		return;
	}
	auto initValue = initExp? initExp->genValue(context) : nullptr;
	auto varType = type->getType(context);

	if (!varType) { // auto type
		if (!initValue) { // auto type requires initialization
			context.addError("auto variable type requires initialization");
			return;
		}
		varType = initValue->getType();
	}
	if (initValue && varType != initValue->getType()) {
		context.addError("global variable initialization requires exact type matching");
		return;
	}

	auto name = getName();
	if (context.loadVarCurr(name)) {
		context.addError("variable " + *name + " already defined");
		return;
	}

	auto var = new GlobalVariable(*context.getModule(), varType, false, GlobalValue::ExternalLinkage, (Constant*) initValue, *name);
	context.storeGlobalVar(var, name);
}

void NFunctionPrototype::genCode(CodeContext& context)
{
	context.addError("Called NFunctionPrototype::genCode; use genFunction instead.");
}

Function* NFunctionPrototype::genFunction(CodeContext& context)
{
	auto function = context.getFunction(name);
	if (function)
		return function;

	auto funcType = getFunctionType(context);
	return Function::Create(funcType, GlobalValue::ExternalLinkage, *name, context.getModule());
}

void NFunctionPrototype::genCodeParams(Function* function, CodeContext& context)
{
	int i = 0;
	for (auto arg = function->arg_begin(); arg != function->arg_end(); arg++) {
		auto param = params->at(i++);
		arg->setName(*param->getName());
		param->setArgument(arg);
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
		auto returnType = function->getFunctionType()->getReturnType();
		if (returnType->isVoidTy())
			body->addItem(new NReturnStatement);
		else
			context.addError("no return for a non-void function");
	}

	if (prototype->getFunctionType(context) != function->getFunctionType()) {
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
	auto funcReturn = func->getReturnType();

	if (funcReturn->isVoidTy()) {
		if (value) {
			context.addError(func->getName().str() + " function declared void, but non-void return found");
			return;
		}
	} else if (!value) {
		context.addError(func->getName().str() + " function declared non-void, but void return found");
		return;
	}
	auto returnVal = value? value->genValue(context) : nullptr;
	if (returnVal)
		typeCastMatch(returnVal, funcReturn, context);
	ReturnInst::Create(context.getContext(), returnVal, context.currBlock());
}

void NWhileStatement::genCode(CodeContext& context)
{
	auto condBlock = context.createBlock();
	auto bodyBlock = context.createBlock();
	auto endBlock = context.createBlock();

	auto startBlock = isDoWhile? bodyBlock : condBlock;
	auto trueBlock = isUntil? endBlock : bodyBlock;
	auto falseBlock = isUntil? bodyBlock : endBlock;

	context.pushLocalTable();
	context.pushContinueBlock(condBlock);
	context.pushRedoBlock(bodyBlock);
	context.pushBreakBlock(endBlock);

	BranchInst::Create(startBlock, context.currBlock());

	context.pushBlock(condBlock);
	auto condValue = condition? condition->genValue(context) : ConstantInt::getTrue(context.getContext());;
	typeCastMatch(condValue, Type::getInt1Ty(context.getContext()), context);
	BranchInst::Create(trueBlock, falseBlock, condValue, context.currBlock());

	context.pushBlock(bodyBlock);
	body->genCode(context);
	BranchInst::Create(condBlock, context.currBlock());

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popContinueBlock();
	context.popRedoBlock();
	context.popBreakBlock();
}

void NSwitchStatement::genCode(CodeContext& context)
{
	auto switchValue = value->genValue(context);
	typeCastMatch(switchValue, Type::getIntNTy(context.getContext(), 32), context);

	auto caseBlock = context.createBlock();
	auto endBlock = context.createBlock(), defaultBlock = endBlock;
	auto switchInst = SwitchInst::Create(switchValue, defaultBlock, cases->size(), context.currBlock());

	context.pushLocalTable();
	context.pushBreakBlock(endBlock);

	set<int64_t> unique;
	bool hasDefault = false;
	for (auto caseItem : *cases) {
		if (caseItem->isValueCase()) {
			auto val = static_cast<ConstantInt*>(caseItem->genValue(context));
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
			BranchInst::Create(caseBlock, context.currBlock());
			context.pushBlock(caseBlock);
		}
	}
	switchInst->setDefaultDest(defaultBlock);

	// NOTE: the last case will create a dangling block which needs a terminator.
	BranchInst::Create(endBlock, context.currBlock());

	context.popLocalTable();
	context.popBreakBlock();
	context.pushBlock(endBlock);
}

void NForStatement::genCode(CodeContext& context)
{
	auto condBlock = context.createBlock();
	auto bodyBlock = context.createBlock();
	auto postBlock = context.createBlock();
	auto endBlock = context.createBlock();

	context.pushLocalTable();
	context.pushContinueBlock(postBlock);
	context.pushRedoBlock(bodyBlock);
	context.pushBreakBlock(endBlock);

	preStm->genCode(context);
	BranchInst::Create(condBlock, context.currBlock());

	context.pushBlock(condBlock);
	auto condValue = condition? condition->genValue(context) : ConstantInt::getTrue(context.getContext());
	typeCastMatch(condValue, Type::getInt1Ty(context.getContext()), context);
	BranchInst::Create(bodyBlock, endBlock, condValue, context.currBlock());

	context.pushBlock(bodyBlock);
	body->genCode(context);
	BranchInst::Create(postBlock, context.currBlock());

	context.pushBlock(postBlock);
	postExp->genCode(context);
	BranchInst::Create(condBlock, context.currBlock());

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popContinueBlock();
	context.popRedoBlock();
	context.popBreakBlock();
}

void NIfStatement::genCode(CodeContext& context)
{
	auto ifBlock = context.createBlock();
	auto elseBlock = context.createBlock();
	auto endBlock = elseBody? context.createBlock() : elseBlock;

	context.pushLocalTable();

	auto condValue = condition->genValue(context);
	typeCastMatch(condValue, Type::getInt1Ty(context.getContext()), context);
	BranchInst::Create(ifBlock, elseBlock, condValue, context.currBlock());

	context.pushBlock(ifBlock);
	body->genCode(context);
	BranchInst::Create(endBlock, context.currBlock());

	context.popLocalTable();
	context.pushLocalTable();

	context.pushBlock(elseBlock);
	if (elseBody) {
		elseBody->genCode(context);
		BranchInst::Create(endBlock, context.currBlock());
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
		label = new LabelBlock(context.createBlock(), false);
		label->block->setName(*name);
		context.setLabelBlock(name, label);
	}
	BranchInst::Create(label->block, context.currBlock());
	context.pushBlock(label->block);
}

void NGotoStatement::genCode(CodeContext& context)
{
	auto skip = context.createBlock();
	auto label = context.getLabelBlock(name);
	if (!label) {
		// trying to jump to a non-existant label. create place holder and
		// later check if it's used at the end of the function.
		label = new LabelBlock(context.createBlock(), true);
		label->block->setName(*name);
		context.setLabelBlock(name, label);
	}
	BranchInst::Create(label->block, context.currBlock());
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
		context.addError("undefined loop branch type: " + type);
		return;
	}
	BranchInst::Create(block, context.currBlock());
	context.pushBlock(context.createBlock());
	return;
error:
	context.addError("no valid context for " + typeName + " statement");
}

Value* NAssignment::genValue(CodeContext& context)
{
	auto lhsVar = lhs->loadVar(context);
	auto rhsExp = rhs->genValue(context);

	if (!(lhsVar && rhsExp))
		return nullptr;

	if (oper != '=') {
		Value* lhsLocal = new LoadInst(lhsVar, "", context.currBlock());
		typeCastUp(lhsLocal, rhsExp, context);
		rhsExp = BinaryOperator::Create(getOperator(oper, lhsLocal->getType(), context), lhsLocal, rhsExp, "", context.currBlock());
	}
	typeCastMatch(rhsExp, lhsVar->getType()->getPointerElementType(), context);
	new StoreInst(rhsExp, lhsVar, context.currBlock());

	return rhsExp;
}

Value* NTernaryOperator::genValue(CodeContext& context)
{
	auto condExp = condition->genValue(context);
	typeCastMatch(condExp, Type::getInt1Ty(context.getContext()), context);

	Value *trueExp, *falseExp, *retVal;
	if (isComplexExp(trueVal->getNodeType()) || isComplexExp(falseVal->getNodeType())) {
		auto trueBlock = context.createBlock();
		auto falseBlock = context.createBlock();
		auto endBlock = context.createBlock();

		BranchInst::Create(trueBlock, falseBlock, condExp, context.currBlock());

		context.pushBlock(trueBlock);
		trueExp = trueVal->genValue(context);
		BranchInst::Create(endBlock, context.currBlock());

		context.pushBlock(falseBlock);
		falseExp = falseVal->genValue(context);
		BranchInst::Create(endBlock, context.currBlock());

		context.pushBlock(endBlock);
		auto result = PHINode::Create(trueExp->getType(), 2, "", context.currBlock());
		result->addIncoming(trueExp, trueBlock);
		result->addIncoming(falseExp, falseBlock);
		retVal = result;
	} else {
		trueExp = trueVal->genValue(context);
		falseExp = falseVal->genValue(context);
		retVal = SelectInst::Create(condExp, trueExp, falseExp, "", context.currBlock());
	}

	if (trueExp->getType() != falseExp->getType())
		context.addError("return types of ternary must match");
	return retVal;
}

Value* NLogicalOperator::genValue(CodeContext& context)
{
	auto saveBlock = context.currBlock();
	auto firstBlock = context.createBlock();
	auto secondBlock = context.createBlock();
	auto trueBlock = (oper == ParserBase::TT_LOG_AND)? firstBlock : secondBlock;
	auto falseBlock = (oper == ParserBase::TT_LOG_AND)? secondBlock : firstBlock;

	auto lhsExp = lhs->genValue(context);
	typeCastMatch(lhsExp, Type::getInt1Ty(context.getContext()), context);
	BranchInst::Create(trueBlock, falseBlock, lhsExp, context.currBlock());

	context.pushBlock(firstBlock);
	auto rhsExp = rhs->genValue(context);
	typeCastMatch(rhsExp, Type::getInt1Ty(context.getContext()), context);
	BranchInst::Create(secondBlock, context.currBlock());

	context.pushBlock(secondBlock);
	auto result = PHINode::Create(Type::getInt1Ty(context.getContext()), 2, "", context.currBlock());
	result->addIncoming(lhsExp, saveBlock);
	result->addIncoming(rhsExp, firstBlock);

	return result;
}

Value* NCompareOperator::genValue(CodeContext& context)
{
	auto lhsExp = lhs->genValue(context);
	auto rhsExp = rhs->genValue(context);

	if (!(rhsExp && lhsExp))
		return nullptr;

	typeCastUp(lhsExp, rhsExp, context);
	auto pred = getPredicate(oper, lhsExp->getType(), context);
	auto op = rhsExp->getType()->isFloatingPointTy()? Instruction::FCmp : Instruction::ICmp;

	return CmpInst::Create(op, pred, lhsExp, rhsExp, "", context.currBlock());
}

Value* NBinaryMathOperator::genValue(CodeContext& context)
{
	auto lhsExp = lhs->genValue(context);
	auto rhsExp = rhs->genValue(context);

	if (!(lhsExp && rhsExp))
		return nullptr;

	typeCastUp(lhsExp, rhsExp, context);
	return BinaryOperator::Create(getOperator(oper, lhsExp->getType(), context), lhsExp, rhsExp, "", context.currBlock());
}

Value* NNullCoalescing::genValue(CodeContext& context)
{
	Value *rhsExp, *retVal;
	auto lhsExp = lhs->genValue(context);
	auto condition = lhsExp;

	typeCastMatch(condition, Type::getInt1Ty(context.getContext()), context);
	if (isComplexExp(rhs->getNodeType())) {
		auto trueBlock = context.currBlock();
		auto falseBlock = context.createBlock();
		auto endBlock = context.createBlock();

		BranchInst::Create(endBlock, falseBlock, condition, context.currBlock());

		context.pushBlock(falseBlock);
		rhsExp = rhs->genValue(context);
		BranchInst::Create(endBlock, context.currBlock());

		context.pushBlock(endBlock);
		auto result = PHINode::Create(lhsExp->getType(), 2, "", context.currBlock());
		result->addIncoming(lhsExp, trueBlock);
		result->addIncoming(rhsExp, falseBlock);

		retVal = result;
	} else {
		rhsExp = rhs->genValue(context);
		retVal = SelectInst::Create(condition, lhsExp, rhsExp, "", context.currBlock());
	}

	if (lhsExp->getType() != rhsExp->getType())
		context.addError("return types of null coalescing operator must match");
	return retVal;
}

Value* NUnaryMathOperator::genValue(CodeContext& context)
{
	Instruction::BinaryOps llvmOp;
	Instruction::OtherOps cmpType;

	auto unaryExp = unary->genValue(context);
	auto type = unaryExp->getType();

	switch (oper) {
	case '+':
	case '-':
		llvmOp = getOperator(oper, type, context);
		return BinaryOperator::Create(llvmOp, Constant::getNullValue(type), unaryExp, "", context.currBlock());
	case '!':
		cmpType = type->isFloatingPointTy()? Instruction::FCmp : Instruction::ICmp;
		return CmpInst::Create(cmpType, getPredicate(ParserBase::TT_EQ, type, context), Constant::getNullValue(type), unaryExp, "", context.currBlock());
	case '~':
		llvmOp = getOperator('^', type, context);
		return BinaryOperator::Create(llvmOp, Constant::getAllOnesValue(type), unaryExp, "", context.currBlock());
	default:
		context.addError("invalid unary operator " + oper);
		return nullptr;
	}
}

Value* NFunctionCall::genValue(CodeContext& context)
{
	auto func = context.getFunction(name);
	if (!func) {
		context.addError("function " + *name + " not defined");
		return nullptr;
	}
	auto argCount = arguments->size();
	auto paramCount = func->getFunctionType()->getNumParams();
	if (argCount != paramCount)
		context.addError("argument count for " + func->getName().str() + " function invalid, "
			+ to_string(argCount) + " arguments given, but " + to_string(paramCount) + " required.");
	auto funcType = func->getFunctionType();
	vector<Value*> exp_list;
	int i = 0;
	for (auto arg : *arguments) {
		auto argExp = arg->genValue(context);
		typeCastMatch(argExp, funcType->getParamType(i++), context);
		exp_list.push_back(argExp);
	}
	return CallInst::Create(func, exp_list, "", context.currBlock());
}

Value* NIncrement::genValue(CodeContext& context)
{
	auto varVal = variable->genValue(context);
	if (!varVal)
		return nullptr;

	auto op = getOperator(isIncrement? '+' : '-', varVal->getType(), context);
	auto one = varVal->getType()->isFloatingPointTy()?
		ConstantFP::get(varVal->getType(), "1.0") :
		ConstantInt::getSigned(varVal->getType(), 1);
	auto result = BinaryOperator::Create(op, varVal, one, "", context.currBlock());
	new StoreInst(result, context.loadVar(variable->getName()), context.currBlock());

	return isPostfix? varVal : result;
}

Value* NBoolConst::genValue(CodeContext& context)
{
	return value? ConstantInt::getTrue(context.getContext()) : ConstantInt::getFalse(context.getContext());
}

Value* NIntConst::genValue(CodeContext& context)
{
	static const map<string, int> suffix = {{"i8", 8}, {"i16", 16}, {"i32", 32}, {"i64", 64}};
	auto bits = 32; // default is int32

	auto data = getValueAndSuffix();
	if (data.size() > 1) {
		auto suf = suffix.find(data[1]);
		if (suf == suffix.end())
			context.addError("invalid integer suffix: " + data[1]);
		else
			bits = suf->second;
	}
	string intVal(data[0], base == 10? 0:2);
	return ConstantInt::get(Type::getIntNTy(context.getContext(), bits), intVal, base);
}

Value* NFloatConst::genValue(CodeContext& context)
{
	static const map<string, Type*> suffix = {
		{"f", Type::getFloatTy(context.getContext())},
		{"d", Type::getDoubleTy(context.getContext())} };
	auto type = Type::getDoubleTy(context.getContext());

	auto data = getValueAndSuffix();
	if (data.size() > 1) {
		auto suf = suffix.find(data[1]);
		if (suf == suffix.end())
			context.addError("invalid float suffix: " + data[1]);
		else
			type = suf->second;
	}
	return ConstantFP::get(type, data[0]);
}

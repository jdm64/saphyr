
#include <llvm-abi/ABI.hpp>
#include <llvm-abi/ArgInfo.hpp>
#include <llvm-abi/Builder.hpp>
#include <llvm-abi/TypeBuilder.hpp>
#include <llvm-abi/FunctionType.hpp>
#include "CodeContext.h"
#include "Type.h"

using namespace llvm_abi;

class SABI
{
public:

	RValue CreateCall(CodeContext& context, SFunction func, VecRValue& values);
};

class SBuilder : public Builder
{
	CodeContext& context;

public:
	SBuilder(CodeContext& context)
	: context(context) {}

	llvm_abi::IRBuilder& getEntryBuilder() override
	{
		return context.IB();
	}

	llvm_abi::IRBuilder& getBuilder() override
	{
		return context.IB();
	}
};

llvm_abi::Type convert(SType* ty)
{
	if (ty->isInteger()) {
		if (ty->isBool())
			return BoolTy;

		auto isUnsigned = ty->isUnsigned();
		switch (ty->size()) {
		case 8:
			return isUnsigned ? UInt8Ty : Int8Ty;
		case 16:
			return isUnsigned ? UInt16Ty : Int16Ty;
		case 32:
			return isUnsigned ? UInt32Ty : Int32Ty;
		case 64:
			return isUnsigned ? UInt64Ty : Int64Ty;
		}
	} else if (ty->isFloating()) {
		return ty->isDouble() ? DoubleTy : FloatTy;
	} else if (ty->isPointer()) {
		return PointerTy;
	} else if (ty->isVoid()) {
		return VoidTy;
	} else if (ty->isEnum()) {
		return convert(ty->subType());
	} else if (ty->isStruct()) {
		SStructType* sTy = static_cast<SStructType*>(ty);

		vector<llvm_abi::Type> types;
		for (auto it = sTy->begin(); it != sTy->end(); it++) {
			auto iTy = it->second[0].second.stype();
			if (iTy->isFunction()) {
				continue;
			}
			types.push_back(convert(iTy));
		}
		TypeBuilder builder;
		return builder.getStructTy(types);
	} else if (ty->isUnion()) {
		SUnionType* uTy = static_cast<SUnionType*>(ty);

		vector<llvm_abi::Type> types;
		for (auto it = uTy->begin(); it != uTy->end(); it++) {
			auto iTy = it->second;
			if (iTy->isFunction()) {
				continue;
			}
			types.push_back(convert(iTy));
		}
		TypeBuilder builder;
		return builder.getUnionTy(types);
	} else if (ty->isFunction()) {
		// TODO
	}
	return PointerTy;
}

RValue SABI::CreateCall(CodeContext& context, SFunction func, VecRValue& values)
{
	Triple triple;
	auto abi = createABI(*context.getModule(), triple);

	auto rType = convert(func.returnTy());
	vector<llvm_abi::Type> args;
	for (auto i = 0; i < func.numParams(); i++) {
		args.push_back(convert(func.getParam(i)));
	}
	
	SBuilder builder(context);
	llvm_abi::FunctionType ftype(CallingConvention::CC_CDefault, rType, args, false);

	auto callBuilder = [&](llvm::ArrayRef<llvm::Value*> encodedArgs) -> llvm::Value* {
		// TODO use ABI call
		return context.IB().CreateCall(func.value(), encodedArgs);
	};

	vector<TypedValue> typedArgs;
	for (auto v : values) {
		typedArgs.push_back({v.value(), convert(v.stype())});
	}

	auto ret = abi.get()->createCall(builder, ftype, callBuilder, typedArgs);
	return RValue(ret, func.returnTy());
}

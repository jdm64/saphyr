
#include <llvm-abi/ABI.hpp>
#include <llvm-abi/ArgInfo.hpp>
#include <llvm-abi/Builder.hpp>
#include <llvm-abi/TypeBuilder.hpp>
#include <llvm-abi/FunctionType.hpp>
#include "CodeContext.h"

using namespace llvm_abi;

class SABI
{
public:

	RValue CreateCall(CodeContext& context);
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
	} else if (ty->isStruct()) {
		SStructType* sTy = static_cast<SStructType*>(ty);

		vector<llvm_abi::Type> types;
		for (auto it = sTy->begin(); it != sTy->end(); it++) {

		}
		TypeBuilder builder;
		return builder.getStructTy(types);
	}
	
	return BoolTy;
}

RValue SABI::CreateCall(CodeContext& context)
{
	Triple triple;
	auto abi = createABI(*context.getModule(), triple);

	SBuilder builder(context);
	llvm_abi::FunctionType ftype(CallingConvention::CC_CDefault);


	abi.get().createCall(builder, );
}

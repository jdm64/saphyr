
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
			return llvm_abi::BoolTy;
		switch (ty->size()) {
		case 8:
			return llvm_abi::Int8Ty;
		}
	}
	
	return llvm_abi::BoolTy;
}

RValue SABI::CreateCall(CodeContext& context)
{
	Triple triple;
	auto abi = createABI(*context.getModule(), triple);

	SBuilder builder(context);
	llvm_abi::FunctionType ftype(CallingConvention::CC_CDefault);


	abi.get().createCall(builder, );
}

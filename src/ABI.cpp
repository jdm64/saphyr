
#include "../../llvm-abi/include/llvm-abi/ABI.hpp"
#include "../../llvm-abi/include/llvm-abi/ArgInfo.hpp"
#include "../../llvm-abi/include/llvm-abi/Builder.hpp"
#include "CodeContext.h"
#include <llvm-abi/FunctionType.hpp>

using namespace llvm_abi;

class SABI
{


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

RValue SABI::CreateCall(CodeContext& context)
{
	Triple triple;
	auto abi = createABI(*context.getModule(), triple);

	SBuilder builder(context);
	llvm_abi::FunctionType ftype(CallingConvention::CC_CDefault);


	abi.get().createCall(builder, );
}

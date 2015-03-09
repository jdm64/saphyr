
auto f(auto x)
{
}

int main()
{
	f(0);

	@()auto a;
	@(auto)int b;

	return 0;
}

========

FunctionPointer.syp: function parameter type not resolved
FunctionPointer.syp: function return type not resolved
FunctionPointer.syp: symbol f not defined
FunctionPointer.syp: function return type not resolved
FunctionPointer.syp: auto variable type requires initialization
FunctionPointer.syp: function parameter type not resolved
FunctionPointer.syp: auto variable type requires initialization
found 7 errors

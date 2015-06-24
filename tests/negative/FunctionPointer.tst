
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

FunctionPointer.syp: parameter can not be auto type
FunctionPointer.syp: function return type can not be auto
FunctionPointer.syp: symbol f not defined
FunctionPointer.syp: function return type can not be auto
FunctionPointer.syp: parameter can not be auto type
found 5 errors

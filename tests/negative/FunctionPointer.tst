
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

error: function parameter type not resolved
error: function return type not resolved
error: symbol f not defined
error: function return type not resolved
error: auto variable type requires initialization
error: function parameter type not resolved
error: auto variable type requires initialization
found 7 errors

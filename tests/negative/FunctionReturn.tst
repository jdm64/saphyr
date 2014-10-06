
void func1(int a);

int func1()
{
}

int func2()
{
	notdefined();
	return;
}

void func3()
{
	func2(4);
	return 4;
}

void func3()
{
}

========

error: function type for func1 doesn't match definition
error: symbol notdefined not defined
error: function func2 declared non-void, but void return found
error: argument count for func2 function invalid, 1 arguments given, but 0 required.
error: function func3 declared void, but non-void return found
error: function func3 already declared
found 6 errors

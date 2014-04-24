
int b = 1;
int a = b + 3;
int c = true;

int func()
{
	auto q;
	int u = 5;
	bool u;

	int d = h + u;

	return 0;
}

========

error: global variables only support constant value initializer
error: global variable initialization requires exact type matching
error: auto variable type requires initialization
error: variable u already defined
error: variable h not declared
found 5 errors


struct Bad
{
	auto z;
	int j = 9;
	int8 j;
}

struct Good
{
	int v;
	int p;
}

struct Good { int a; }

struct More { bool b; float f; }

void func()
{
	Nothing h;
	Good k;
	int w;

	int q = w.a + 5 + k.n;
}

void func2()
{
	Good x, y;
	More w;

	auto z = x + y;
	z = x + w;
}

========

error: struct members must not have auto type
error: structs don't support variable initialization
error: member name j already declared
error: Good type already declared
error: Nothing type not declared
error: auto variable type requires initialization
error: w is not a struct or union
error: k doesn't have member n
error: can not perform operation on composite types
error: can not cast complex types
error: can not perform operation on composite types
found 11 errors

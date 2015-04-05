
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
	k.e++;
}

========

Struct.syp: struct members must not have auto type
Struct.syp: structs don't support variable initialization
Struct.syp: member name j already declared
Struct.syp: Good type already declared
Struct.syp: Nothing type not declared
Struct.syp: auto variable type requires initialization
Struct.syp: w is not a struct/union/enum
Struct.syp: k doesn't have member n
Struct.syp: can not perform operation on composite types
Struct.syp: can not cast complex types
Struct.syp: can not perform operation on composite types
Struct.syp: variable k not declared
found 12 errors

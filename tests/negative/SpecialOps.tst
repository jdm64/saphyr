
struct STR { int a; }

void func()
{
	int a;
	float b;
	STR STR;

	auto z = true? a : b;

	auto y = a ?? b;

	a = sizeof void + sizeof STR;
}

========

error: return types of ternary must match
error: return types of null coalescing operator must match
error: size of void is invalid
error: STR is ambigious, both a type and a variable
found 4 errors

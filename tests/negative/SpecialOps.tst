
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

SpecialOps.syp: return types of ternary must match
SpecialOps.syp: return types of null coalescing operator must match
SpecialOps.syp: size of void is invalid
SpecialOps.syp: STR is ambigious, both a type and a variable
found 4 errors

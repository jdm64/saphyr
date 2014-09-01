
void func()
{
	int a;

	a@ = 5;

	a = q$;
}

void func2()
{
	@int a = 4.0;

	auto z = a + 3;
	auto q = a + a;
}

void func3()
{
	vec<3, int> v;
	@int p;

	v = p;
}

========

error: variable a can not be dereferenced
error: variable q not declared
error: can't cast value to pointer type
error: pointer arithmetic only valid using ++/-- operators
error: can't perform operation with two pointers
error: can not cast pointer to vec type
found 6 errors

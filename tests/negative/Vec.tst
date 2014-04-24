
void castVec()
{
	vec<2,int8> a;
	vec<3,int> b;

	if (a) {
		auto c = a + b;
	} else {
		a++;
	}
}

int func()
{
	vec<0, int> a;
	vec<1, auto> b;

	return 0;
}

========

error: can not cast vec type to bool
error: can not cast vec types of different sizes
error: can not cast vec types of different sizes
error: vec size must be greater than 0
error: auto variable type requires initialization
error: vec type can not be auto
error: auto variable type requires initialization
found 7 errors

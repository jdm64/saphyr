
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

Vec.syp: can not cast vec type to bool
Vec.syp: can not cast vec types of different sizes
Vec.syp: can not cast vec types of different sizes
Vec.syp: vec size must be greater than 0
Vec.syp: auto variable type requires initialization
Vec.syp: vec type can not be auto
Vec.syp: auto variable type requires initialization
found 7 errors

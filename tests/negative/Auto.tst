
auto test()
{
}

void test2(auto a)
{
}


int main()
{
	@auto a;
	[2]auto b;
	vec<3,auto> c;
	@()auto d;
	@(auto)int e;

	int s = sizeof auto;

	int n = new auto;

	return 0;
}

========

Auto.syp: function return type can not be auto
Auto.syp: parameter can not be auto type
Auto.syp: can't create pointer to auto type
Auto.syp: can't create array of auto types
Auto.syp: vec type can not be auto
Auto.syp: function return type can not be auto
Auto.syp: parameter can not be auto type
Auto.syp: size of auto is invalid
Auto.syp: can't call new on auto type
found 9 errors

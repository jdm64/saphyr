
union U
{
	int a, b;
}

int main()
{
	U x;

	x.t;
}

========

Union.syp: no return for a non-void function
Union.syp: x doesn't have member t
found 2 errors

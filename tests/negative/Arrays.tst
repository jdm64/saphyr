
struct S { int z; }

int arr()
{
	[4]int a;
	S w;

	a[w + 1] = 5;

	return a[w] + w[3];
}

void foo()
{
	[0]int a;

	z[4] = 5;
}

========

Arrays.syp: can not cast complex types
Arrays.syp: can not perform operation on composite types
Arrays.syp: array index is not able to be cast to an int
Arrays.syp: array index is not able to be cast to an int
Arrays.syp: variable w is not an array or vec
Arrays.syp: Array size must be positive
Arrays.syp: variable z not declared
found 7 errors

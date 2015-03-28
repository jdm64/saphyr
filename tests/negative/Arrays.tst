
struct S { int z; }

int arr()
{
	[4]int a;
	S w;

	return a[w] + w[3];
}

void foo()
{
	[0]int a;
}

========

Arrays.syp: array index is not able to be cast to an int
Arrays.syp: variable w is not an array or vec
Arrays.syp: Array size must be positive
Arrays.syp: auto variable type requires initialization
found 4 errors

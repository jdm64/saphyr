
struct S { int z; }

int arr()
{
	[4]int a;
	S w;

	return a[w] + w[3];
}

========

error: array index is not able to be cast to an int
error: variable w is not an array or vec
found 2 errors

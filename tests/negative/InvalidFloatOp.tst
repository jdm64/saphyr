
int one()
{
	float f = 3, d = 7;

	f &= d;
	f |= d;
	f <<= d;
	f >>= d;
	f ^= d;

	return f;
}

========

error: AND operator invalid for float types
error: OR operator invalid for float types
error: shift operator invalid for float types
error: shift operator invalid for float types
error: XOR operator invalid for float types
found 5 errors

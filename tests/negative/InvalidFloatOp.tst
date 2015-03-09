
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

InvalidFloatOp.syp: AND operator invalid for float types
InvalidFloatOp.syp: OR operator invalid for float types
InvalidFloatOp.syp: shift operator invalid for float types
InvalidFloatOp.syp: shift operator invalid for float types
InvalidFloatOp.syp: XOR operator invalid for float types
found 5 errors

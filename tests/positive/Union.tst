
struct LargeStruct
{
	double b;
	int64 a;
}

union TheUnion
{
	int small;
	int64 large;
	LargeStruct super;
}

TheUnion doUnion(TheUnion u)
{
	TheUnion oth;

	oth.small = u.super.b;
	oth.super.a = u.small;

	return oth;
}

int main()
{
	TheUnion a;

	a.super.a = 7;
	a.super.b = 1.2;

	return doUnion(a).small;
}

========

%TheUnion = type { %LargeStruct }
%LargeStruct = type { double, i64 }

define %TheUnion @doUnion(%TheUnion %u) {
  %1 = alloca %TheUnion
  store %TheUnion %u, %TheUnion* %1
  %oth = alloca %TheUnion
  %2 = bitcast %TheUnion* %oth to i32*
  %3 = bitcast %TheUnion* %1 to %LargeStruct*
  %4 = getelementptr %LargeStruct* %3, i32 0, i32 0
  %5 = load double* %4
  %6 = fptosi double %5 to i32
  store i32 %6, i32* %2
  %7 = bitcast %TheUnion* %oth to %LargeStruct*
  %8 = getelementptr %LargeStruct* %7, i32 0, i32 1
  %9 = bitcast %TheUnion* %1 to i32*
  %10 = load i32* %9
  %11 = sext i32 %10 to i64
  store i64 %11, i64* %8
  %12 = load %TheUnion* %oth
  ret %TheUnion %12
}

define i32 @main() {
  %a = alloca %TheUnion
  %1 = bitcast %TheUnion* %a to %LargeStruct*
  %2 = getelementptr %LargeStruct* %1, i32 0, i32 1
  %3 = sext i32 7 to i64
  store i64 %3, i64* %2
  %4 = bitcast %TheUnion* %a to %LargeStruct*
  %5 = getelementptr %LargeStruct* %4, i32 0, i32 0
  store double 1.200000e+00, double* %5
  %6 = load %TheUnion* %a
  %7 = call %TheUnion @doUnion(%TheUnion %6)
  %8 = alloca %TheUnion
  store %TheUnion %7, %TheUnion* %8
  %9 = bitcast %TheUnion* %8 to i32*
  %10 = load i32* %9
  ret i32 %10
}

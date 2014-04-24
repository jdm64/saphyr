
double other()
{
	uint64 a = 7;
	int b = 3;

	return a / b;
}

int main()
{
	uint8 a = 4;
	uint b = 6;

	return a % b;
}

========

define double @other() {
  %a = alloca i64
  %1 = sext i32 7 to i64
  store i64 %1, i64* %a
  %b = alloca i32
  store i32 3, i32* %b
  %2 = load i64* %a
  %3 = load i32* %b
  %4 = sext i32 %3 to i64
  %5 = udiv i64 %2, %4
  %6 = uitofp i64 %5 to double
  ret double %6
}

define i32 @main() {
  %a = alloca i8
  %1 = trunc i32 4 to i8
  store i8 %1, i8* %a
  %b = alloca i32
  store i32 6, i32* %b
  %2 = load i8* %a
  %3 = load i32* %b
  %4 = zext i8 %2 to i32
  %5 = urem i32 %4, %3
  ret i32 %5
}


void uintCmp()
{
	uint f = 39;
	int8 d = 50;
	bool a;

	a = f < d;
	a = d > f;
	a = f <= d;
	a = f >= d;
	a = f != d;
	a = f == d;
}

int main()
{
	int f = 39;
	uint8 d = 50;
	bool a;

	a = f < d;
	a = d > f;
	a = f <= d;
	a = f >= d;
	a = f != d;
	a = f == d;

	return a;
}

========

define void @uintCmp() {
  %f = alloca i32
  store i32 39, i32* %f
  %d = alloca i8
  %1 = trunc i32 50 to i8
  store i8 %1, i8* %d
  %a = alloca i1
  %2 = load i32* %f
  %3 = load i8* %d
  %4 = sext i8 %3 to i32
  %5 = icmp ult i32 %2, %4
  store i1 %5, i1* %a
  %6 = load i8* %d
  %7 = load i32* %f
  %8 = sext i8 %6 to i32
  %9 = icmp ugt i32 %8, %7
  store i1 %9, i1* %a
  %10 = load i32* %f
  %11 = load i8* %d
  %12 = sext i8 %11 to i32
  %13 = icmp ule i32 %10, %12
  store i1 %13, i1* %a
  %14 = load i32* %f
  %15 = load i8* %d
  %16 = sext i8 %15 to i32
  %17 = icmp uge i32 %14, %16
  store i1 %17, i1* %a
  %18 = load i32* %f
  %19 = load i8* %d
  %20 = sext i8 %19 to i32
  %21 = icmp ne i32 %18, %20
  store i1 %21, i1* %a
  %22 = load i32* %f
  %23 = load i8* %d
  %24 = sext i8 %23 to i32
  %25 = icmp eq i32 %22, %24
  store i1 %25, i1* %a
  ret void
}

define i32 @main() {
  %f = alloca i32
  store i32 39, i32* %f
  %d = alloca i8
  %1 = trunc i32 50 to i8
  store i8 %1, i8* %d
  %a = alloca i1
  %2 = load i32* %f
  %3 = load i8* %d
  %4 = zext i8 %3 to i32
  %5 = icmp slt i32 %2, %4
  store i1 %5, i1* %a
  %6 = load i8* %d
  %7 = load i32* %f
  %8 = zext i8 %6 to i32
  %9 = icmp sgt i32 %8, %7
  store i1 %9, i1* %a
  %10 = load i32* %f
  %11 = load i8* %d
  %12 = zext i8 %11 to i32
  %13 = icmp sle i32 %10, %12
  store i1 %13, i1* %a
  %14 = load i32* %f
  %15 = load i8* %d
  %16 = zext i8 %15 to i32
  %17 = icmp sge i32 %14, %16
  store i1 %17, i1* %a
  %18 = load i32* %f
  %19 = load i8* %d
  %20 = zext i8 %19 to i32
  %21 = icmp ne i32 %18, %20
  store i1 %21, i1* %a
  %22 = load i32* %f
  %23 = load i8* %d
  %24 = zext i8 %23 to i32
  %25 = icmp eq i32 %22, %24
  store i1 %25, i1* %a
  %26 = load i1* %a
  %27 = zext i1 %26 to i32
  ret i32 %27
}

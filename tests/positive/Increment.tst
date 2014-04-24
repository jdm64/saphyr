
struct S
{
	int a, b;
}

int func()
{
	return 5;
}

S func2()
{
	S s;
	return s;
}

[5]int func3()
{
	[5]int a;
	return a;
}

void call()
{
	auto a = func()++, b = func2().b++, c = func3()[3]++;
}

int main()
{
	auto a = 5, b = 2.8;

	a++;
	++b;

	--a;
	b--;

	return a;
}

========

%S = type { i32, i32 }

define i32 @func() {
  ret i32 5
}

define %S @func2() {
  %s = alloca %S
  %1 = load %S* %s
  ret %S %1
}

define [5 x i32] @func3() {
  %a = alloca [5 x i32]
  %1 = load [5 x i32]* %a
  ret [5 x i32] %1
}

define void @call() {
  %1 = call i32 @func()
  %2 = alloca i32
  store i32 %1, i32* %2
  %3 = load i32* %2
  %4 = add i32 %3, 1
  store i32 %4, i32* %2
  %a = alloca i32
  store i32 %3, i32* %a
  %5 = call %S @func2()
  %6 = alloca %S
  store %S %5, %S* %6
  %7 = getelementptr %S* %6, i32 0, i32 1
  %8 = load i32* %7
  %9 = add i32 %8, 1
  store i32 %9, i32* %7
  %b = alloca i32
  store i32 %8, i32* %b
  %10 = call [5 x i32] @func3()
  %11 = alloca [5 x i32]
  store [5 x i32] %10, [5 x i32]* %11
  %12 = sext i32 3 to i64
  %13 = getelementptr [5 x i32]* %11, i32 0, i64 %12
  %14 = load i32* %13
  %15 = add i32 %14, 1
  store i32 %15, i32* %13
  %c = alloca i32
  store i32 %14, i32* %c
  ret void
}

define i32 @main() {
  %a = alloca i32
  store i32 5, i32* %a
  %b = alloca double
  store double 2.800000e+00, double* %b
  %1 = load i32* %a
  %2 = add i32 %1, 1
  store i32 %2, i32* %a
  %3 = load double* %b
  %4 = fadd double %3, 1.000000e+00
  store double %4, double* %b
  %5 = load i32* %a
  %6 = sub i32 %5, 1
  store i32 %6, i32* %a
  %7 = load double* %b
  %8 = fsub double %7, 1.000000e+00
  store double %8, double* %b
  %9 = load i32* %a
  ret i32 %9
}

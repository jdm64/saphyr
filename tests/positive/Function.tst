
double calcTime(double a, double b);

void empty()
{
}

void run()
{
	auto a = calcTime(true, 4.8);
	auto b = calcTime(2 * a, 3 + a);
}

double calcTime(double a, double b)
{
	int8 s = a;
	uint8 t = b;
	return s * t + a + b;
}

int main()
{
	run();
	return 0;
}

========

define double @calcTime(double %a, double %b) {
  %1 = alloca double
  store double %a, double* %1
  %2 = alloca double
  store double %b, double* %2
  %3 = load double* %1
  %s = alloca i8
  %4 = fptosi double %3 to i8
  store i8 %4, i8* %s
  %5 = load double* %2
  %t = alloca i8
  %6 = fptoui double %5 to i8
  store i8 %6, i8* %t
  %7 = load i8* %s
  %8 = load i8* %t
  %9 = sext i8 %7 to i32
  %10 = zext i8 %8 to i32
  %11 = mul i32 %9, %10
  %12 = load double* %1
  %13 = sitofp i32 %11 to double
  %14 = fadd double %13, %12
  %15 = load double* %2
  %16 = fadd double %14, %15
  ret double %16
}

define void @empty() {
  ret void
}

define void @run() {
  %1 = uitofp i1 true to double
  %2 = call double @calcTime(double %1, double 4.800000e+00)
  %a = alloca double
  store double %2, double* %a
  %3 = load double* %a
  %4 = sitofp i32 2 to double
  %5 = fmul double %4, %3
  %6 = load double* %a
  %7 = sitofp i32 3 to double
  %8 = fadd double %7, %6
  %9 = call double @calcTime(double %5, double %8)
  %b = alloca double
  store double %9, double* %b
  ret void
}

define i32 @main() {
  call void @run()
  ret i32 0
}

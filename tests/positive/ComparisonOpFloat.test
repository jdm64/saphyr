
int main()
{
	float f = 3.9;
	double d = 0.5;
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

define i32 @main() {
  %f = alloca float
  %1 = fptrunc double 3.900000e+00 to float
  store float %1, float* %f
  %d = alloca double
  store double 5.000000e-01, double* %d
  %a = alloca i1
  %2 = load float* %f
  %3 = load double* %d
  %4 = fpext float %2 to double
  %5 = fcmp olt double %4, %3
  store i1 %5, i1* %a
  %6 = load double* %d
  %7 = load float* %f
  %8 = fpext float %7 to double
  %9 = fcmp ogt double %6, %8
  store i1 %9, i1* %a
  %10 = load float* %f
  %11 = load double* %d
  %12 = fpext float %10 to double
  %13 = fcmp ole double %12, %11
  store i1 %13, i1* %a
  %14 = load float* %f
  %15 = load double* %d
  %16 = fpext float %14 to double
  %17 = fcmp oge double %16, %15
  store i1 %17, i1* %a
  %18 = load float* %f
  %19 = load double* %d
  %20 = fpext float %18 to double
  %21 = fcmp one double %20, %19
  store i1 %21, i1* %a
  %22 = load float* %f
  %23 = load double* %d
  %24 = fpext float %22 to double
  %25 = fcmp oeq double %24, %23
  store i1 %25, i1* %a
  %26 = load i1* %a
  %27 = zext i1 %26 to i32
  ret i32 %27
}

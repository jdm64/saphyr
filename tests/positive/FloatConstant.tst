
int main()
{
	auto a = 1.0, b = 2.1_f, c = 3.2_d, d = 4.3e5, e = 5.4E-6;

	return c;
}

========

define i32 @main() {
  %a = alloca double
  store double 1.000000e+00, double* %a
  %b = alloca float
  store float 0x4000CCCCC0000000, float* %b
  %c = alloca double
  store double 3.200000e+00, double* %c
  %d = alloca double
  store double 4.300000e+05, double* %d
  %e = alloca double
  store double 5.400000e-06, double* %e
  %1 = load double* %c
  %2 = fptosi double %1 to i32
  ret i32 %2
}

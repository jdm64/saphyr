
double external(double d);

double here(double d);

double here(double d)
{
	return 5;
}

int main()
{
	return external(5) + here(3);
}

========

declare double @external(double)

define double @here(double %d) {
  %1 = alloca double
  store double %d, double* %1
  %2 = sitofp i32 5 to double
  ret double %2
}

define i32 @main() {
  %1 = sitofp i32 5 to double
  %2 = call double @external(double %1)
  %3 = sitofp i32 3 to double
  %4 = call double @here(double %3)
  %5 = fadd double %2, %4
  %6 = fptosi double %5 to i32
  ret i32 %6
}

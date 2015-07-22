
void switchWithDefault(int x)
{
	switch (x) {
	case 4:
		x = 3;
	case 6:
		x = 1;
		break;
	default:
		x = 7;
		break;
	}
}

double switchNoDefault(float x)
{
	switch (x) {
	case 1:
		x = 4;
	case 3:
		x = 6;
		break;
	}
	return x;	
}

int main()
{
	switchWithDefault(7);
	auto x = switchNoDefault(false);

	return x;
}

========

define void @switchWithDefault(i32 %x) {
  %1 = alloca i32
  store i32 %x, i32* %1
  %2 = load i32* %1
  switch i32 %2, label %5 [
    i32 4, label %3
    i32 6, label %4
  ]

; <label>:3                                       ; preds = %0
  store i32 3, i32* %1
  br label %4

; <label>:4                                       ; preds = %0, %3
  store i32 1, i32* %1
  br label %6

; <label>:5                                       ; preds = %0
  store i32 7, i32* %1
  br label %6

; <label>:6                                       ; preds = %5, %4
  ret void
}

define double @switchNoDefault(float %x) {
  %1 = alloca float
  store float %x, float* %1
  %2 = load float* %1
  %3 = fptosi float %2 to i32
  switch i32 %3, label %8 [
    i32 1, label %4
    i32 3, label %6
  ]

; <label>:4                                       ; preds = %0
  %5 = sitofp i32 4 to float
  store float %5, float* %1
  br label %6

; <label>:6                                       ; preds = %0, %4
  %7 = sitofp i32 6 to float
  store float %7, float* %1
  br label %8

; <label>:8                                       ; preds = %0, %6
  %9 = load float* %1
  %10 = fpext float %9 to double
  ret double %10
}

define i32 @main() {
  call void @switchWithDefault(i32 7)
  %1 = uitofp i1 false to float
  %2 = call double @switchNoDefault(float %1)
  %x = alloca double
  store double %2, double* %x
  %3 = load double* %x
  %4 = fptosi double %3 to i32
  ret i32 %4
}

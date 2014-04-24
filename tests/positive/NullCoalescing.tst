
int func()
{
	[2]int i;
	return i[1] ?? 4;
}

int other(int a)
{
	int z = 5, c = 2;

	return (z + a) ?? (2 * c);
}

int main()
{
	int x = 10;
	auto a = (2 * x) ?? 4;

	return func() ?? other(a);
}

========

define i32 @func() {
  %i = alloca [2 x i32]
  %1 = sext i32 1 to i64
  %2 = getelementptr [2 x i32]* %i, i32 0, i64 %1
  %3 = load i32* %2
  %4 = icmp ne i32 %3, 0
  %5 = select i1 %4, i32 %3, i32 4
  ret i32 %5
}

define i32 @other(i32 %a) {
  %1 = alloca i32
  store i32 %a, i32* %1
  %z = alloca i32
  store i32 5, i32* %z
  %c = alloca i32
  store i32 2, i32* %c
  %2 = load i32* %z
  %3 = load i32* %1
  %4 = add i32 %2, %3
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %9, label %6

; <label>:6                                       ; preds = %0
  %7 = load i32* %c
  %8 = mul i32 2, %7
  br label %9

; <label>:9                                       ; preds = %6, %0
  %10 = phi i32 [ %4, %0 ], [ %8, %6 ]
  ret i32 %10
}

define i32 @main() {
  %x = alloca i32
  store i32 10, i32* %x
  %1 = load i32* %x
  %2 = mul i32 2, %1
  %3 = icmp ne i32 %2, 0
  %4 = select i1 %3, i32 %2, i32 4
  %a = alloca i32
  store i32 %4, i32* %a
  %5 = call i32 @func()
  %6 = icmp ne i32 %5, 0
  br i1 %6, label %10, label %7

; <label>:7                                       ; preds = %0
  %8 = load i32* %a
  %9 = call i32 @other(i32 %8)
  br label %10

; <label>:10                                      ; preds = %7, %0
  %11 = phi i32 [ %5, %0 ], [ %9, %7 ]
  ret i32 %11
}

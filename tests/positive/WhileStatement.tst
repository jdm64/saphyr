
void func1(int x)
{
	while (x < 4)
		x++;
}

void func2(int x)
{
	do
		x--;
	while (x > 4);
}

void func3(int x)
{
	until (x != 9)
		x += 1;
}

void func4()
{
	int x = 0;
	while () {
		if (x == 5)
			break;
	}
}

int main()
{
	int x;
	do
		x -= 2;
	until (x);

	return 0;
}

========

define void @func1(i32 %x) {
  %1 = alloca i32
  store i32 %x, i32* %1
  br label %2

; <label>:2                                       ; preds = %5, %0
  %3 = load i32* %1
  %4 = icmp slt i32 %3, 4
  br i1 %4, label %5, label %8

; <label>:5                                       ; preds = %2
  %6 = load i32* %1
  %7 = add i32 %6, 1
  store i32 %7, i32* %1
  br label %2

; <label>:8                                       ; preds = %2
  ret void
}

define void @func2(i32 %x) {
  %1 = alloca i32
  store i32 %x, i32* %1
  br label %5

; <label>:2                                       ; preds = %5
  %3 = load i32* %1
  %4 = icmp sgt i32 %3, 4
  br i1 %4, label %5, label %8

; <label>:5                                       ; preds = %2, %0
  %6 = load i32* %1
  %7 = add i32 %6, -1
  store i32 %7, i32* %1
  br label %2

; <label>:8                                       ; preds = %2
  ret void
}

define void @func3(i32 %x) {
  %1 = alloca i32
  store i32 %x, i32* %1
  br label %2

; <label>:2                                       ; preds = %5, %0
  %3 = load i32* %1
  %4 = icmp ne i32 %3, 9
  br i1 %4, label %8, label %5

; <label>:5                                       ; preds = %2
  %6 = load i32* %1
  %7 = add i32 %6, 1
  store i32 %7, i32* %1
  br label %2

; <label>:8                                       ; preds = %2
  ret void
}

define void @func4() {
  %x = alloca i32
  store i32 0, i32* %x
  br label %1

; <label>:1                                       ; preds = %2, %0
  br i1 true, label %2, label %5

; <label>:2                                       ; preds = %1
  %3 = load i32* %x
  %4 = icmp eq i32 %3, 5
  br i1 %4, label %5, label %1

; <label>:5                                       ; preds = %2, %1
  ret void
}

define i32 @main() {
  %x = alloca i32
  br label %4

; <label>:1                                       ; preds = %4
  %2 = load i32* %x
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %7, label %4

; <label>:4                                       ; preds = %1, %0
  %5 = load i32* %x
  %6 = sub i32 %5, 2
  store i32 %6, i32* %x
  br label %1

; <label>:7                                       ; preds = %1
  ret i32 0
}

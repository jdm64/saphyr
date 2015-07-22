
@[4]int newArr(int val)
{
	auto p = new [4]int;
	for (int i = 0; i < 4; i++) {
		p[i] = val;
	}
	return p;
}

int main()
{
	auto x = newArr(5);

	auto tmp = x[4];

	delete x;

	return tmp;
}

========

define [4 x i32]* @newArr(i32 %val) {
  %1 = alloca i32
  store i32 %val, i32* %1
  %2 = call i8* @malloc(i64 16)
  %3 = bitcast i8* %2 to [4 x i32]*
  %p = alloca [4 x i32]*
  store [4 x i32]* %3, [4 x i32]** %p
  %i = alloca i32
  store i32 0, i32* %i
  br label %4

; <label>:4                                       ; preds = %13, %0
  %5 = load i32* %i
  %6 = icmp slt i32 %5, 4
  br i1 %6, label %7, label %16

; <label>:7                                       ; preds = %4
  %8 = load i32* %i
  %9 = load [4 x i32]** %p
  %10 = sext i32 %8 to i64
  %11 = getelementptr [4 x i32]* %9, i32 0, i64 %10
  %12 = load i32* %1
  store i32 %12, i32* %11
  br label %13

; <label>:13                                      ; preds = %7
  %14 = load i32* %i
  %15 = add i32 %14, 1
  store i32 %15, i32* %i
  br label %4

; <label>:16                                      ; preds = %4
  %17 = load [4 x i32]** %p
  ret [4 x i32]* %17
}

declare i8* @malloc(i64)

define i32 @main() {
  %1 = call [4 x i32]* @newArr(i32 5)
  %x = alloca [4 x i32]*
  store [4 x i32]* %1, [4 x i32]** %x
  %2 = load [4 x i32]** %x
  %3 = sext i32 4 to i64
  %4 = getelementptr [4 x i32]* %2, i32 0, i64 %3
  %5 = load i32* %4
  %tmp = alloca i32
  store i32 %5, i32* %tmp
  %6 = load [4 x i32]** %x
  %7 = bitcast [4 x i32]* %6 to i8*
  call void @free(i8* %7)
  %8 = load i32* %tmp
  ret i32 %8
}

declare void @free(i8*)

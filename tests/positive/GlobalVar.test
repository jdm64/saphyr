
struct MyStruct
{
	int a, b;
	[4]double c;
}

int global = 234, g2 = 2;
[7]int gArr;
@int p = null;
@[]int p2 = null;
MyStruct mStr;

void run()
{
	// override global
	bool global = false;
	mStr.c[3] = 2.8;
}

int main()
{
	int a = global;
	bool b = gArr[a];
	return b;
}

========

%MyStruct = type { i32, i32, [4 x double] }

@global = global i32 234
@g2 = global i32 2
@gArr = external global [7 x i32]
@p = global i32* null
@p2 = global [0 x i32]* null
@mStr = external global %MyStruct

define void @run() {
  %global = alloca i1
  store i1 false, i1* %global
  %1 = getelementptr %MyStruct* @mStr, i32 0, i32 2
  %2 = sext i32 3 to i64
  %3 = getelementptr [4 x double]* %1, i32 0, i64 %2
  store double 2.800000e+00, double* %3
  ret void
}

define i32 @main() {
  %1 = load i32* @global
  %a = alloca i32
  store i32 %1, i32* %a
  %2 = load i32* %a
  %3 = sext i32 %2 to i64
  %4 = getelementptr [7 x i32]* @gArr, i32 0, i64 %3
  %5 = load i32* %4
  %b = alloca i1
  %6 = icmp ne i32 %5, 0
  store i1 %6, i1* %b
  %7 = load i1* %b
  %8 = zext i1 %7 to i32
  ret i32 %8
}


void intFunc()
{
	auto a = 9, b = 8_i8, c = 7_i16, d = 6_i32, e = 5_i64;
}

void uintFunc()
{
	auto a = 10_u8, b = 11_u16, c = 12_u32, d = 13_u64;

	c = a / b;
	d = b / c;
	a = c / d;
}

void base()
{
	auto a = 0b10, b = 0o10, c = 0x10;
}

void main()
{
	auto a = 0b1010_i8, b = 0xf0f0_u64, c = 0o7070_u16;

	auto d = a * b, e = b * c;
}

void withSep()
{
	int a = 1'002'003, b = 0xf'4a'13;
}

========

define void @intFunc() {
  %a = alloca i32
  store i32 9, i32* %a
  %b = alloca i8
  store i8 8, i8* %b
  %c = alloca i16
  store i16 7, i16* %c
  %d = alloca i32
  store i32 6, i32* %d
  %e = alloca i64
  store i64 5, i64* %e
  ret void
}

define void @uintFunc() {
  %a = alloca i8
  store i8 10, i8* %a
  %b = alloca i16
  store i16 11, i16* %b
  %c = alloca i32
  store i32 12, i32* %c
  %d = alloca i64
  store i64 13, i64* %d
  %1 = load i8* %a
  %2 = load i16* %b
  %3 = zext i8 %1 to i32
  %4 = zext i16 %2 to i32
  %5 = sdiv i32 %3, %4
  store i32 %5, i32* %c
  %6 = load i16* %b
  %7 = load i32* %c
  %8 = zext i16 %6 to i32
  %9 = udiv i32 %8, %7
  %10 = zext i32 %9 to i64
  store i64 %10, i64* %d
  %11 = load i32* %c
  %12 = load i64* %d
  %13 = zext i32 %11 to i64
  %14 = udiv i64 %13, %12
  %15 = trunc i64 %14 to i8
  store i8 %15, i8* %a
  ret void
}

define void @base() {
  %a = alloca i32
  store i32 2, i32* %a
  %b = alloca i32
  store i32 8, i32* %b
  %c = alloca i32
  store i32 16, i32* %c
  ret void
}

define void @main() {
  %a = alloca i8
  store i8 10, i8* %a
  %b = alloca i64
  store i64 61680, i64* %b
  %c = alloca i16
  store i16 3640, i16* %c
  %1 = load i8* %a
  %2 = load i64* %b
  %3 = sext i8 %1 to i64
  %4 = mul i64 %3, %2
  %d = alloca i64
  store i64 %4, i64* %d
  %5 = load i64* %b
  %6 = load i16* %c
  %7 = zext i16 %6 to i64
  %8 = mul i64 %5, %7
  %e = alloca i64
  store i64 %8, i64* %e
  ret void
}

define void @withSep() {
  %a = alloca i32
  store i32 1002003, i32* %a
  %b = alloca i32
  store i32 1002003, i32* %b
  ret void
}

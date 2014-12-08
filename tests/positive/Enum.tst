
enum Colors
{
	RED, ORANGE = 5, YELLOW, GREEN = 2,
	BLUE = 7, PURPLE
}

enum Dir { N, S, E, W }

int func(Colors c)
{
	return c + Dir.S;
}

int func2()
{
	auto a = Colors.BLUE, b = Dir.S;

	auto c = a + b;

	return c;
}

int main()
{
	Colors a, b = Colors.BLUE;

	a = b.YELLOW;

	auto i = func(b);

	return i;
}

========

define i32 @func(i32 %c) {
  %1 = alloca i32
  store i32 %c, i32* %1
  %2 = load i32* %1
  %3 = add i32 %2, 1
  ret i32 %3
}

define i32 @func2() {
  %a = alloca i32
  store i32 7, i32* %a
  %b = alloca i32
  store i32 1, i32* %b
  %1 = load i32* %a
  %2 = load i32* %b
  %3 = add i32 %1, %2
  %c = alloca i32
  store i32 %3, i32* %c
  %4 = load i32* %c
  ret i32 %4
}

define i32 @main() {
  %a = alloca i32
  %b = alloca i32
  store i32 7, i32* %b
  store i32 6, i32* %a
  %1 = load i32* %b
  %2 = call i32 @func(i32 %1)
  %i = alloca i32
  store i32 %2, i32* %i
  %3 = load i32* %i
  ret i32 %3
}

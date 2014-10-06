
int func(int a)
{
	return a + 1;
}

int main()
{
	auto ptr = func;

	ptr(4);

	auto ptr2 = func$$;

	ptr2(4);

	return 0;
}

========

define i32 @func(i32 %a) {
  %1 = alloca i32
  store i32 %a, i32* %1
  %2 = load i32* %1
  %3 = add i32 %2, 1
  ret i32 %3
}

define i32 @main() {
  %ptr = alloca i32 (i32)*
  store i32 (i32)* @func, i32 (i32)** %ptr
  %1 = load i32 (i32)** %ptr
  %2 = call i32 %1(i32 4)
  %ptr2 = alloca i32 (i32)*
  store i32 (i32)* @func, i32 (i32)** %ptr2
  %3 = load i32 (i32)** %ptr2
  %4 = call i32 %3(i32 4)
  ret i32 0
}

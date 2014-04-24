
int main()
{
	auto a = false, b = true;

	return a + b;
}

========

define i32 @main() {
  %a = alloca i1
  store i1 false, i1* %a
  %b = alloca i1
  store i1 true, i1* %b
  %1 = load i1* %a
  %2 = load i1* %b
  %3 = add i1 %1, %2
  %4 = zext i1 %3 to i32
  ret i32 %4
}

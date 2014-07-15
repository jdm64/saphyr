
void test()
{
	int a = 5;
	@int ptr = a$;

	ptr@ = 7 + ptr@;
}

void inc(@int p)
{
	p@++;
	++p@;
}

int other()
{
	int a = 0;
	inc(a$);

	auto p1 = a$;
	@int p2 = p1;

	return p1@ + p2@;
}

========

define void @test() {
  %a = alloca i32
  store i32 5, i32* %a
  %ptr = alloca i32*
  store i32* %a, i32** %ptr
  %1 = load i32** %ptr
  %2 = load i32** %ptr
  %3 = load i32* %2
  %4 = add i32 7, %3
  store i32 %4, i32* %1
  ret void
}

define void @inc(i32* %p) {
  %1 = alloca i32*
  store i32* %p, i32** %1
  %2 = load i32** %1
  %3 = load i32* %2
  %4 = add i32 %3, 1
  store i32 %4, i32* %2
  %5 = load i32** %1
  %6 = load i32* %5
  %7 = add i32 %6, 1
  store i32 %7, i32* %5
  ret void
}

define i32 @other() {
  %a = alloca i32
  store i32 0, i32* %a
  call void @inc(i32* %a)
  %p1 = alloca i32*
  store i32* %a, i32** %p1
  %1 = load i32** %p1
  %p2 = alloca i32*
  store i32* %1, i32** %p2
  %2 = load i32** %p1
  %3 = load i32* %2
  %4 = load i32** %p2
  %5 = load i32* %4
  %6 = add i32 %3, %5
  ret i32 %6
}

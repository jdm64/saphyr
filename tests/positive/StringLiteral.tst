
auto gA = "global string";

int main()
{
	auto str1 = "double quote";
	auto str2 = `back tick`;
	auto str3 = "žščř";

	return 0;
}

========

@0 = private constant [14 x i8] c"global string\00"
@gA = global [14 x i8]* @0
@1 = private constant [13 x i8] c"double quote\00"
@2 = private constant [10 x i8] c"back tick\00"
@3 = private constant [9 x i8] c"\C5\BE\C5\A1\C4\8D\C5\99\00"

define i32 @main() {
  %str1 = alloca [13 x i8]*
  store [13 x i8]* @1, [13 x i8]** %str1
  %str2 = alloca [10 x i8]*
  store [10 x i8]* @2, [10 x i8]** %str2
  %str3 = alloca [9 x i8]*
  store [9 x i8]* @3, [9 x i8]** %str3
  ret i32 0
}

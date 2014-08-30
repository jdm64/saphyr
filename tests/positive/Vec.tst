
vec<3,int> retVecWithNum()
{
	int a = 5, b = 8;
	return a + b;
}

void numToVec()
{
	vec<3,int> z;
	vec<3,bool> b;

	z += 3;
	b = 4;
}

void vecCasting()
{
	vec<3,int8> a = 3;
	vec<3,int16> b;
	vec<3,bool> c;

	c = a + b;
}

int vecEquality()
{
	vec<3,uint> a, b;

	auto c = a == b;

	return c[2];
}

void vecOps()
{
	vec<3,int> a;

	auto c = -a;
	c = ~a;
	c++;
	c--;
}

========

define <3 x i32> @retVecWithNum() {
  %a = alloca i32
  store i32 5, i32* %a
  %b = alloca i32
  store i32 8, i32* %b
  %1 = load i32* %a
  %2 = load i32* %b
  %3 = add i32 %1, %2
  %4 = insertelement <1 x i32> undef, i32 %3, i32 0
  %5 = shufflevector <1 x i32> %4, <1 x i32> undef, <3 x i32> zeroinitializer
  ret <3 x i32> %5
}

define void @numToVec() {
  %z = alloca <3 x i32>
  %b = alloca <3 x i1>
  %1 = load <3 x i32>* %z
  %2 = insertelement <1 x i32> undef, i32 3, i32 0
  %3 = shufflevector <1 x i32> %2, <1 x i32> undef, <3 x i32> zeroinitializer
  %4 = add <3 x i32> %1, %3
  store <3 x i32> %4, <3 x i32>* %z
  %5 = icmp ne i32 4, 0
  %6 = insertelement <1 x i1> undef, i1 %5, i32 0
  %7 = shufflevector <1 x i1> %6, <1 x i1> undef, <3 x i32> zeroinitializer
  store <3 x i1> %7, <3 x i1>* %b
  ret void
}

define void @vecCasting() {
  %a = alloca <3 x i8>
  %1 = trunc i32 3 to i8
  %2 = insertelement <1 x i8> undef, i8 %1, i32 0
  %3 = shufflevector <1 x i8> %2, <1 x i8> undef, <3 x i32> zeroinitializer
  store <3 x i8> %3, <3 x i8>* %a
  %b = alloca <3 x i16>
  %c = alloca <3 x i1>
  %4 = load <3 x i8>* %a
  %5 = load <3 x i16>* %b
  %6 = sext <3 x i8> %4 to <3 x i16>
  %7 = add <3 x i16> %6, %5
  %8 = icmp ne <3 x i16> %7, zeroinitializer
  store <3 x i1> %8, <3 x i1>* %c
  ret void
}

define i32 @vecEquality() {
  %a = alloca <3 x i32>
  %b = alloca <3 x i32>
  %1 = load <3 x i32>* %a
  %2 = load <3 x i32>* %b
  %3 = icmp eq <3 x i32> %1, %2
  %c = alloca <3 x i1>
  store <3 x i1> %3, <3 x i1>* %c
  %4 = sext i32 2 to i64
  %5 = getelementptr <3 x i1>* %c, i32 0, i64 %4
  %6 = load i1* %5
  %7 = zext i1 %6 to i32
  ret i32 %7
}

define void @vecOps() {
  %a = alloca <3 x i32>
  %1 = load <3 x i32>* %a
  %2 = sub <3 x i32> zeroinitializer, %1
  %c = alloca <3 x i32>
  store <3 x i32> %2, <3 x i32>* %c
  %3 = load <3 x i32>* %a
  %4 = xor <3 x i32> <i32 -1, i32 -1, i32 -1>, %3
  store <3 x i32> %4, <3 x i32>* %c
  %5 = load <3 x i32>* %c
  %6 = add <3 x i32> %5, <i32 1, i32 1, i32 1>
  store <3 x i32> %6, <3 x i32>* %c
  %7 = load <3 x i32>* %c
  %8 = add <3 x i32> %7, <i32 -1, i32 -1, i32 -1>
  store <3 x i32> %8, <3 x i32>* %c
  ret void
}

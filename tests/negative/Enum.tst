
enum Color {Red, Green, Blue}

enum Dir {N, S, E, W}

enum Broken { A = 3 + Dir.N, B = 3.4, A = 2 }

int main()
{
	auto a = Color.Red, b = Dir.S;

	auto c = a + b;

	auto d = c.N;

	Dir ee = Color.Red;

	return Color.NotFound;
}

========

error: enum initializer must be a constant
error: enum initializer must be an int-like constant
error: enum member name A already declared
error: c is not a struct/union/enum
error: can't cast to enum type
error: Color doesn't have member NotFound
found 6 errors


int f = 0;

int f()
{
	return 0;
}

void g()
{
	return;
}

double g = 0.0;

int main()
{
	f();
	g = 0;

	return 0;
}

========

error: variable f already defined
error: variable g already defined
error: function f not defined
error: can not cast complex types
found 4 errors

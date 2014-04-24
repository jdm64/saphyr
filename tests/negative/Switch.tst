
int func()
{
	int x = 4;
	switch (x) {
	case 1:
		x = 2;
		break;
	default:
		x = 0;
		break;
	case 4:
		x = 8;
		break;
	default:
		x = 42;
	case 1:
		x = 3;
		break;
	}
	return x;
}

========

error: switch statement has more than one default
error: switch case values are not unique
found 2 errors

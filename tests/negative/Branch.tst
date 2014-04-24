
void func()
{
	int x = 9;
	if (x == 4) {
		break;
	} else {
		continue;
	}
}

void run()
{
	int x = 2;
lb:
	goto notfound;
lb:
	int z = 4;
}

========

error: break invalid outside a loop/switch block
error: continue invalid outside a loop/switch block
error: label lb already defined
error: label notfound not defined
found 4 errors

#include <stdio.h>
int main(int argc, char *argv[]) {
	unsigned int x = 4;
	int sx = (int)(x);

	unsigned int bigx = 0xfffffefe;
	short kindabig = (short)bigx;
	printf("%hi\n", kindabig);

        short short2 = -1;
	int wasashort = (int)short2;
	printf("%i\n", wasashort);

	int foo = -1;
	unsigned int bar = (unsigned int)foo;
	printf("bar: %u\n", bar);

	float f = 3.14;
	int wasafloat = (int)f;
	printf("wasafloat: %i\n", wasafloat);

	return 0;
}

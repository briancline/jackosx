#include <stdio.h>

extern void *vst_main(void);

void *vst_main(void)
{
	printf("Hello world, this is vst_main() in Mach-O object\n");
	return 0;
}

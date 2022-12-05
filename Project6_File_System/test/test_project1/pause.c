#include <bios.h>

int main(void)
{
	// Check the data array
	bios_putstr("Press any key to continue!\n");
	while(bios_getchar()==-1);

	return 0;
}
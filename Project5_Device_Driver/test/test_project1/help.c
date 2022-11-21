#include <bios.h>

int main(void)
{
	bios_putstr("Type app name to launch an application\n\r"
                "or bat name (without \".txt\") to launch a batch file\n\r");
	return 0;
}
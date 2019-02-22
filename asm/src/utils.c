#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

void
panic(const char *reason)
{
	printf("panic: %s\n", reason);
	exit(1);
}

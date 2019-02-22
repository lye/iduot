#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "program.h"

int
main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s <file>\n", argv[0]);
		return 1;
	}

	const char *fpath = argv[1];

	program_t prog;
	program_init(&prog);
	int err = program_parse_file(&prog, fpath);
	if (err != 0) {
		printf("Error: %s\n", strerror(err));
		return 1;
	}

	program_free(&prog);
	return 0;

	return 0;
}

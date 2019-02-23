#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "program.h"

int
main(int argc, char **argv)
{
	if (argc != 3) {
		printf("Usage: %s <infile> <outfile>\n", argv[0]);
		return 1;
	}

	const char *inpath = argv[1];
	const char *outpath = argv[2];

	program_t prog;
	program_init(&prog);
	int err = program_parse_file(&prog, inpath);
	if (err != 0) {
		printf("Error: %s\n", strerror(err));
		return 1;
	}

	size_t outlen = 0;
	err = program_compile(&prog, NULL, &outlen);
	if (0 != err) {
		printf("Error: %s\n", strerror(err));
		return 1;
	}

	char *bs = malloc(outlen);
	err = program_compile(&prog, bs, &outlen);
	if (0 != err) {
		free(bs);
		printf("Error: %s\n", strerror(err));
		return 1;
	}

	int fdout = open(outpath, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (0 > fdout) {
		free(bs);
		printf("Error: %s\n", strerror(errno));
		return 1;
	}

	size_t written = 0;
	while (written < outlen) {
		ssize_t w = write(fdout, bs + written, outlen - written);
		if (w <= 0) {
			free(bs);
			unlink(outpath);
			close(fdout);
			return 1;
		}

		written += w;
	}

	free(bs);
	close(fdout);
	program_free(&prog);

	return 0;
}

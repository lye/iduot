#pragma once
#include "inst.h"

// program_t represents an unencoded program read in from a file. Right
// now it's a little basic, and just consists of a list of instructions.
typedef struct {
	size_t  insts_len;
	size_t  insts_cap;
	inst_t *insts;
}
program_t;

// program_init initializes a program for use.
void program_init(program_t *this);

// program_free frees the specified program.
void program_free(program_t *this);

// program_push_inst adds an instruction to the end of the program.
void program_push_inst(program_t *this, inst_t inst);

// program_parse_file parses a program from the specified file. It returns
// 0 if no error, errno otherwise.
int program_parse_file(program_t *this, const char *path);

// program_parse_bytes parses a program from the specified buffer. It returns
// 0 if no error, errno otherwise.
int program_parse_bytes(program_t *this, const void *bs, size_t len);

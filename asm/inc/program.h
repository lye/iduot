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

// program_compile compiles the program to the provided buffer. The size of 
// the program is written to buf_len. If buf is NULL, the size is still
// written.
//
// Since iduot uses 12-bit words and uses 1 word/instruction (and the host
// arch likely does not), the buffer may be padded with zeros. buf_len is 
// specified in padded host bytes (for ease-of-use).
//
// 0 is returned on success.
int program_compile(const program_t *this, void *buf, size_t *buf_len);

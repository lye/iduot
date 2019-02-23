#pragma once
#include "inst.h"

typedef struct {
	ssize_t off;
	size_t  uses;
	char   *label;
}
label_t;

// program_t represents an unencoded program read in from a file. Right
// now it's a little basic, and just consists of a list of instructions.
typedef struct {
	size_t   labels_len;
	size_t   labels_cap;
	label_t *labels;

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

// program_label_ref stores the label name in the program and notes its
// use. The use count isn't used for anything right now. The returned pointer
// is owned by the program and can be used until the program is freed.
const char* program_label_ref(program_t *this, const char *name);

// program_label_end sets a label on the next instruction written. A label
// is an arbitrary currently unique string that resolves to the position
// of the instruction at compile time. It can't be referenced by an inst_t;
// it's used for synthetic instructions (since the immediate needs to be
// loaded before jce/mv PC). It returns 0 on success.
int program_label_end(program_t *this, const char *name);

// program_label_find returns the index in 12-bit words for the corresponding
// label, or <0 if there is no such label.
ssize_t program_label_find(const program_t *this, const char *name);

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

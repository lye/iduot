#pragma once
#include "inst.h"

typedef struct {
	size_t  insts_len;
	size_t  insts_cap;
	inst_t *insts;
}
program_t;

void program_init(program_t *this);
void program_free(program_t *this);

void program_push_inst(program_t *this, inst_t inst);

int program_parse_file(program_t *this, const char *path);
int program_parse_bytes(program_t *this, const void *bs, size_t len);

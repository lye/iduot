#include <sys/types.h>
#include <stdlib.h>
#include <strings.h>
#include "program.h"
#include "utils.h"

void
program_init(program_t *this)
{
	bzero(this, sizeof(*this));
}

void
program_free(program_t *this)
{
	free(this->insts);
}

void
program_push_inst(program_t *this, inst_t inst)
{
	if (this->insts_len == this->insts_cap) {
		this->insts_cap = this->insts_cap ? this->insts_cap * 2 : 8;
		this->insts = realloc(
			this->insts,
			sizeof(inst_t) * this->insts_cap
		);
		if (NULL == this->insts) {
			panic("realloc failure");
		}
	}

	this->insts[this->insts_len] = inst;
	this->insts_len += 1;
}

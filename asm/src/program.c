#include <sys/types.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <errno.h>
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

int
program_compile(const program_t *this, void *buf, size_t *buf_len)
{
	if (NULL == buf_len) {
		// XXX: stop using posix error codes so we can be more specific
		// about what the error is.
		return ENOMEM;
	}

	// Instructions are 12-bits each, which means we can fit 2 into
	// every 3 8 uint8_t's.
	size_t len = (this->insts_len * 3 + 1) / 2;
	if (NULL == buf) {
		*buf_len = len;
		return 0;
	}
	else if (len > *buf_len) {
		printf("len = %lu, buf_len = %lu\n", len, *buf_len);
		return ENOMEM;
	}

	*buf_len = len;

	// Host:  0     1     2     3 
	// Insts: |---1---| |---2---|
	//
	// Even: inst & 0xff0 >> 4, inst & 0xf << 4
	// Odd: inst & 0xf00 >> 8, inst & 0xff

	uint8_t *bs = buf;

	for (size_t i = 0; i < this->insts_len; i += 1) {
		inst_enc_t inst = inst_encode(this->insts[i]);

		if (i % 2 == 0) {
			*bs++ = (inst & 0xff0) >> 4;
			*bs = (inst & 0xf) << 4;
		}
		else {
			*bs++ |= (inst & 0xf00) >> 8;
			*bs++ = inst & 0xff;
		}
	}

	return 0;
}

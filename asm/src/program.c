#include <sys/types.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
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

	for (size_t i = 0; i < this->labels_len; i += 1) {
		free(this->labels[i].label);
	}

	free(this->labels);
}

void
program_push_inst(program_t *this, inst_t inst)
{
	// NB: This is a gross special-case. If we're inserting an INST_LOAD_IMM
	// then put two instructions in -- one for the instruction, one for the
	// immediate.
	if (
		INST_LOAD_IMM == inst.type
		&& REG_IDS != inst.load_imm.reg
		&& 0 != inst.load_imm.imm
	) {
		inst_t is[] = {
			{
				.type = INST_LOAD_IMM,
				.load_imm = {
					.reg = inst.load_imm.reg,
				},
			},
			{
				.type = INST_LOAD_IMM,
				.load_imm = {
					.reg = REG_IDS,
					.imm = inst.load_imm.imm,
				},
			},
		};

		program_push_inst(this, is[0]);
		program_push_inst(this, is[1]);
		return;
	}

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

int
program_label_end(program_t *this, const char *name)
{
	if (0 <= program_label_find(this, name)) {
		// XXX: error codes
		return 1;
	}

	if (this->labels_len == this->labels_cap) {
		this->labels_cap = this->labels_cap ? this->labels_cap * 2 : 8;
		this->labels = realloc(
			this->labels,
			this->labels_cap * sizeof(label_t)
		);
		if (NULL == this->labels) {
			return ENOMEM;
		}
	}

	label_t *label = &this->labels[this->labels_len++];
	label->off = this->insts_len;
	label->label = strdup(name);
	return 0;
}

ssize_t
program_label_find(const program_t *this, const char *name)
{
	for (size_t i = 0; i < this->labels_len; i += 1) {
		label_t *label = &this->labels[i];
		if (0 == strcmp(name, label->label)) {
			return label->off;
		}
	}

	return -1;
}

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
		&& (
			0 != inst.load_imm.imm
			|| NULL != inst.load_imm.label
		)
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
					.label = inst.load_imm.label,
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

// NB: This is not exposed because the pointers returned are not stable.
static label_t*
program_label_find_internal(const program_t *this, const char *name)
{
	for (size_t i = 0; i < this->labels_len; i += 1) {
		label_t *label = &this->labels[i];
		if (0 == strcmp(name, label->label)) {
			return label;
		}
	}

	return NULL;
}

label_t*
program_label_insert_internal(program_t *this, const char *name)
{
	if (this->labels_len == this->labels_cap) {
		this->labels_cap = this->labels_cap ? this->labels_cap * 2 : 8;
		this->labels = realloc(
			this->labels,
			this->labels_cap * sizeof(label_t)
		);
		if (NULL == this->labels) {
			// XXX: Realllly want to panic here.
			return NULL;
		}
	}

	label_t *label = &this->labels[this->labels_len++];
	bzero(label, sizeof(*label));
	label->label = strdup(name);
	return label;
}

const char*
program_label_ref(program_t *this, const char *name)
{
	label_t *label = program_label_find_internal(this, name);
	if (NULL == label) {
		label = program_label_insert_internal(this, name);
		if (NULL == label) {
			return NULL;
		}
	}

	label->uses += 1;
	return label->label;
}

int
program_label_end(program_t *this, const char *name)
{
	if (0 <= program_label_find(this, name)) {
		// XXX: error codes
		return 1;
	}

	label_t *label = program_label_insert_internal(this, name);
	if (NULL == label) {
		return ENOMEM;
	}

	label->off = this->insts_len;
	return 0;
}

ssize_t
program_label_find(const program_t *this, const char *name)
{
	label_t *label = program_label_find_internal(this, name);
	if (NULL == label) {
		return -1;
	}

	// NB: referenced but undefined labels have off=-1, so this is fine.
	return label->off;
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
		// XXX: Might want to resolve labels somewhere else; this is
		// kind of gross to do here.
		if (
			INST_LOAD_IMM == this->insts[i].type
			&& REG_IDS == this->insts[i].load_imm.reg
			&& NULL != this->insts[i].load_imm.label
		) {
			label_t *label = program_label_find_internal(
				this,
				this->insts[i].load_imm.label
			);
			if (NULL == label) {
				return ENOENT;
			}

			this->insts[i].load_imm.imm = label->off;
		}

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

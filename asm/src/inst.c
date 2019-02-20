#include "inst.h"
#include "utils.h"
#include <assert.h>

inst_format_t
inst_type_format(inst_type_t type)
{
	static const struct {
		inst_type_t type;
		inst_format_t format;
	}
	table[] = {
		{ INST_LOAD_IMM,    INST_FORMAT_LOAD_IMM },
		{ INST_LOAD_MEM,    INST_FORMAT_REG2     },
		{ INST_STORE_MEM,   INST_FORMAT_REG2     },
		{ INST_LOAD_STACK,  INST_FORMAT_REG2     },
		{ INST_STORE_STACK, INST_FORMAT_REG2     },
		{ INST_ADD,         INST_FORMAT_REG2     },
		{ INST_SUB,         INST_FORMAT_REG2     },
		{ INST_MUL,         INST_FORMAT_REG2     },
		{ INST_DIV,         INST_FORMAT_REG2     },
		{ INST_XOR,         INST_FORMAT_REG2     },
		{ INST_AND,         INST_FORMAT_REG2     },
		{ INST_NOT,         INST_FORMAT_REG2     },
		{ INST_CMP,         INST_FORMAT_REG2     },
		{ INST_JE,          INST_FORMAT_REG2     },
		{ INST_WAIT,        INST_FORMAT_REG1     },
		{ INST_FORK,        INST_FORMAT_REG1     },
		{ INST_SIGNAL,      INST_FORMAT_SIGNAL   },
		{ INST_ALLOC,       INST_FORMAT_REG1     },
		{ INST_FREE,        INST_FORMAT_REG1     },
		{ INST_HALT,        INST_FORMAT_REG1     },
	};
	STATIC_ASSERT(
		ARRSIZE(table) == INSTS,
		"missing instance format entries"
	);

	for (size_t i = 0; i < ARRSIZE(table); i += 1) {
		if (table[i].type == type) {
			return table[i].format;
		}
	}

	UNREACHABLE;
}

void
inst_type_encode(inst_type_t type, inst_enc_t *val)
{
	static const struct {
		inst_type_t type;
		uint16_t length;  // length of opcode, in bits
		uint16_t opcode;
	}
	table[] = {
		{ INST_LOAD_IMM,    4, 0b0000    },
		{ INST_LOAD_MEM,    4, 0b0001    },
		{ INST_STORE_MEM,   4, 0b0010    },
		{ INST_LOAD_STACK,  4, 0b0011    },
		{ INST_STORE_STACK, 4, 0b0100    },
		{ INST_ADD,         4, 0b0101    },
		{ INST_SUB,         4, 0b0110    },
		{ INST_MUL,         4, 0b0111    },
		{ INST_DIV,         4, 0b1000    },
		{ INST_XOR,         4, 0b1001    },
		{ INST_AND,         4, 0b1010    },
		{ INST_NOT,         4, 0b1011    },
		{ INST_CMP,         4, 0b1100    },
		{ INST_JE,          4, 0b1101    },
		{ INST_WAIT,        5, 0b11100   },
		{ INST_FORK,        5, 0b11101   },
		{ INST_SIGNAL,      5, 0b11110   },
		{ INST_ALLOC,       7, 0b1111100 },
		{ INST_FREE,        7, 0b1111101 },
		{ INST_HALT,        7, 0b1111111 },
	};
	STATIC_ASSERT(
		ARRSIZE(table) == INSTS,
		"missing instruction opcode entries"
	);

	for (size_t i = 0; i < ARRSIZE(table); i += 1) {
		if (table[i].type == type) {
			*val |= table[i].opcode << (IDUOT_WORD_BITS - table[i].length);
			return;
		}
	}

	UNREACHABLE;
}

uint16_t
reg_id_encode(reg_id_t reg)
{
	static const struct {
		reg_id_t reg;
		uint16_t val;
	}
	table[] = {
		{ REG_ID_ZERO,       0x0 },
		{ REG_ID_PC,         0x1 },
		{ REG_ID_SP,         0x2 },
		{ REG_ID_TSEG,       0x3 },
		{ REG_ID_MSEG,       0x4 },
		{ REG_ID_CARRY,      0x5 },
		{ REG_ID_A,          0x6 },
		{ REG_ID_B,          0x7 },
		{ REG_ID_C,          0x8 },
		{ REG_ID_D,          0x9 },
		{ REG_ID_E,          0xA },
		{ REG_ID_F,          0xB },
		{ REG_ID_G,          0xC },
		{ REG_ID_H,          0xD },
		{ REG_ID_RESERVED_0, 0xE },
		{ REG_ID_RESERVED_1, 0xF },
	};
	STATIC_ASSERT(
		ARRSIZE(table) == REG_IDS,
		"missing register encoding"
	);

	for (size_t i = 0; i < ARRSIZE(table); i += 1) {
		if (table[i].reg == reg) {
			return table[i].val;
		}
	}
	
	UNREACHABLE;
}

void
inst_reg1_encode(inst_reg1_t op, inst_enc_t *enc)
{
	*enc |= reg_id_encode(op.reg);
}

void
inst_reg2_encode(inst_reg2_t op, inst_enc_t *enc)
{
	*enc |= reg_id_encode(op.reg1) << IDUOT_REG_BITS;
	*enc |= reg_id_encode(op.reg2);
}

void
inst_load_imm_encode(inst_load_imm_t op, inst_enc_t *enc)
{
	assert(op.imm <= IDUOT_IMM_MAX);
	assert(op.imm != 0);

	*enc |= reg_id_encode(op.reg) << IDUOT_IMM_BITS;
	*enc |= op.imm;
}

void
inst_signal_encode(inst_signal_t op, inst_enc_t *enc)
{
	assert(op.signal <= IDUOT_SIG_MAX);

	*enc |= reg_id_encode(op.reg) << IDUOT_SIG_BITS;
	*enc |= op.signal;
}

inst_enc_t
inst_encode(inst_t inst)
{
	inst_enc_t enc = 0;

	inst_type_encode(inst.type, &enc);

	switch (inst_type_format(inst.type)) {
		case INST_FORMAT_REG1:
			inst_reg1_encode(inst.reg1, &enc);
			break;

		case INST_FORMAT_REG2:
			inst_reg2_encode(inst.reg2, &enc);
			break;

		case INST_FORMAT_LOAD_IMM:
			inst_load_imm_encode(inst.load_imm, &enc);
			break;

		case INST_FORMAT_SIGNAL:
			inst_signal_encode(inst.signal, &enc);
			break;

		case INST_FORMATS:
			UNREACHABLE;
	}

	return enc;
}

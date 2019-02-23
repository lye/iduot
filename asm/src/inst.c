#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include "inst.h"
#include "utils.h"

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
		{ INST_LOAD_STK,    INST_FORMAT_REG2     },
		{ INST_STORE_STK,   INST_FORMAT_REG2     },
		{ INST_ADD,         INST_FORMAT_REG2     },
		{ INST_SUB,         INST_FORMAT_REG2     },
		{ INST_MUL,         INST_FORMAT_REG2     },
		{ INST_DIV,         INST_FORMAT_REG2     },
		{ INST_NAND,        INST_FORMAT_REG2     },
		{ INST_CMP,         INST_FORMAT_REG2     },
		{ INST_JCE,         INST_FORMAT_REG2     },
		{ INST_MV,          INST_FORMAT_REG2     },
		{ INST_WAIT,        INST_FORMAT_REG2     },
		{ INST_SIGNAL,      INST_FORMAT_REG2     },

		{ INST_ALLOC,       INST_FORMAT_REG1     },
		{ INST_FREE,        INST_FORMAT_REG1     },
		{ INST_FORK,        INST_FORMAT_REG1     },
		{ INST_WAITFOR,     INST_FORMAT_REG1     },
		{ INST_GETISEG,     INST_FORMAT_REG1     },
		{ INST_SETISEG,     INST_FORMAT_REG1     },
		{ INST_TASKID,      INST_FORMAT_REG1     },

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

inst_type_t
inst_type_parse(const char *str)
{
	static const struct {
		inst_type_t type;
		const char *str;
	}
	table[] = {
		{ INST_LOAD_IMM,  "loadimm"  },
		{ INST_LOAD_MEM,  "loadmem"  },
		{ INST_STORE_MEM, "storemem" },
		{ INST_LOAD_STK,  "loadstk"  },
		{ INST_STORE_STK, "storestk" },
		{ INST_ADD,       "add"      },
		{ INST_SUB,       "sub"      },
		{ INST_MUL,       "mul"      },
		{ INST_DIV,       "div"      },
		{ INST_NAND,      "nand"     },
		{ INST_CMP,       "cmp"      },
		{ INST_JCE,       "jce"      },
		{ INST_MV,        "mv"       },
		{ INST_SIGNAL,    "signal"   },
		{ INST_WAIT,      "wait"     },
		{ INST_ALLOC,     "alloc"    },
		{ INST_FREE,      "free"     },
		{ INST_FORK,      "fork"     },
		{ INST_WAITFOR,   "waitfor"  },
		{ INST_GETISEG,   "getiseg"  },
		{ INST_SETISEG,   "setiseg"  },
		{ INST_TASKID,    "taskid"   },
		{ INST_HALT,      "halt"     },
	};

	for (size_t i = 0; i < ARRSIZE(table); i += 1) {
		if (0 == strcmp(table[i].str, str)) {
			return table[i].type;
		}
	}

	return INSTS;
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
		{ INST_MV,          4, 0b0000     },
		{ INST_LOAD_MEM,    4, 0b0001     },
		{ INST_STORE_MEM,   4, 0b0010     },
		{ INST_LOAD_STK,    4, 0b0011     },
		{ INST_STORE_STK,   4, 0b0100     },
		{ INST_ADD,         4, 0b0101     },
		{ INST_SUB,         4, 0b0110     },
		{ INST_MUL,         4, 0b0111     },
		{ INST_DIV,         4, 0b1000     },
		{ INST_NAND,        4, 0b1001     },
		{ INST_CMP,         4, 0b1010     },
		{ INST_JCE,         4, 0b1011     },
		// 0b1010 unused
		{ INST_SIGNAL,      4, 0b1101     },
		{ INST_WAIT,        4, 0b1110     },
		{ INST_ALLOC,       8, 0b11110000 },
		{ INST_FREE,        8, 0b11110001 },
		{ INST_FORK,        8, 0b11110010 },
		{ INST_WAITFOR,     8, 0b11110011 },
		{ INST_GETISEG,     8, 0b11110100 },
		{ INST_SETISEG,     8, 0b11110101 },
		{ INST_TASKID,      8, 0b11110110 },
		{ INST_LOAD_IMM,    8, 0b11110111 },
		// 0b11111000-0b11111110 are unused
		{ INST_HALT,        8, 0b11111111 },
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

reg_id_t
reg_id_parse(const char *str)
{
	static const struct {
		reg_id_t    reg;
		const char *str;
	}
	table[] = {
		{ REG_ID_PC,    "PC"    },
		{ REG_ID_SP,    "SP"    },
		{ REG_ID_RMEM,  "RMEM"  },
		{ REG_ID_WMEM,  "WMEM"  },
		{ REG_ID_CARRY, "CARRY" },
		{ REG_ID_ZERO,  "ZERO"  },
		{ REG_ID_A,     "A"     },
		{ REG_ID_B,     "B"     },
		{ REG_ID_C,     "C"     },
		{ REG_ID_D,     "D"     },
		{ REG_ID_E,     "E"     },
		{ REG_ID_F,     "F"     },
		{ REG_ID_G,     "G"     },
		{ REG_ID_H,     "H"     },
		{ REG_ID_I,     "I"     },
		{ REG_ID_J,     "J"     },
	};

	for (size_t i = 0; i < ARRSIZE(table); i += 1) {
		if (0 == strcmp(table[i].str, str)) {
			return table[i].reg;
		}
	}

	return REG_IDS;
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
		{ REG_ID_RMEM,       0x3 },
		{ REG_ID_WMEM,       0x4 },
		{ REG_ID_CARRY,      0x5 },
		{ REG_ID_A,          0x6 },
		{ REG_ID_B,          0x7 },
		{ REG_ID_C,          0x8 },
		{ REG_ID_D,          0x9 },
		{ REG_ID_E,          0xA },
		{ REG_ID_F,          0xB },
		{ REG_ID_G,          0xC },
		{ REG_ID_H,          0xD },
		{ REG_ID_I,          0xE },
		{ REG_ID_J,          0xF },
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
	// NB: Special-casing imm=0 (which is disallowed by the spec) to
	// indicate this is the instruction half of the two-word instruction.
	if (op.imm == 0) {
		inst_reg1_t op1 = {
			.reg = op.reg,
		};

		inst_reg1_encode(op1, enc);
	}
	else {
		*enc = op.imm;
	}
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

		case INST_FORMATS:
			UNREACHABLE;
	}

	return enc;
}

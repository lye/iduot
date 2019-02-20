#include <sys/types.h>

// IDUOT_WORD_BITS is the number of bits per word, which is defined as 12 
// because iduot is a 12-bit CPU.
static const uint16_t IDUOT_WORD_BITS = 12;

// IDUOT_REG_BITS is the number of bits used to encode register ids.
static const uint16_t IDUOT_REG_BITS = 4;

// IDUOT_IMM_MAX is the maximum immediate value for load immediate
// instructions.
static const uint16_t IDUOT_IMM_MAX = 0b1111;

// inst_enc_t is an encoded instruction. For convenience, we're encoding to
// uint16_t's and packing them to uint12_t's later on. This makes manipulation
// a lot more straightforward.
typedef uint16_t inst_enc_t;

// inst_format_t is an enumeration which indicates which instruction substruct
// represents the operands for the given instruction.
typedef enum {
	INST_FORMAT_REG1,
	INST_FORMAT_REG2,
	INST_FORMAT_LOAD_IMM,
	INST_FORMAT_SIGNAL,
	INST_FORMATS,
}
inst_format_t;

// inst_type_t is an enumeration which defines a symbol for each iduot
// instruction.
typedef enum {
	INST_LOAD_IMM,
	INST_LOAD_MEM,
	INST_STORE_MEM,
	INST_LOAD_STACK,
	INST_STORE_STACK,
	INST_ADD,
	INST_SUB,
	INST_MUL,
	INST_DIV,
	INST_XOR,
	INST_AND,
	INST_NOT,
	INST_CMP,
	INST_JE,
	INST_WAIT,
	INST_FORK,
	INST_SIGNAL,
	INST_ALLOC,
	INST_FREE,
	INST_HALT,
	INSTS,
}
inst_type_t;

// inst_type_format returns the corresponding operand format for the given
// instruction.
inst_format_t inst_type_format(inst_type_t type);

// inst_type_encode encodes the instruction opcode to the passed-in value.
// The idea is that the value is 0-initialized and inst_type_encode can be
// combined with the corresponding format encode to build the full encoded
// instruction.
void inst_type_encode(inst_type_t type, inst_enc_t *val);

// reg_id_t is an enumeration which defines a symbol for each iduot register.
typedef enum {
	REG_ID_PC,    // program counter
	REG_ID_SP,    // stack pointer
	REG_ID_TSEG,  // task segment
	REG_ID_MSEG,  // active memory segment
	REG_ID_CARRY, // carry
	REG_ID_ZERO,
	REG_ID_A,
	REG_ID_B,
	REG_ID_C,
	REG_ID_D,
	REG_ID_E,
	REG_ID_F,
	REG_ID_G,
	REG_ID_H,
	REG_ID_RESERVED_0,
	REG_ID_RESERVED_1,
	REG_IDS,
}
reg_id_t;

// reg_id_encode returns the 4-bit numeric identifier for the specified
// register id.
uint16_t reg_id_encode(reg_id_t reg);

// inst_reg1_t is used for instructions that only have one register operand,
// which is basically all the concurrency instructions. This corresponds to 
// INST_FORMAT_REG1.
typedef struct {
	reg_id_t reg;
}
inst_reg1_t;

// inst_reg1_encode encodes the operands specified into the given instruction.
void inst_reg1_encode(inst_reg1_t op, inst_enc_t *enc);

// inst_reg2_t is used for most instructions that have two register operands.
// This corresponds to INST_FORMAT_REG2.
typedef struct {
	reg_id_t reg1, reg2;
}
inst_reg2_t;

// inst_reg2_encode encodes the operands specified into the given instruction.
void inst_reg2_encode(inst_reg2_t op, inst_enc_t *enc);

// inst_load_imm_t is used for the INST_LOAD_IMM. The immediate value is
// limited to 4 bits. This corresponds to INST_FORMAT_LOAD_IMM.
typedef struct {
	uint8_t imm;
}
inst_load_imm_t;

// inst_load_imm_encode encodes load immediate operands.
void inst_load_imm_encode(inst_load_imm_t op, inst_enc_t *enc);

// inst_signal_t is used for INST_SIGNAL. The immediate value (which is the
// signal number) is limited to 3 bits. This corresponds to INST_FORMAT_SIGNAL.
typedef struct {
	reg_id_t reg;
	uint8_t imm; 
}
inst_signal_t;

// inst_signal_encode yep it encodes the signal operands.
void inst_signal_encode(inst_signal_t op, inst_enc_t *enc);

// inst_t is a single instruction.
typedef struct {
	inst_type_t type;

	union {
		inst_reg1_t reg1;
		inst_reg2_t reg2;
		inst_load_imm_t load_imm;
		inst_signal_t signal;
	};
}
inst_t;
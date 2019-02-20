#include "test.h"
#include "inst.h"

START_TEST(encode_doc_wait_a_b)
{
	inst_t inst = {
		.type = INST_WAIT,
		.reg2 = {
			.reg1 = REG_ID_A,
			.reg2 = REG_ID_B,
		},
	};

	ck_assert_int_eq(
		inst_encode(inst),
		0b111001100111
	);
}
END_TEST

START_TEST(encode_doc_alloc_b)
{
	inst_t inst = {
		.type = INST_ALLOC,
		.reg1 = {
			.reg = REG_ID_B,
		},
	};

	ck_assert_int_eq(
		inst_encode(inst),
		0b111100000111
	);
}
END_TEST

START_TEST(encode_doc_halt_f)
{
	inst_t inst = {
		.type = INST_HALT,
		.reg1 = {
			.reg = REG_ID_F,
		},
	};

	ck_assert_int_eq(
		inst_encode(inst),
		0b111111111011
	);
}
END_TEST

START_TEST(encode_doc_add_a_b)
{
	inst_t inst = {
		.type = INST_ADD,
		.reg2 = {
			.reg1 = REG_ID_A,
			.reg2 = REG_ID_B,
		},
	};

	ck_assert_int_eq(
		inst_encode(inst),
		0b010101100111
	);
}
END_TEST

START_TEST(encode_doc_loadi_a_4)
{
	inst_t inst = {
		.type = INST_LOAD_IMM,
		.load_imm = {
			.reg = REG_ID_A,
			.imm = 4,
		},
	};

	ck_assert_int_eq(
		inst_encode(inst),
		0b000001100100
	);
}
END_TEST

START_TEST(encode_doc_signal_a_3)
{
	inst_t inst = {
		.type = INST_SIGNAL,
		.reg2 = {
			.reg1 = REG_ID_A,
			.reg2 = REG_ID_B,
		},
	};

	ck_assert_int_eq(
		inst_encode(inst),
		0b110101100111
	);
}
END_TEST

START_TEST(const_imm_max)
{
	ck_assert_int_eq(0b1111, IDUOT_IMM_MAX);
}
END_TEST

Suite*
suite_inst()
{
	tcase_t tests[] = {
		{ "encode_doc_wait_a_b",   &encode_doc_wait_a_b   },
		{ "encode_doc_add_a_b",    &encode_doc_add_a_b    },
		{ "encode_doc_alloc_b",    &encode_doc_alloc_b    },
		{ "encode_doc_halt_f",     &encode_doc_halt_f     },
		{ "encode_doc_loadi_a_4",  &encode_doc_loadi_a_4  },
		{ "encode_doc_signal_a_3", &encode_doc_signal_a_3 },
		{ "const_imm_max",         &const_imm_max         },
	};

	return tcase_build_suite("inst", tests, sizeof(tests));
}

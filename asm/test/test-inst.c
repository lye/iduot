#include "test.h"
#include "inst.h"

START_TEST(encode_doc_wait_a)
{
	inst_t inst = {
		.type = INST_WAIT,
		.reg1 = {
			.reg = REG_ID_A,
		},
	};

	ck_assert_int_eq(
		inst_encode(inst),
		0b111000000110
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
		0b111110000111
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
		.signal = {
			.reg = REG_ID_A,
			.signal = 3,
		},
	};

	ck_assert_int_eq(
		inst_encode(inst),
		0b111100110011
	);
}
END_TEST

START_TEST(const_sig_max)
{
	ck_assert_int_eq(0b111, IDUOT_SIG_MAX);
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
		{ "encode_doc_wait_a",     &encode_doc_wait_a     },
		{ "encode_doc_add_a_b",    &encode_doc_add_a_b    },
		{ "encode_doc_alloc_b",    &encode_doc_alloc_b    },
		{ "encode_doc_loadi_a_4",  &encode_doc_loadi_a_4  },
		{ "encode_doc_signal_a_3", &encode_doc_signal_a_3 },
		{ "const_imm_max",         &const_imm_max         },
		{ "const_sig_max",         &const_sig_max         },
	};

	return tcase_build_suite("inst", tests, sizeof(tests));
}

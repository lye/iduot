#include "test.h"
#include "program.h"

START_TEST(compile_one_inst)
{
	inst_t insts[] = {
		{ 
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_ID_A,
				.imm = 4,
			},
		},
	};
	program_t prog = {
		.insts_len = 1,
		.insts     = insts,
	};
	uint8_t bs[2] = {0xff, 0xff};
	size_t bs_len = sizeof(bs);

	ck_assert_int_eq(0, program_compile(&prog, bs, &bs_len));
	ck_assert_int_eq(2, bs_len);
	// 0000 0110 0100
	ck_assert_int_eq(0b01000000, bs[1]);
	ck_assert_int_eq(0b00000110, bs[0]);
}
END_TEST

START_TEST(compile_two_inst)
{
	inst_t insts[] = {
		{ 
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_ID_A,
				.imm = 4,
			},
		},
		{
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_ID_B,
				.imm = 5,
			},
		}
	};
	program_t prog = {
		.insts_len = 2,
		.insts     = insts,
	};
	uint8_t bs[3] = {0xff, 0xff, 0xff};
	size_t bs_len = sizeof(bs);

	ck_assert_int_eq(0, program_compile(&prog, bs, &bs_len));
	ck_assert_int_eq(3, bs_len);
	// 0000 0110 0100
	// 0000 0111 0101
	ck_assert_int_eq(0b00000110, bs[0]);
	ck_assert_int_eq(0b01000000, bs[1]);
	ck_assert_int_eq(0b01110101, bs[2]);
}
END_TEST

START_TEST(compile_three_inst)
{
	inst_t insts[] = {
		{ 
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_ID_A,
				.imm = 4,
			},
		},
		{
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_ID_B,
				.imm = 5,
			},
		},
		{
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_ID_C,
				.imm = 6
			},
		},
	};
	program_t prog = {
		.insts_len = 3,
		.insts     = insts,
	};
	uint8_t bs[5] = {0xff, 0xff, 0xff, 0xff, 0xff};
	size_t bs_len = sizeof(bs);

	ck_assert_int_eq(0, program_compile(&prog, bs, &bs_len));
	ck_assert_int_eq(5, bs_len);
	// 0000 0110 0100
	// 0000 0111 0101
	// 0000 1000 0110
	ck_assert_int_eq(0b00000110, bs[0]);
	ck_assert_int_eq(0b01000000, bs[1]);
	ck_assert_int_eq(0b01110101, bs[2]);
	ck_assert_int_eq(0b00001000, bs[3]);
	ck_assert_int_eq(0b01100000, bs[4]);
}
END_TEST

Suite*
suite_program()
{
	tcase_t tests[] = {
		{ "compile_one_inst",   &compile_one_inst   },
		{ "compile_two_inst",   &compile_two_inst   },
		{ "compile_three_inst", &compile_three_inst },
	};

	return tcase_build_suite("program", tests, sizeof(tests));
}

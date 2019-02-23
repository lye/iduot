#include "test.h"
#include "program.h"

START_TEST(compile_one_inst)
{
	inst_t insts[] = {
		{ 
			.type = INST_LOAD_MEM,
			.reg2 = {
				.reg1 = REG_ID_A,
				.reg2 = REG_ID_B,
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
	// 0001 0110 0111
	ck_assert_int_eq(0b00010110, bs[0]);
	ck_assert_int_eq(0b01110000, bs[1]);
}
END_TEST

START_TEST(compile_two_inst)
{
	inst_t insts[] = {
		{ 
			.type = INST_LOAD_MEM,
			.reg2 = {
				.reg1 = REG_ID_A,
				.reg2 = REG_ID_B,
			},
		},
		{
			.type = INST_LOAD_MEM,
			.reg2 = {
				.reg1 = REG_ID_C,
				.reg2 = REG_ID_D,
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
	// 0001 0110 0111
	// 0001 1000 1001
	ck_assert_int_eq(0b00010110, bs[0]);
	ck_assert_int_eq(0b01110001, bs[1]);
	ck_assert_int_eq(0b10001001, bs[2]);
}
END_TEST

START_TEST(compile_three_inst)
{
	inst_t insts[] = {
		{ 
			.type = INST_LOAD_MEM,
			.reg2 = {
				.reg1 = REG_ID_A,
				.reg2 = REG_ID_B,
			},
		},
		{
			.type = INST_LOAD_MEM,
			.reg2 = {
				.reg1 = REG_ID_C,
				.reg2 = REG_ID_D,
			},
		},
		{
			.type = INST_LOAD_MEM,
			.reg2 = {
				.reg1 = REG_ID_E,
				.reg2 = REG_ID_F,
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
	// 0001 0110 0111
	// 0001 1000 1001
	// 0001 1010 1011
	ck_assert_int_eq(0b00010110, bs[0]);
	ck_assert_int_eq(0b01110001, bs[1]);
	ck_assert_int_eq(0b10001001, bs[2]);
	ck_assert_int_eq(0b00011010, bs[3]);
	ck_assert_int_eq(0b10110000, bs[4]);
}
END_TEST

START_TEST(push_load_imm)
{
	inst_t inst = {
		.type = INST_LOAD_IMM,
		.load_imm = {
			.reg = REG_ID_A,
			.imm = 4,
		},
	};

	program_t prog;
	program_init(&prog);
	program_push_inst(&prog, inst);

	uint8_t bs[3];
	size_t bs_len = sizeof(bs);

	ck_assert_int_eq(2, prog.insts_len);
	ck_assert_int_eq(0, program_compile(&prog, bs, &bs_len));

	// 1111 0111 0110
	// 0000 0000 0100
	ck_assert_int_eq(0b11110111, bs[0]);
	ck_assert_int_eq(0b01100000, bs[1]);
	ck_assert_int_eq(0b00000100, bs[2]);

	program_free(&prog);
}
END_TEST

Suite*
suite_program()
{
	tcase_t tests[] = {
		{ "compile_one_inst",   &compile_one_inst   },
		{ "compile_two_inst",   &compile_two_inst   },
		{ "compile_three_inst", &compile_three_inst },
		{ "push_load_imm",      &push_load_imm      },
	};

	return tcase_build_suite("program", tests, sizeof(tests));
}

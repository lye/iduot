#include "test.h"
#include "program.h"

START_TEST(parse_empty)
{
	program_t prog;
	program_init(&prog);
	ck_assert_int_eq(0, program_parse_bytes(&prog, "", 0));
	ck_assert_int_eq(0, prog.insts_len);
	program_free(&prog);
}
END_TEST

START_TEST(parse_loadimm)
{
	const char *bs = "loadimm A 4";

	program_t prog;
	program_init(&prog);
	ck_assert_int_eq(0, program_parse_bytes(&prog, bs, strlen(bs)));
	ck_assert_int_eq(1, prog.insts_len);
	inst_t *inst = &prog.insts[0];
	ck_assert_int_eq(INST_LOAD_IMM, inst->type);
	ck_assert_int_eq(REG_ID_A, inst->load_imm.reg);
	ck_assert_int_eq(4, inst->load_imm.imm);
	program_free(&prog);
}
END_TEST

Suite*
suite_parser()
{
	tcase_t tests[] = {
		{ "parse_empty",   &parse_empty   },
		{ "parse_loadimm", &parse_loadimm },
	};

	return tcase_build_suite("parser", tests, sizeof(tests));
}

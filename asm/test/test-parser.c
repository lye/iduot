#include "test.h"
#include "program.h"
#include "utils.h"

static void
ck_assert_inst_eq(inst_t *a, inst_t *b)
{
	ck_assert_int_eq(a->type, b->type);

	switch (inst_type_format(a->type)) {
		case INST_FORMAT_REG1:
			ck_assert_int_eq(
				a->reg1.reg,
				b->reg1.reg
			);
			break;

		case INST_FORMAT_REG2:
			ck_assert_int_eq(
				a->reg2.reg1,
				b->reg2.reg1
			);
			ck_assert_int_eq(
				a->reg2.reg2,
				b->reg2.reg2
			);
			break;

		case INST_FORMAT_LOAD_IMM:
			ck_assert_int_eq(
				a->load_imm.reg,
				b->load_imm.reg
			);
			ck_assert_int_eq(
				a->load_imm.imm,
				b->load_imm.imm
			);
			break;

		default:
			UNREACHABLE;
	}
}

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

	inst_t expect = {
		.type = INST_LOAD_IMM,
		.load_imm = {
			.reg = REG_ID_A,
			.imm = 4,
		}
	};

	ck_assert_inst_eq(&expect, &prog.insts[0]);

	program_free(&prog);
}
END_TEST

START_TEST(parse_loadimm2_no_nl)
{
	const char *bs =
		"loadimm A 4\n"
		"loadimm B 2";

	program_t prog;
	program_init(&prog);

	ck_assert_int_eq(0, program_parse_bytes(&prog, bs, strlen(bs)));

	inst_t expect = {
		.type = INST_LOAD_IMM,
		.load_imm = {
			.reg = REG_ID_A,
			.imm = 4,
		},
	};

	ck_assert_int_eq(2, prog.insts_len);
	ck_assert_inst_eq(&expect, &prog.insts[0]);
	expect.load_imm.reg = REG_ID_B;
	expect.load_imm.imm = 2;
	ck_assert_inst_eq(&expect, &prog.insts[1]);

	program_free(&prog);
}
END_TEST

START_TEST(parse_loadimm2_nl)
{
	const char *bs = 
		"loadimm A 4\n"
		"loadimm B 2\n";

	program_t prog;
	program_init(&prog);

	ck_assert_int_eq(0, program_parse_bytes(&prog, bs, strlen(bs)));

	inst_t expect = {
		.type = INST_LOAD_IMM,
		.load_imm = {
			.reg = REG_ID_A,
			.imm = 4,
		},
	};

	ck_assert_int_eq(2, prog.insts_len);
	ck_assert_inst_eq(&expect, &prog.insts[0]);
	expect.load_imm.reg = REG_ID_B;
	expect.load_imm.imm = 2;
	ck_assert_inst_eq(&expect, &prog.insts[1]);

	program_free(&prog);
}
END_TEST

Suite*
suite_parser()
{
	tcase_t tests[] = {
		{ "parse_empty",          &parse_empty          },
		{ "parse_loadimm",        &parse_loadimm        },
		{ "parse_loadimm2_no_nl", &parse_loadimm2_no_nl },
		{ "parse_loadimm2_nl",    &parse_loadimm2_nl    },
	};

	return tcase_build_suite("parser", tests, sizeof(tests));
}

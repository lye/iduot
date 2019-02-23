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
	ck_assert_int_eq(2, prog.insts_len);

	inst_t expect[] = {
		{
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_ID_A,
				.imm = 0,
			},
		},
		{
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_IDS,
				.imm = 4,
			},
		},
	};

	ck_assert_inst_eq(&expect[0], &prog.insts[0]);
	ck_assert_inst_eq(&expect[1], &prog.insts[1]);

	program_free(&prog);
}
END_TEST

static void
ck_loadimm2(program_t *prog)
{
	inst_t expect[] = {
		{
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_ID_A,
			},
		},
		{
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_IDS,
				.imm = 4,
			},
		},
		{
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_ID_B,
			},
		},
		{
			.type = INST_LOAD_IMM,
			.load_imm = {
				.reg = REG_IDS,
				.imm = 2,
			},
		},
	};

	ck_assert_int_eq(4, prog->insts_len);

	ck_assert_inst_eq(&expect[0], &prog->insts[0]);
	ck_assert_inst_eq(&expect[1], &prog->insts[1]);
	ck_assert_inst_eq(&expect[2], &prog->insts[2]);
	ck_assert_inst_eq(&expect[3], &prog->insts[3]);
}

START_TEST(parse_loadimm2_no_nl)
{
	const char *bs =
		"loadimm A 4\n"
		"loadimm B 2";

	program_t prog;
	program_init(&prog);

	ck_assert_int_eq(0, program_parse_bytes(&prog, bs, strlen(bs)));
	ck_loadimm2(&prog);
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
	ck_loadimm2(&prog);
	program_free(&prog);
}
END_TEST

START_TEST(parse_loadimm2_nls)
{
	const char *bs = 
		"\n\nloadimm A 4\n\n\nloadimm B 2\n";

	program_t prog;
	program_init(&prog);

	ck_assert_int_eq(0, program_parse_bytes(&prog, bs, strlen(bs)));
	ck_loadimm2(&prog);
	program_free(&prog);
}
END_TEST

START_TEST(parse_halt)
{
	const char *bs = "halt A";

	program_t prog;
	program_init(&prog);

	ck_assert_int_eq(0, program_parse_bytes(&prog, bs, strlen(bs)));
	ck_assert_int_eq(1, prog.insts_len);
	ck_assert_int_eq(INST_HALT, prog.insts[0].type);
	ck_assert_int_eq(REG_ID_A, prog.insts[0].reg1.reg);

	program_free(&prog);
}
END_TEST

START_TEST(parse_mv)
{
	const char *bs = "mv A B";

	program_t prog;
	program_init(&prog);

	ck_assert_int_eq(0, program_parse_bytes(&prog, bs, strlen(bs)));
	ck_assert_int_eq(1, prog.insts_len);
	ck_assert_int_eq(INST_MV, prog.insts[0].type);
	ck_assert_int_eq(REG_ID_A, prog.insts[0].reg2.reg1);
	ck_assert_int_eq(REG_ID_B, prog.insts[0].reg2.reg2);

	program_free(&prog);
}
END_TEST

START_TEST(parse_labels)
{
	const char *bs =
		"halt A\n"
		"foo:\n"
		"halt B\n"
		"halt C\n"
		"bar:\n"
		"halt D";

	program_t prog;
	program_init(&prog);

	ck_assert_int_eq(0, program_parse_bytes(&prog, bs, strlen(bs)));
	ck_assert_int_eq(2, prog.labels_len);
	ck_assert_int_eq(1, program_label_find(&prog, "foo"));
	ck_assert_int_eq(3, program_label_find(&prog, "bar"));
	ck_assert_int_eq(-1, program_label_find(&prog, "baz"));
	program_free(&prog);
}
END_TEST

START_TEST(parse_label_dupe)
{
	const char *bs = "foo: halt A\nfoo: halt B";
	program_t prog;
	program_init(&prog);
	ck_assert_int_ne(0, program_parse_bytes(&prog, bs, strlen(bs)));
	program_free(&prog);
}
END_TEST

START_TEST(parse_load_imm_label)
{
	const char *bs = "loadimm A foo\nfoo:";
	program_t prog;

	program_init(&prog);
	ck_assert_int_eq(0, program_parse_bytes(&prog, bs, strlen(bs)));
	ck_assert_int_eq(2, prog.insts_len);

	ck_assert_int_eq(INST_LOAD_IMM, prog.insts[0].type);
	ck_assert_int_eq(REG_ID_A, prog.insts[0].load_imm.reg);
	ck_assert_int_eq(0, prog.insts[0].load_imm.imm);
	ck_assert_ptr_eq(NULL, prog.insts[0].load_imm.label);

	ck_assert_int_eq(INST_LOAD_IMM, prog.insts[1].type);
	ck_assert_int_eq(REG_IDS, prog.insts[1].load_imm.reg);
	ck_assert_int_eq(0, prog.insts[1].load_imm.imm);
	ck_assert_str_eq("foo", prog.insts[1].load_imm.label);

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
		{ "parse_loadimm2_nls",   &parse_loadimm2_nls   },
		{ "parse_halt",           &parse_halt           },
		{ "parse_mv",             &parse_mv             },
		{ "parse_labels",         &parse_labels         },
		{ "parse_label_dupe",     &parse_label_dupe     },
		{ "parse_load_imm_label", &parse_load_imm_label },
	};

	return tcase_build_suite("parser", tests, sizeof(tests));
}

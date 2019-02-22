%{
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/mman.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <strings.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include "inst.h"
	#include "program.h"

	extern int line_no;
	extern int yydebug;
	extern int yylex();
	extern int yyparse();
	void yyerror(const char *s);

	struct yy_buffer_state* yy_scan_bytes(const char *, size_t);
	void yy_delete_buffer(struct yy_buffer_state*);

	static program_t prog;
%}

%union {
	int         ival;
	char       *sval;
	inst_t      inst;
	inst_type_t inst_ty;
	reg_id_t    reg;
}

%token T_WS
%token T_NL
%token <ival> T_NUM
%token <sval> T_DECLR
%token <inst_ty> T_INSTR_LI
%token <inst_ty> T_INSTR_1
%token <inst_ty> T_INSTR_2
%token <reg>  T_REG
%token <slit> T_ID
%token <slit> T_LABEL
%debug

%%

program:
	stmt stmtlist
	| %empty
	{}

stmtlist:
	T_NL stmt stmtlist
	| T_NL
	| %empty
	{}

stmt:
	stmt_li
	| stmt_1
	| stmt_2
	| stmt_dec
	| stmt_label
	| T_NL
	{
		// no-op; mods handled by indiv. statements.
	}

stmt_dec:
	T_DECLR
	{
		// TODO DECLS
	}

stmt_label:
	T_LABEL
	{
		// TODO LABELS
	}

stmt_li:
	T_INSTR_LI T_WS T_REG T_WS T_NUM
	{
		if ($5 > 0xf) {
			yyerror("load immediate value exceeds maximum (0xf)");
		}

		inst_t i = {
			.type = $1,
			.load_imm = {
				.reg = $3,
				.imm = $5 & 0xf,
			},
		};

		program_push_inst(&prog, i);
	};

stmt_1:
	T_INSTR_1 T_WS T_REG
	{
		inst_t i = {
			.type = $1,
			.reg1 = {
				.reg = $3,
			},
		};

		program_push_inst(&prog, i);
	};

stmt_2:
	T_INSTR_2 T_WS T_REG T_WS T_REG
	{
		inst_t i = {
			.type = $1,
			.reg2 = {
				.reg1 = $3,
				.reg2 = $5,
			},
		};

		program_push_inst(&prog, i);
	};

%%

int
program_parse_file(program_t *this, const char *path)
{
	int fd = open(path, O_RDONLY);
	if (0 > fd) {
		return errno;
	}

	struct stat sb;
	if (0 != fstat(fd, &sb)) {
		close(fd);
		return errno;
	}

	void *bs = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (NULL == bs) {
		close(fd);
		return errno;
	}

	close(fd);

	int ret = program_parse_bytes(this, bs, sb.st_size);
	munmap(bs, sb.st_size);
	return ret;
}

int
program_parse_bytes(program_t *this, const void *bs, size_t len)
{
	yydebug = (NULL == getenv("DEBUG")) ? 0 : 1;
	line_no = 1;
	program_init(&prog);

	struct yy_buffer_state *buf = yy_scan_bytes(bs, len);
	int ret = yyparse();
	yy_delete_buffer(buf);

	*this = prog;

	return ret;
}

void
yyerror(const char *reason)
{
	printf("Parse error: %s, on line %d\n", reason, line_no);
}

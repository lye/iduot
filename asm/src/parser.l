%{
	#include "inst.h"
	#include "program.h"
	#include "utils.h"
	#include "parser.tab.h"

	int line_no = 1;
%}

%option noyywrap

ID    [a-zA-Z0-9_]+
DIGIT [0-9]

%%
[ \t]*\n {
	line_no += 1;
	return T_NL;
}

[ \t]+ {
	return T_WS;
}

;.*\n {
	line_no += 1;
	return T_NL;
}

\.{ID} {
	yylval.sval = yytext;
	return T_DECLR;
}

{ID}: {
	yylval.sval = yytext;
	return T_LABEL;
}

{DIGIT}+ {
	yylval.ival = atoi(yytext);
	return T_NUM;
}

{ID} {
	yylval.reg = reg_id_parse(yytext);
	if (REG_IDS != yylval.reg) {
		return T_REG;
	}

	yylval.inst_ty = inst_type_parse(yytext);
	if (INSTS != yylval.inst_ty) {
		switch (inst_type_format(yylval.inst_ty)) {
			case INST_FORMAT_REG1:
				return T_INSTR_1;
			case INST_FORMAT_REG2:
				return T_INSTR_2;
			case INST_FORMAT_LOAD_IMM:
				return T_INSTR_LI;
			case INST_FORMATS:
				break;
		}

		UNREACHABLE;
	}

	yylval.sval = yytext;
	return T_ID;
}

%%

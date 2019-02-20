#include <sys/stat.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <check.h>

typedef struct {
	const char *name;
	void (*func)();
}
tcase_t;

Suite* tcase_build_suite(const char *name, tcase_t *tests, size_t blen);

#define ck_err(err) \
	if (!NEM_err_ok(err)) { \
		char *tmp; \
		asprintf(\
			&tmp,\
			"%s\n  -> in %s, %s:%d",\
			NEM_err_string(err),\
			__FUNCTION__,\
			__FILE__,\
			__LINE__\
		); \
		ck_assert_msg(false, tmp); \
	}

// fucking shit.
#ifndef CK_FLOATING_DIG
#define CK_FLOATING_DIG 6
#endif 
#ifndef _ck_assert_floating
#define _ck_assert_floating(X, OP, Y, TP, TM) do { \
  TP _ck_x = (X); \
  TP _ck_y = (Y); \
  ck_assert_msg(_ck_x OP _ck_y, \
  "Assertion '%s' failed: %s == %.*" TM "g, %s == %.*" TM "g", \
  #X" "#OP" "#Y, \
  #X, (int)CK_FLOATING_DIG, _ck_x, \
  #Y, (int)CK_FLOATING_DIG, _ck_y); \
} while (0)
#endif

#ifndef ck_assert_float_eq
#define ck_assert_float_eq(X, Y) _ck_assert_floating(X, ==, Y, float, "")
#endif

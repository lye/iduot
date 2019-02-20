#define ARRSIZE(x) (sizeof(x)/sizeof((x)[0]))
#define STATIC_ASSERT(c, msg) _Static_assert(c, msg)
#define UNREACHABLE __builtin_unreachable()

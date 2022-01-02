#ifndef FILE_MINIPKG2_PRINT_H
#define FILE_MINIPKG2_PRINT_H
#include <stdnoreturn.h>
#include <stdbool.h>

extern int verbosity;

void print(int color, const char*, ...);
void println(int color, const char*, ...);
bool yesno(const char* q, bool def, ...);
void print_errno(int color, const char*, ...);

#define COLOR_LOG          32
#define COLOR_WARN         33
#define COLOR_ERROR        31
#define COLOR_INFO         34

#define log(...)           (verbosity >= 1 ? (println(COLOR_LOG, __VA_ARGS__), 0) : 0)
#define info(...)          println(COLOR_INFO, __VA_ARGS__)
#define warn(...)          println(COLOR_WARN, __VA_ARGS__)
#define error(...)         println(COLOR_ERROR, __VA_ARGS__)
#define fail(...)          (error(__VA_ARGS__), exit(1))
#define warn_errno(...)    (print_errno(COLOR_WARN, __VA_ARGS__))
#define error_errno(...)   (print_errno(COLOR_ERROR, __VA_ARGS__))
#define fail_errno(...)    (error_errno(__VA_ARGS__), exit(1))

// Check if ((`code` `cmp`) == 1); else fail.
// Notes:
// - `code` will be executed once.
// - Use check*f in fork() contexts.
#define check(code, cmp)   (((code) cmp) || (fail_errno("%s: %d: Failed to execute: %s", __func__, __LINE__, #code),0))

// Check if (`code` == `val`); else fail
#define checkv(code, val)  check(code, == (val))

// Check if (`code` == 0); else fail.
#define check0(code)       checkv(code, 0)

// Variants of check*,
// except that it will use _exit() instead of exit().
#define checkf(code, cmp)  (((code) cmp) || (error_errno("%s: %d: Failed to execute: %s", __func__, __LINE__, #code), _exit(10), 0))
#define checkvf(code, val) checkf(code, == (val))
#define check0f(code)      checkvf(code, 0)

#endif /* FILE_MINIPKG2_PRINT_H */

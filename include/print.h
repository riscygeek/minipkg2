#ifndef FILE_MINIPKG2_PRINT_H
#define FILE_MINIPKG2_PRINT_H
#include <stdbool.h>

void print(int color, const char*, ...);
bool yesno(const char* q, bool def, ...);


#define log(...)     print(32, __VA_ARGS__)
#define warn(...)    print(33, __VA_ARGS__)
#define fail(...)    (print(31, __VA_ARGS__), exit(1))

#endif /* FILE_MINIPKG2_PRINT_H */

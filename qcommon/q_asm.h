#pragma once

// msvc, gcc and clang defines to allow inline assembly with intel syntax
// compile with -masm=intel

#if defined(_MSC_VER)

#define qasm1(a) __asm a
#define qasm2(a, b) __asm a, b
#define qasm3(a, b, c) __asm a, b, c
#define qasmL(a) __asm a

#define qnakedstart(a) __asm lea eax, [__hookstart] __asm jmp __hookend __asm __hookstart:
#define qnakedend(a) __asm __hookend:

#elif defined(__GNUC__)

// gcc has a few issues with defines in function defines
// So we'll create a double define to ensure the defines are properly processed
#define qasm1i(a) __asm__(#a "\n");
#define qasm2i(a, b) __asm__(#a ", " #b "\n");
#define qasm3i(a, b, c) __asm__(#a ", " #b ", " #c "\n");

// proxy defines
#define qasm1(a) qasm1i(a)
#define qasm2(a, b) qasm2i(a, b)
#define qasm3(a, b, c) qasm3i(a, b, c)

#define qasmL(a) __asm__(#a "\n");

#define qnakedstart(a)                                                                                                                                         \
    __asm__(".intel_syntax noprefix\n");                                                                                                                       \
    __asm__("lea eax, [__hookstart" #a "]\n");                                                                                                                 \
    __asm__("jmp __hookend" #a "\n");                                                                                                                          \
    __asm__(".att_syntax\n");                                                                                                                                  \
    __asm__("__hookstart" #a ":\n");                                                                                                                           \
    __asm__(".intel_syntax noprefix\n");

#define qnakedend(a)                                                                                                                                           \
    __asm__(".att_syntax\n");                                                                                                                                  \
    __asm__("__hookend" #a ":\n");                                                                                                                             \
    __asm__(".intel_syntax noprefix\n");

#elif defined(__clang__)

// in order to use intel syntax need "Codewarrior-style inline assembly" enabled
#define qasm1i(a) __asm { a}
#define qasm2i(a, b) __asm { a, b}
#define qasm3i(a, b, c) __asm { a, b, c}

// proxy defines
#define qasm1(a) qasm1i(a)
#define qasm2(a, b) qasm2i(a, b)
#define qasm3(a, b, c) qasm3i(a, b, c)

#define qasmL(a) __asm__(#a "\n");

#define qnakedstart(a)                                                                                                                                         \
    __asm__(".intel_syntax noprefix\n");                                                                                                                       \
    __asm__("lea eax, [__hookstart" #a "]\n");                                                                                                                 \
    __asm__("jmp __hookend" #a "\n");                                                                                                                          \
    __asm__(".att_syntax\n");                                                                                                                                  \
    __asm__("__hookstart" #a ":\n");                                                                                                                           \
    __asm__(".intel_syntax noprefix\n");

#define qnakedend(a)                                                                                                                                           \
    __asm__(".att_syntax\n");                                                                                                                                  \
    __asm__("__hookend" #a ":\n");                                                                                                                             \
    __asm__(".intel_syntax noprefix\n");

#else

#error missing inline assembly for your compiler

#endif

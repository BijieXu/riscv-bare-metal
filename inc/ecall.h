#ifndef _ASM_RISCV_ECALL_H
#define _ASM_RISCV_ECALL_H
#define RISCV_ECALL(which, arg0, arg1, arg2) ({            \
    register unsigned long a0 asm ("a0") = (unsigned long)(arg0);   \
    register unsigned long a1 asm ("a1") = (unsigned long)(arg1);   \
    register unsigned long a2 asm ("a2") = (unsigned long)(arg2);   \
    register unsigned long a7 asm ("a7") = (unsigned long)(which);  \
    asm volatile ("ecall"                   \
              : "+r" (a0)               \
              : "r" (a1), "r" (a2), "r" (a7)        \
              : "memory");              \
    a0;                         \
})
#define RISCV_ECALL_0(which) RISCV_ECALL(which, 0, 0, 0)
#define RISCV_ECALL_1(which, arg0) RISCV_ECALL(which, arg0, 0, 0)
#define RISCV_ECALL_2(which, arg0, arg1) RISCV_ECALL(which, arg0, arg1, 0)
#endif
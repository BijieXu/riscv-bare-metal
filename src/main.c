#include <timer.h>

#include "kalloc.h"
#include "kvm.h"
#include "mm.h"
#include "trap.h"
#include "uart.h"
#include "ecall.h"

extern int m_count;
extern int s_count;
extern int system_err;
int main() {
    uart_init();
    kinit();
    kvminit();
    kvminithart();
    paginginit();
    kernel_trap_init();
    print_s("Hello world!\n");
    print_s("Raise exception to enable timer...\n");
    msoftint_make();
    RISCV_ECALL_0(77);
    print_s("Back to user mode\n");
    while (!system_err)
        ;
    printf("system_err = %d\n", system_err);
    return 0;
}

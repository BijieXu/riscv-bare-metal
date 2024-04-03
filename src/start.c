#include "main.h"
#include "riscv.h"
#include "stdint.h"
#include "timer.h"
#include "ecall.h"
#include "uart.h"

// extern void timervec();
extern void trap_entry();

void start() {
    // set M Previous Privilege mode to Supervisor, for mret.
    unsigned long x = r_mstatus();
    x &= ~MSTATUS_MPP_MASK;
    x |= MSTATUS_MPP_S;
    w_mstatus(x);
    // enable machine-mode interrupt
    x = r_mstatus();
    x |= MSTATUS_MIE;
    w_mstatus(x);
    // enable machine-mode timer and soft interrupt.
    x = r_mie();
    x |= MIE_MTIE | MIE_MSIE;
    w_mie(x);

    // set M Exception Program Counter to main, for mret.
    // requires gcc -mcmodel=medany
    w_mepc((uint64_t)main);

    // disable paging for now.
    w_satp(0);

    // physical memory protection
    w_pmpcfg0(0xf);
    w_pmpaddr0(0xffffffffffffffff);

    // delegate all interrupts and exceptions to supervisor mode.
    //w_medeleg(0xffff);
    //w_mideleg(0xffff);
    //w_medeleg(0xb109);
    //w_mideleg(0x222);

    // setup trap_entry
    w_mtvec((uint64_t)trap_entry);

    // keep each CPU's hartid in its tp register, for cpuid().
    int id = r_mhartid();
    w_tp(id);

    //timer_init();

    // switch to supervisor mode and jump to main().
    asm volatile("mret");
}

void start_kernel() {
    // set M Previous Privilege mode to Supervisor, for mret.
    unsigned long x = r_mstatus();
    x &= ~MSTATUS_MPP_MASK;
    x |= MSTATUS_MPP_S;
    w_mstatus(x);
    // enable machine-mode interrupt
    x = r_mstatus();
    x |= MSTATUS_MIE;
    w_mstatus(x);
    // enable machine-mode timer and soft interrupt.
    x = r_mie();
    x |= MIE_MTIE | MIE_MSIE;
    w_mie(x);

    // set M Exception Program Counter to main, for mret.
    // requires gcc -mcmodel=medany
    w_mepc((uint64_t)main);

    // disable paging for now.
    w_satp(0);

    // physical memory protection
    w_pmpcfg0(0xf);
    w_pmpaddr0(0xffffffffffffffff);

    // delegate all interrupts and exceptions to supervisor mode.
    //w_medeleg(0xffff);
    //w_mideleg(0xffff);
    w_medeleg(0xb109);
    w_mideleg(0x222);

    // setup trap_entry
    w_mtvec((uint64_t)trap_entry);

    // keep each CPU's hartid in its tp register, for cpuid().
    int id = r_mhartid();
    w_tp(id);

    //timer_init();

    // switch to supervisor mode and jump to main().
    asm volatile("mret");
}

void start_main() {
    //w_mtvec((uint64_t)trap_entry);
    uart_init();
    print_s("Hello world!!!!\n");
    //asm volatile("ecall");
    print_s("Raise exception to enable timer...\n");
    RISCV_ECALL_0(88);
    print_s("\nBack to main\n");
    while (1)
        ;
    return;
}

void start_user() {
    // set M Previous Privilege mode to User, for mret.
    unsigned long x = r_mstatus();
    x &= ~MSTATUS_MPP_MASK;
    x |= MSTATUS_MPP_U;
    w_mstatus(x);
    // enable machine-mode interrupt
    x = r_mstatus();
    x |= MSTATUS_MIE;
    w_mstatus(x);
    // enable machine-mode timer and soft interrupt.
    x = r_mie();
    x |= MIE_MTIE | MIE_MSIE;
    w_mie(x);

    // set M Exception Program Counter to main, for mret.
    // requires gcc -mcmodel=medany
    w_mepc((uint64_t)main);

    // disable paging for now.
    w_satp(0);

    // physical memory protection
    w_pmpcfg0(0xf);
    w_pmpaddr0(0xffffffffffffffff);

    // delegate all interrupts and exceptions to supervisor mode.
    //w_medeleg(0xffff);
    //w_mideleg(0xffff);

    // setup trap_entry
    //w_stvec((uint64_t)trap_entry);
    w_mtvec((uint64_t)trap_entry);

    // keep each CPU's hartid in its tp register, for cpuid().
    int id = r_mhartid();
    w_tp(id);

    //timer_init();

    // switch to supervisor mode and jump to main().
    asm volatile("mret");
}
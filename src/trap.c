#include "riscv.h"
#include "uart.h"
//#include "encoding.h"
#define CLINT_BASE 0x2000000
#define MTIME (volatile unsigned long long int *)(CLINT_BASE + 0xbff8)
#define MTIMECMP (volatile unsigned long long int *)(CLINT_BASE + 0x4000)

volatile int system_err = 0;

void msoftint_make(void)
{
    *(volatile unsigned int*)CLINT_BASE = 1;
}
void msoftint_clear(void)
{
    *(volatile unsigned int*)CLINT_BASE = 0;
}

#define S_SOFT_INT      (1)
#define M_SOFT_INT      (3)
#define S_TIMER_INT     (5)
#define M_TIMER_INT     (7)
#define S_EXT_INT       (9)
#define M_EXT_INT       (11)

#define INSTRUCTION_ADDR_MISALIGNED     (0)
#define INSTRUCTION_ACCESS_FAULT        (1)
#define ILLEGAL_INSTRUCTION             (2)
#define BREAK_POINT                     (3)
#define LOAD_ADDR_MISALIGNED            (4)
#define LOAD_ACCESS_FAULT               (5)
#define STORE_ADDR_MISALIGNED           (6)
#define STORE_ACCESS_FAULT              (7)
#define ECALL_FROM_UMODE                (8)
#define ECALL_FROM_SMODE                (9)
#define ECALL_FROM_MMODE                (11)
#define INSTRUCTION_PAGE_FAULT          (12)
#define LOAD_PAGE_FAULT                 (13)
#define STORE_PAGE_FAULT                (15)

static char *interrupt_cause[] = {
    "Reserved",
    "Supervisor software interrupt",
    "Reserved",
    "Machine software interrupt",
    "Reserved",
    "Supervisor timer interrupt",
    "Reserved",
    "Machine timer interrupt",
    "Reserved",
    "Supervisor external interrupt",
    "Reserved",
    "Machine external interrupt",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

static char *exception_cause[] = {
    "Instruction address misaligned",
    "Instruction access fault",
    "Illegal instruction",
    "Breakpoint",
    "Load address misaligned",
    "Load access fault",
    "Store/AMO address misaligned",
    "Store/AMO access fault",
    "Environment call from U-mode",
    "Environment call from S-mode",
    "Reserved",
    "Environment call from M-mode",
    "Instruction page fault",
    "Load page fault",
    "Reserved",
    "Store/AMO page fault"
};

int m_count = 0;

void handle_interrupt(uint64_t mcause) {
    int mcode = mcause & 0xff;
    printf("M-Interrupt : %s.\r\n", interrupt_cause[mcode]);
    
    switch (mcode) {
        case M_SOFT_INT:
            msoftint_clear();
            break;
        case M_TIMER_INT:
            *MTIMECMP = *MTIME + 0xfffff * 5;
            m_count++;
            //if (m_count == 10) {
            if (system_err) {
                unsigned long long int mie;
                asm volatile("csrr %0, mie" : "=r"(mie));
                mie &= ~(1 << 7);
                asm volatile("csrw mie, %0" : "=r"(mie));
            }
            w_sip(SIP_SSIP); 
            break;
        default:
            system_err = -1;
            break;
    }
}

void handle_exception(uint64_t mcause) {
    unsigned long long int mie;
    unsigned long long int opcode;
    int mcode = mcause & 0xff;
    printf("M-Exception : %s.\r\n", exception_cause[mcode]); 
    switch (mcode) {
        case ILLEGAL_INSTRUCTION:
            unsigned long mepc;
            unsigned long tval;
            asm volatile("csrr %0, mepc" : "=r"(mepc));
            asm volatile("csrr %0, mtval" : "=r"(tval));
            printf("tval = %p\r\n", tval);
            printf("mepc = %p\r\n", mepc);
            break;
        case ECALL_FROM_SMODE:
            *MTIMECMP = *MTIME + 0xfffff * 5;

            asm volatile("csrr %0, mie" : "=r"(mie));
            mie |= (1 << 7);
            asm volatile("csrw mie, %0" : "=r"(mie));
            asm volatile("mv %0, a7" : "=r"(opcode));
            break;
        case ECALL_FROM_UMODE:
            *MTIMECMP = *MTIME + 0xfffff * 5;

            asm volatile("csrr %0, mie" : "=r"(mie));
            mie |= (1 << 7);
            asm volatile("csrw mie, %0" : "=r"(mie));
            asm volatile("mv %0, a7" : "=r"(opcode));
            break;
        default:
            system_err = -2;
            break;
    }
}

void handle_trap() {
    uint64_t mcause, mepc;
    asm volatile("csrr %0, mcause" : "=r"(mcause));
    asm volatile("csrr %0, mepc" : "=r"(mepc));

    if (mcause >> 63) {
        handle_interrupt(mcause);
    } else {
        handle_exception(mcause);
        asm volatile("csrr t0, mepc");
        asm volatile("addi t0, t0, 0x4");
        asm volatile("csrw mepc, t0");
    }
}

int s_count = 0;
void handle_s_interrupt(uint64_t scause) {
    int scode = scause & 0xff;
    printf("S-Interrupt : %s.\r\n", interrupt_cause[scode]);
    
    switch (scode) {
        case S_SOFT_INT:
            w_sip(r_sip() & (~SIP_SSIP));
            for (int i= 0; (!system_err) && i < 4096; i++)
                kalloc();
            break;
        case S_TIMER_INT:
            s_count++;
            break;
        default:
            system_err = -3;
            break;
    }
}

void handle_s_exception(uint64_t scause) {
    int scode = scause & 0xff;
    printf("S-Exception : %s.\r\n", exception_cause[scode]); 
    switch (scode) {
        case ILLEGAL_INSTRUCTION:
            unsigned long sepc;
            unsigned long tval;
            asm volatile("csrr %0, sepc" : "=r"(sepc));
            asm volatile("csrr %0, stval" : "=r"(tval));
            printf("tval = %p\r\n", tval);
            printf("sepc = %p\r\n", sepc);
            break;
        case ECALL_FROM_SMODE:
            break;
        case ECALL_FROM_UMODE:
            break;
        default:
            system_err = -4;
            break;
    }
}

void kerneltrap() {
    uint64_t scause, sepc;
    asm volatile("csrr %0, scause" : "=r"(scause));
    asm volatile("csrr %0, sepc" : "=r"(sepc));

    if (scause >> 63) {
        handle_s_interrupt(scause);
    } else {
        handle_s_exception(scause);
        asm volatile("csrr t0, sepc");
        asm volatile("addi t0, t0, 0x4");
        asm volatile("csrw sepc, t0");
    }
}

extern void kernelvec();
extern unsigned long __stack_start;
void kernel_trap_init(void)
{
    //this stack is used for supervisor_trap_entry in entry.S
    w_sscratch((unsigned long)(__stack_start + 4096 * 3));
    // set the supervisor trap handler.
    w_stvec((unsigned long)kernelvec);
    // enable supervisor interrupts.
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    // enable supervisor timer and soft interrupts.
    w_sie(r_sie() | SIE_STIE | SIE_SSIE);
}
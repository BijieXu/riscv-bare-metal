#include <stdint.h>
#include <stdarg.h>

#include "uart.h"

static volatile uint8_t *uart;

void uart_init() {
    uart = (uint8_t *)(void *)NS16550A_UART0_CTRL_ADDR;
    uint32_t uart_freq = UART0_CLOCK_FREQ;
    uint32_t baud_rate = UART0_BAUD_RATE;
    uint32_t divisor = uart_freq / (16 * baud_rate);
    uart[UART_LCR] = UART_LCR_DLAB;
    uart[UART_DLL] = divisor & 0xff;
    uart[UART_DLM] = (divisor >> 8) & 0xff;
    uart[UART_LCR] = UART_LCR_PODD | UART_LCR_8BIT;
}

static int ns16550a_putchar(int ch) {
    while ((uart[UART_LSR] & UART_LSR_RI) == 0)
        ;
    return uart[UART_THR] = ch & 0xff;
}

void uart_send(char c) { ns16550a_putchar(c); }

void print_s(const char *s) {
    while (*s != '\0') {
        /* convert newline to carrige return + newline */
        if (*s == '\n') uart_send('\r');
        uart_send(*s++);
    }
}

void print_c(char c) { uart_send(c); }

void print_i(unsigned long int x) {
    if (x < 0) {
        print_c('-');
        x = -x;
    }
    if (x >= 10) print_i(x / 10);
    print_c(x % 10 + '0');
}

void print_h(unsigned long int d) {
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_send(n);
    }
}

//printf functions
#define uart_putc uart_send
static char digits[] = "0123456789abcdef";
static void print_int(int xx, int base, int sign)
{
    char buf[16];
    int i;
    unsigned int x;
    if(sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;
    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while((x /= base) != 0);
    if(sign)
        buf[i++] = '-';
    while(--i >= 0)
        uart_putc(buf[i]);
}
static void print_ptr(unsigned long x)
{
    int i;
    uart_putc('0');
    uart_putc('x');
    for (i = 0; i < (sizeof(unsigned long) * 2); i++, x <<= 4)
        uart_putc(digits[x >> (sizeof(unsigned long) * 8 - 4)]);
}
// Print to the console. only understands %d, %x, %p, %s.
void printf(char *fmt, ...)
{
    va_list ap;
    int i, c;
    char *s;
    if (fmt == 0)
        return;
    va_start(ap, fmt);
    for(i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if(c != '%'){
            uart_putc(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if(c == 0)
            break;
        switch(c){
        case 'd':
            print_int(va_arg(ap, int), 10, 1);
            break;
        case 'x':
            print_int(va_arg(ap, int), 16, 1);
            break;
        case 'p':
            print_ptr(va_arg(ap, unsigned long));
            break;
        case 's':
            if((s = va_arg(ap, char*)) == 0)
                s = "(null)";
            for(; *s; s++)
                uart_putc(*s);
            break;
        case '%':
            uart_putc('%');
            break;
        default:
            // Print unknown % sequence to draw attention.
            uart_putc('%');
            uart_putc(c);
            break;
        }
    }
}

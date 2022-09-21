#include <asm/herring.h>

void duart_init(void)
{
    MEM(DUART_IMR) = 0x00;        // Mask all interrupts
    MEM(DUART_MR1B) = 0b00010011; // No Rx RTS, No Parity, 8 bits per character
    MEM(DUART_MR2B) = 0b00000111; // Channel mode normal, No Tx RTS, No CTS, stop bit length 1
    MEM(DUART_ACR) = 0x80;        // Baudrate set 2
    MEM(DUART_CRB) = 0x80;        // Set Rx extended bit
    MEM(DUART_CRB) = 0xA0;        // Set Tx extended bit
    MEM(DUART_CSRB) = 0x77;       // 57600 baud
    MEM(DUART_CRB) = 0b0101;      // Enable Tx/Rx
}

void duart_putc(char c)
{
    while ((MEM(DUART_SRB) & 0b00000100) == 0)
    {
    }

    MEM(DUART_TBB) = c;

    // Always print a carriage return after a line feed
    if (c == 0x0A)
    {
        duart_putc(0x0D);
    }
}

char duart_getc(void)
{
    while ((MEM(DUART_SRB) & 0b00000001) == 0)
    {
    }

    return MEM(DUART_RBB);
}

void duart_puts(const char *s)
{
    unsigned i = 0;

    while (s[i] != 0)
    {
        duart_putc(s[i]);
        i++;
    }
}

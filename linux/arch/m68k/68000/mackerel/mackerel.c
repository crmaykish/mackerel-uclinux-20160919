#include <asm/mackerel.h>

void duart_putc(char c)
{
    while ((MEM(DUART1_SRB) & 0b00000100) == 0)
    {
    }

    MEM(DUART1_TBB) = c;

    // Always print a carriage return after a line feed
    if (c == 0x0A)
    {
        duart_putc(0x0D);
    }
}

char duart_getc(void)
{
    while ((MEM(DUART1_SRB) & 0b00000001) == 0)
    {
    }

    return MEM(DUART1_RBB);
}

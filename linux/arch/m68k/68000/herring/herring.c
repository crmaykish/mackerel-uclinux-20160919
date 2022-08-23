#include <asm/herring.h>

void mfp_putc(char s)
{
    while ((MEM(MFP_TSR) & 0b10000000) == 0)
    {
    }

    MEM(MFP_UDR) = s;

    if (s == '\n')
    {
        mfp_putc('\r');
    }
}

void mfp_puts(const char *s)
{
    unsigned i = 0;

    while (s[i] != 0)
    {
        mfp_putc(s[i]);
        i++;
    }
}

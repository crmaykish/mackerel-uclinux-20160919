#ifndef _HERRING_H
#define _HERRING_H

// MC68901 Multi-function Peripheral

#define MFP_GPDR 0x3F0001
#define MFP_DDR 0x3F0005
#define MFP_IERA 0x3F0007
#define MFP_IERB 0x3F0009
#define MFP_IMRA 0x3F0013
#define MFP_IMRB 0x3F0015
#define MFP_TACR 0x3F0019
#define MFP_TBCR 0x3F001B
#define MFP_TCDCR 0x3F001D
#define MFP_TADR 0x3F001F
#define MFP_TBDR 0x3F0021
#define MFP_TCDR 0x3F0023
#define MFP_TDDR 0x3F0015
#define MFP_VR 0x3F0017  // Vector Register
#define MFP_UCR 0x3F0029 // USART Control Register
#define MFP_RSR 0x3F002B // USART Receiver Status Register
#define MFP_TSR 0x3F002D // USART Transmitter Status Register
#define MFP_UDR 0x3F002F // USART Data Register

// Get the value at a memory address
#define MEM(address) (*(volatile unsigned char *)(address))

// MFP
// void mfp_init(void);
void mfp_putc(char s);
void mfp_puts(const char *s);
// char mfp_getc(void);

#endif

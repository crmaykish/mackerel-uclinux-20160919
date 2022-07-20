#ifndef __CVMX_ADDRESSES_USB_H__
#define __CVMX_ADDRESSES_USB_H__

#include <linux/delay.h>

/*
 * The macros cvmx_likely and cvmx_unlikely use the
 * __builtin_expect GCC operation to control branch
 * probabilities for a conditional. For example, an "if"
 * statement in the code that will almost always be
 * executed should be written as "if (cvmx_likely(...))".
 * If the "else" section of an if statement is more
 * probable, use "if (cvmx_unlikey(...))".
 */
#define cvmx_likely(x)		__builtin_expect(!!(x), 1)
#define cvmx_unlikely(x)	__builtin_expect(!!(x), 0)

#define	cvmx_wait_usec(us)	udelay(us)

#define	CVMX_ENABLE_CSR_ADDRESS_CHECKING	0

static inline uint64_t CVMX_USBCX_DAINT(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_DAINT(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000818ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_DAINTMSK(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_DAINTMSK(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F001000081Cull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_DCFG(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_DCFG(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000800ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_DCTL(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_DCTL(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000804ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_DIEPCTLX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 4)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_DIEPCTLX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000900ull) + ((offset&7) + (block_id&1)*0x8000000000ull)*32;
}

static inline uint64_t CVMX_USBCX_DIEPINTX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 4)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_DIEPINTX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000908ull) + ((offset&7) + (block_id&1)*0x8000000000ull)*32;
}

static inline uint64_t CVMX_USBCX_DIEPMSK(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_DIEPMSK(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000810ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_DIEPTSIZX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 4)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_DIEPTSIZX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000910ull) + ((offset&7) + (block_id&1)*0x8000000000ull)*32;
}

static inline uint64_t CVMX_USBCX_DOEPCTLX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 4)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_DOEPCTLX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000B00ull) + ((offset&7) + (block_id&1)*0x8000000000ull)*32;
}

static inline uint64_t CVMX_USBCX_DOEPINTX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 4)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_DOEPINTX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000B08ull) + ((offset&7) + (block_id&1)*0x8000000000ull)*32;
}

static inline uint64_t CVMX_USBCX_DOEPMSK(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_DOEPMSK(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000814ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_DOEPTSIZX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 4)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 4)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_DOEPTSIZX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000B10ull) + ((offset&7) + (block_id&1)*0x8000000000ull)*32;
}

static inline uint64_t CVMX_USBCX_DPTXFSIZX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((((offset >= 1) && (offset <= 4))) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((((offset >= 1) && (offset <= 4))) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((((offset >= 1) && (offset <= 4))) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((((offset >= 1) && (offset <= 4))) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((((offset >= 1) && (offset <= 4))) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_DPTXFSIZX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000100ull) + ((offset&7) + (block_id&1)*0x40000000000ull)*4;
}

static inline uint64_t CVMX_USBCX_DSTS(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_DSTS(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000808ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_DTKNQR1(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_DTKNQR1(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000820ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_DTKNQR2(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_DTKNQR2(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000824ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_DTKNQR3(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_DTKNQR3(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000830ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_DTKNQR4(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_DTKNQR4(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000834ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GAHBCFG(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GAHBCFG(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000008ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GHWCFG1(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GHWCFG1(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000044ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GHWCFG2(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GHWCFG2(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000048ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GHWCFG3(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GHWCFG3(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F001000004Cull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GHWCFG4(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GHWCFG4(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000050ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GINTMSK(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GINTMSK(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000018ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GINTSTS(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GINTSTS(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000014ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GNPTXFSIZ(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GNPTXFSIZ(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000028ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GNPTXSTS(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GNPTXSTS(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F001000002Cull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GOTGCTL(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GOTGCTL(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000000ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GOTGINT(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GOTGINT(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000004ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GRSTCTL(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GRSTCTL(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000010ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GRXFSIZ(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GRXFSIZ(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000024ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GRXSTSPD(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GRXSTSPD(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010040020ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GRXSTSPH(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GRXSTSPH(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000020ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GRXSTSRD(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GRXSTSRD(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F001004001Cull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GRXSTSRH(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GRXSTSRH(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F001000001Cull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GSNPSID(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GSNPSID(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000040ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_GUSBCFG(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_GUSBCFG(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F001000000Cull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_HAINT(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_HAINT(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000414ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_HAINTMSK(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_HAINTMSK(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000418ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_HCCHARX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 7)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_HCCHARX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000500ull) + ((offset&7) + (block_id&1)*0x8000000000ull)*32;
}

static inline uint64_t CVMX_USBCX_HCFG(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_HCFG(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000400ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_HCINTMSKX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 7)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_HCINTMSKX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F001000050Cull) + ((offset&7) + (block_id&1)*0x8000000000ull)*32;
}

static inline uint64_t CVMX_USBCX_HCINTX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 7)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_HCINTX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000508ull) + ((offset&7) + (block_id&1)*0x8000000000ull)*32;
}

static inline uint64_t CVMX_USBCX_HCSPLTX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 7)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_HCSPLTX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000504ull) + ((offset&7) + (block_id&1)*0x8000000000ull)*32;
}

static inline uint64_t CVMX_USBCX_HCTSIZX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 7)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_HCTSIZX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000510ull) + ((offset&7) + (block_id&1)*0x8000000000ull)*32;
}

static inline uint64_t CVMX_USBCX_HFIR(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_HFIR(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000404ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_HFNUM(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_HFNUM(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000408ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_HPRT(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_HPRT(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000440ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_HPTXFSIZ(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_HPTXFSIZ(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000100ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_HPTXSTS(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_HPTXSTS(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000410ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBCX_NPTXDFIFOX(unsigned long offset, unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && (((offset <= 7)) && ((block_id == 0)))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && (((offset <= 7)) && ((block_id <= 1))))))
        cvmx_warn("CVMX_USBCX_NPTXDFIFOX(%lu,%lu) is invalid on this chip\n", offset, block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010001000ull) + ((offset&7) + (block_id&1)*0x100000000ull)*4096;
}

static inline uint64_t CVMX_USBCX_PCGCCTL(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBCX_PCGCCTL(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0010000E00ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_BIST_STATUS(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_BIST_STATUS(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00011800680007F8ull) + (block_id&1)*0x10000000ull;
}

static inline uint64_t CVMX_USBNX_CLK_CTL(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_CLK_CTL(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x0001180068000010ull) + (block_id&1)*0x10000000ull;
}

static inline uint64_t CVMX_USBNX_CTL_STATUS(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_CTL_STATUS(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000800ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_INB_CHN0(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_INB_CHN0(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000818ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_INB_CHN1(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_INB_CHN1(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000820ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_INB_CHN2(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_INB_CHN2(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000828ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_INB_CHN3(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_INB_CHN3(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000830ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_INB_CHN4(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_INB_CHN4(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000838ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_INB_CHN5(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_INB_CHN5(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000840ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_INB_CHN6(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_INB_CHN6(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000848ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_INB_CHN7(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_INB_CHN7(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000850ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_OUTB_CHN0(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_OUTB_CHN0(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000858ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_OUTB_CHN1(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_OUTB_CHN1(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000860ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_OUTB_CHN2(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_OUTB_CHN2(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000868ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_OUTB_CHN3(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_OUTB_CHN3(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000870ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_OUTB_CHN4(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_OUTB_CHN4(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000878ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_OUTB_CHN5(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_OUTB_CHN5(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000880ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_OUTB_CHN6(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_OUTB_CHN6(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000888ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA0_OUTB_CHN7(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA0_OUTB_CHN7(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000890ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_DMA_TEST(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_DMA_TEST(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x00016F0000000808ull) + (block_id&1)*0x100000000000ull;
}

static inline uint64_t CVMX_USBNX_INT_ENB(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_INT_ENB(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x0001180068000008ull) + (block_id&1)*0x10000000ull;
}

static inline uint64_t CVMX_USBNX_INT_SUM(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_INT_SUM(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x0001180068000000ull) + (block_id&1)*0x10000000ull;
}

static inline uint64_t CVMX_USBNX_USBP_CTL_STATUS(unsigned long block_id)
{
#if CVMX_ENABLE_CSR_ADDRESS_CHECKING
    if (!(
        (OCTEON_IS_MODEL(OCTEON_CN30XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN50XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN31XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN56XX) && ((block_id == 0))) ||
        (OCTEON_IS_MODEL(OCTEON_CN52XX) && ((block_id <= 1)))))
        cvmx_warn("CVMX_USBNX_USBP_CTL_STATUS(%lu) is invalid on this chip\n", block_id);
#endif
    return CVMX_ADD_IO_SEG(0x0001180068000018ull) + (block_id&1)*0x10000000ull;
}

int __cvmx_helper_board_usb_get_num_ports(int supported_ports)
{
    switch (cvmx_sysinfo_get()->board_type) {
        case CVMX_BOARD_TYPE_NIC_XLE_4G:
            return 0;
    }

    return supported_ports;
}

#endif /* __CVMX_ADDRESSES_USB_H__ */

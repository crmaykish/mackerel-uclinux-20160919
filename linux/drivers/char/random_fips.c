/*
 * random_fips.c -- A FIPS compliant random number generator
 *
 * Copyright Pauli <paul_dale@securecomputing.com>, 2008
 *
 * Copyright Mick McCreath <mick_mccreath@securecomputing.com>, 2007
 *
 * Copyright Matt Mackall <mpm@selenic.com>, 2003, 2004, 2005
 *
 * Copyright Theodore Ts'o, 1994, 1995, 1996, 1997, 1998, 1999.  All
 * rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * ALTERNATIVELY, this product may be distributed under the terms of
 * the GNU General Public License, in which case the provisions of the GPL are
 * required INSTEAD OF the above restrictions.  (This clause is
 * necessary due to a potential bad interaction between the GPL and
 * the restrictions contained in a BSD-style copyright.)
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/*
 * (now, with legal B.S. out of the way.....)
 *
 * This is a FIPS approved RNG using the AES 128-bit key method from the
 * document "NIST-Recommended Random Number Generator Based on ANSI X9.31
 * Appendix A.2.4 Using the 3-Key Triple DES and AES Algorithms", dated
 * January 31, 2005.
 *
 * It is based upon random.c, with all of the entropy store and pool stuff
 * removed.
 *
 * It is an alternative to random.c and is configured with CONFIG_FIPS_RNG, 
 * which will NOT compile in random.c, hence some code duplication of 'Random
 * functions for networking'.
 *
 * Exported interfaces ---- output
 * ===============================
 *
 * There are three exported interfaces; the first is one designed to
 * be used from within the kernel:
 *
 * 	void get_random_bytes(void *buf, int nbytes);
 *
 * This interface will return the requested number of random bytes,
 * and place it in the requested buffer.
 *
 * The two other interfaces are two character devices /dev/random and
 * /dev/urandom.  Both of these interfaces are considered to be the same and
 * are suitable for use when very high quality randomness is desired (for
 * example, for key generation or one-time pads).
 *
 * Configuring the /dev/random driver under Linux
 * ==============================================
 *
 * The /dev/random driver under Linux uses minor numbers 8 and 9 of
 * the /dev/mem major number (#1).  So if your system does not have
 * /dev/random and /dev/urandom created already, they can be created
 * by using the commands:
 *
 * 	mknod /dev/random c 1 8
 * 	mknod /dev/urandom c 1 9
 */

#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/percpu.h>
#include <linux/crypto.h>
#include <linux/cryptohash.h>
#include <linux/time.h>
#include <linux/scatterlist.h>
#include <linux/highmem.h>
#include <linux/syscalls.h>
#include <crypto/aes.h>

#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>

/*
 * Macro's, defines and constants
 */
#ifdef RANDOM_FIPS_DEBUG
#	define DEBUG_ENT(fmt, arg...)	\
		printk(KERN_DEBUG "%s,%d:" fmt, __FILE__, __LINE__, ## arg);
#else
#	define DEBUG_ENT(fmt, arg...)
#endif

/* Size of random num cache */
#define CACHE_SIZE_BYTES	SEED_SIZE_BYTES * 32

/*
 * Static global variables
 */

/* Are our data structures inited yet? */
static int FIPS_rand_inited = 0;

/* Spin lock for generating random data */
static spinlock_t rand_lock ____cacheline_aligned_in_smp;

/* Amount of cached random data available */
static unsigned int bytesLeft = 0;

/*
 * Links directly into Kernel AES crypto module.
 */
extern void rand_aes_encrypt(const struct crypto_aes_ctx *ctx, u8 *out,
			  const u8 *in);
extern int rand_aes_set_key(struct crypto_aes_ctx *ctx, const u8 *in_key,
			 unsigned int key_len);

/* 
 * Kernel block cipher for AES with 128-bit key for internal KERNEL use only
 * when using get_random_bytes(), until the crypto module has been loaded.
 */
static struct crypto_aes_ctx aes_ctx_internal;
static int aes_ctx_internal_in_use;

/* Kernel block cipher for AES with 128-bit key */
static struct crypto_blkcipher *aes_blkcipher = NULL;

/* V from "ANSI X9.31 Appendix A.2.4 Using AES" */
static u8 *FIPS_seedp;
static u8 *FIPS_datetimep;
static u8 *FIPS_resultp;
static u8 *FIPS_intermediatep;
static unsigned char been_seeded;

void add_input_randomness(unsigned int type, unsigned int code,
				 unsigned int value)
{
}
EXPORT_SYMBOL_GPL(add_input_randomness);

void add_interrupt_randomness(int irq, int irq_flags)
{
}

void add_device_randomness(const void *buf, unsigned int size)
{
}
EXPORT_SYMBOL(add_device_randomness);

#ifdef CONFIG_BLOCK
void add_disk_randomness(struct gendisk *disk)
{
}
#endif

/* ==================== Generation of random data ==================== */
static void get_datetime_vector(u8 *buf)
{
	struct timeval tv;
	struct timespec ts;
	int i;

	ts = CURRENT_TIME;
	buf[0] = (u8) (ts.tv_sec & 0xff);
	buf[1] = (u8) ((ts.tv_sec >> 8) & 0xff);
	buf[2] = (u8) ((ts.tv_sec >> 16) & 0xff);
	buf[3] = (u8) ((ts.tv_sec >> 24) & 0xff);

	buf[4] = (u8) (ts.tv_nsec & 0xff);
	buf[5] = (u8) ((ts.tv_nsec >> 8) & 0xff);
	buf[6] = (u8) ((ts.tv_nsec >> 16) & 0xff);
	buf[7] = (u8) ((ts.tv_nsec >> 24) & 0xff);

	if (been_seeded) {
		do_gettimeofday(&tv);
		buf[8] = (u8) (tv.tv_sec & 0xff);
		buf[9] = (u8) ((tv.tv_sec >> 8) & 0xff);
		buf[10] = (u8) ((tv.tv_sec >> 16) & 0xff);
		buf[11] = (u8) ((tv.tv_sec >> 24) & 0xff);

		buf[12] = (u8) (tv.tv_usec & 0xff);
		buf[13] = (u8) ((tv.tv_usec >> 8) & 0xff);
		buf[14] = (u8) ((tv.tv_usec >> 16) & 0xff);
		buf[15] = (u8) ((tv.tv_usec >> 24) & 0xff);
	} else {
		for (i=8; i<=15; i++)
			buf[i] = (u8) (i ^ 0x55);
	}
}

/*
 * Assumption: size of dst is at least slen.
 */
static int rand_encrypt(struct crypto_blkcipher *aes_cipher,
			const u8 *src, u8 *dst, unsigned int slen)
{
	struct scatterlist sg_src, sg_dst;
	struct blkcipher_desc aes_desc = {0};
	int ret;

	if (aes_cipher != NULL)
	{
		sg_init_one(&sg_src, src, slen);
		sg_init_one(&sg_dst, dst, slen);

		memset(&aes_desc, 0, sizeof(aes_desc));
		aes_desc.tfm = aes_cipher;

		crypto_blkcipher_clear_flags(aes_cipher, ~0);
		ret = crypto_blkcipher_encrypt(&aes_desc, &sg_dst, &sg_src, slen);
		if (ret)
		{
			printk(KERN_CRIT "crypto_blkcipher_encrypt failed, flags=0x%x\n",
					aes_desc.flags);
			return ret;
		}
	}
	else
	{
		if (slen % AES_BLOCK_SIZE_BYTES)
		{
			return -EINVAL;
		}

		while (slen)
		{
			rand_aes_encrypt(&aes_ctx_internal, dst, src);
			src += AES_BLOCK_SIZE_BYTES;
			dst += AES_BLOCK_SIZE_BYTES;
			slen -= AES_BLOCK_SIZE_BYTES;
		}
	}

	return 0;
}

static void FIPS_gen_rand_one_block(struct crypto_blkcipher *aes_cipher,
				u8 *result, u8 *datetime, u8 *seed)
{
	int i;
	unsigned int ret, len = SEED_SIZE_BYTES;

	/*
	 * Step 1: I = ede*K(DT)
	 * 	I = intermediate
	 * 	DT = datetime
	 */
	ret = rand_encrypt(aes_cipher, datetime, FIPS_intermediatep, len);
	if (ret)
	{
		printk(KERN_CRIT "%s: step 1 failed\n", __FUNCTION__);
		return;
	}

	/*
	 * Step 2: R = ede*K(I XOR V)
	 *  I = intermediate
	 *  V = seed
	 *  R = result
	 */
	for (i = 0; i < len; ++i)
	{
		seed[i] ^= FIPS_intermediatep[i];
	}

	ret = rand_encrypt(aes_cipher, seed, result, len);
	if (ret)
	{
		printk(KERN_CRIT "%s: step 2 failed\n", __FUNCTION__);
		return;
	}

	/*
	 * Step 3: V = ede*K(R XOR I)
	 *  I = intermediate
	 *  V = seed
	 *  R = result
	 */
	for (i = 0; i < len; ++i)
	{
		FIPS_intermediatep[i] ^= result[i];
	}

	ret = rand_encrypt(aes_cipher, FIPS_intermediatep, seed, len);
	if (ret)
	{
		printk(KERN_CRIT "%s: step 3 failed\n", __FUNCTION__);
		return;
	}
}

static void FIPS_gen_rand(void *buf, int nbytes)
{
	/* 
	 * Test vector for 'Continuous random number generator test' from
	 * FIPS 140-2, Section 4.9.2.
	 */
	static u8 testVector[SEED_SIZE_BYTES];
	static unsigned char done_first = 0;
	int n;

	while (nbytes > 0)
	{
		get_datetime_vector(FIPS_datetimep);

		FIPS_gen_rand_one_block(aes_blkcipher, FIPS_resultp, FIPS_datetimep, FIPS_seedp);

		/* Never let the first block escape */
		if (done_first) {
			/* Perform 'Continuous random number generator test' from
			 * FIPS 140-2, Section 4.9.2.
			 */
			if (memcmp(FIPS_resultp, testVector, sizeof(testVector)) == 0)
			{
				printk(KERN_CRIT "%s: RNG has stopped working!\n", __FUNCTION__);
				return;
			}

			/* Successive values of R may be concatenated to produce a
			 * pseudo random number of the desired length.
			 */ 
			n = min_t(int, SEED_SIZE_BYTES, nbytes);
			memcpy(buf, FIPS_resultp, n);
			buf += n;
			nbytes -= n;
		} else
			done_first = 1;
		memcpy(testVector, FIPS_resultp, sizeof(testVector));
	}
}

static void FIPS_gen_rand_cached(void *buf, int nbytes)
{
	static u8 cache[CACHE_SIZE_BYTES];

	/* Exhaust cache before generating more random data */
	if (bytesLeft != 0)
	{
		int copyLen = min_t(int, nbytes, bytesLeft);

		memcpy(buf, cache + (sizeof(cache) - bytesLeft), copyLen);

		buf += copyLen;
		nbytes -= copyLen;

		bytesLeft -= copyLen;

		/* If cache had enough bytes, then we are done */
		if (nbytes == 0)
			return;
	}

	/* The cache has been exhausted so first thing to do is re-fill it */
	while (nbytes > 0)
	{
		int copyLen = min_t(int, nbytes, sizeof(cache));

		FIPS_gen_rand(cache, sizeof(cache));

		/* Copy what we need/can */
		memcpy(buf, cache, copyLen);

		/* Update buffer details */
		buf += copyLen;
		nbytes -= copyLen;

		bytesLeft = sizeof(cache) - copyLen;
	}
}

static void FIPS_rand_seed(const u8 *buf, size_t nbytes)
{
	while (nbytes > 0) {
		const size_t n = min_t(size_t, nbytes, SEED_SIZE_BYTES);
		int i;

		for (i=0; i<n; i++)
			FIPS_seedp[i] ^= buf[i];
		nbytes -= n;
		buf += n;
	}
}

static ssize_t FIPS_gen_rand_user(void __user *buf, size_t nbytes)
{
	ssize_t ret = 0, i;
	__u8 tmp[SEED_SIZE_BYTES];
	unsigned long flags;

	if (nbytes == 0)
		return 0;

	while (nbytes) {
		if (need_resched()) {
			if (signal_pending(current)) {
				if (ret == 0)
					ret = -ERESTARTSYS;
				break;
			}
			schedule();
		}

		i = min_t(int, nbytes, sizeof(tmp));
		spin_lock_irqsave(&rand_lock, flags);
		FIPS_gen_rand_cached(tmp, i);
		spin_unlock_irqrestore(&rand_lock, flags);
		if (copy_to_user(buf, tmp, i)) {
			/* Wipe data just returned from memory */
			memset(tmp, 0, sizeof(tmp));
			ret = -EFAULT;
			break;
		}

		/* Wipe data just returned from memory */
		memset(tmp, 0, sizeof(tmp));

		nbytes -= i;
		buf += i;
		ret += i;
	}

	return ret;
}

static void get_aes_key(u8 *key)
{

	if (aes_blkcipher == NULL) {
		/* Initialise to something very non-random.
		 * This is okay because we're properly seeded at init time
		 * and the stuff we really care about gets seeded from us
		 * after this.
		 */
		const char c = 55;
		memset(key, c, SEED_SIZE_BYTES);
	} else
		FIPS_gen_rand_cached(key, KEY_SIZE_BYTES);
}

static void set_initial_seed(void)
{
	u8 tmp[SEED_SIZE_BYTES];

	get_aes_key(tmp);

	FIPS_rand_seed(tmp, sizeof(tmp));
	memset(tmp, 0, sizeof(tmp));
}

static void FIPS_rand_init(void)
{
	spin_lock_init(&rand_lock);
	FIPS_seedp = kmalloc(SEED_SIZE_BYTES, GFP_KERNEL);
	FIPS_datetimep = kmalloc(SEED_SIZE_BYTES, GFP_KERNEL);
	FIPS_resultp = kmalloc(SEED_SIZE_BYTES, GFP_KERNEL);
	FIPS_intermediatep = kmalloc(SEED_SIZE_BYTES, GFP_KERNEL);
	FIPS_rand_inited = 1;
}

/*
 * This function is the exported kernel interface.  It returns some
 * number of good random numbers, suitable for seeding TCP sequence
 * numbers, etc.
 */
void get_random_bytes(void *buf, int nbytes)
{
	unsigned long flags;
	int ret = 0;

	if (!FIPS_rand_inited)
		FIPS_rand_init();

	spin_lock_irqsave(&rand_lock, flags);

	if (aes_blkcipher == NULL)
	{
		/* Use our own tfm, as crypto module has NOT been loaded yet */
		if (!aes_ctx_internal_in_use)
		{
			u8 key[KEY_SIZE_BYTES];

			/* Set the seed */
			set_initial_seed();

			/* Set the key */
			get_aes_key(key);
			ret = rand_aes_set_key(&aes_ctx_internal, key, sizeof(key));

			if (ret)
			{
				panic("Failed to initialize RNG: %d", -ret);
			}

			aes_ctx_internal_in_use = 1;
		}
	}

	FIPS_gen_rand_cached(buf, nbytes);

	spin_unlock_irqrestore(&rand_lock, flags);
}
EXPORT_SYMBOL(get_random_bytes);

static int rand_initialize(void)
{
	int ret = 0;
	u8 key[KEY_SIZE_BYTES];
	unsigned long flags;
	
	if (!FIPS_rand_inited)
		FIPS_rand_init();

	spin_lock_irqsave(&rand_lock, flags);

	/* Set the seed */
	set_initial_seed();

	/* Create initial key for RNG */
	get_aes_key(key);

	aes_blkcipher = crypto_alloc_blkcipher("ecb(aes)", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(aes_blkcipher))
	{
		printk(KERN_ERR "failed to load cipher for 'ecb(aes)': %ld\n",
		       PTR_ERR(aes_blkcipher));

		ret = -EFAULT;
		goto error;
	}

	ret = crypto_blkcipher_setkey(aes_blkcipher, key, sizeof(key));
	if (ret)
	{
		printk(KERN_ERR "setkey() failed flags=%x\n",
				crypto_blkcipher_get_flags(aes_blkcipher));
		crypto_free_blkcipher(aes_blkcipher);
		aes_blkcipher = NULL;

		ret = -EKEYREJECTED;
		goto error;
	}

	/* No longer need internal AES crypto */
	if (aes_ctx_internal_in_use)
	{
		memset(&aes_ctx_internal, 0, sizeof(aes_ctx_internal));
		aes_ctx_internal_in_use = 0;
	}

	/* Reset cache to be empty, as we are changing RNG generator */
	bytesLeft = 0;

error:
	spin_unlock_irqrestore(&rand_lock, flags);
	/* Wipe key from memory */
	memset(key, 0, sizeof(key));

	return ret;
}
module_init(rand_initialize);

void rand_initialize_irq(int irq)
{
}

#ifdef CONFIG_BLOCK
void rand_initialize_disk(struct gendisk *disk)
{
}
#endif

static ssize_t
_random_read(char __user *buf, size_t nbytes)
{
	ssize_t n;

	n = FIPS_gen_rand_user(buf, nbytes);
	return n;
}

static ssize_t
random_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{
	ssize_t n;

	n = _random_read(buf, nbytes);

	/*
	 * If we gave the user some bytes, update the access time.
	 */
	if (n > 0)
		file_accessed(file);

	return n;
}

static ssize_t
urandom_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{
	return FIPS_gen_rand_user(buf, nbytes);
}

static unsigned int random_poll(struct file *file, poll_table * wait)
{
	/* Always available to read or write */
	return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
}

static int
write_pool(const char __user *buffer, size_t count)
{
	size_t bytes;
	u8 buf[SEED_SIZE_BYTES];
	const char __user *p = buffer;
	unsigned long flags;

	while (count > 0) {
		bytes = min(count, sizeof(buf));
		if (copy_from_user(&buf, p, bytes))
			return -EFAULT;

		count -= bytes;
		p += bytes;

		spin_lock_irqsave(&rand_lock, flags);
		FIPS_rand_seed(buf, bytes);
		spin_unlock_irqrestore(&rand_lock, flags);
	}

	/* Now iterate the RNG once to spice things up
	 * Well to purge the first block.
	 */
	spin_lock_irqsave(&rand_lock, flags);
	FIPS_gen_rand(buf, sizeof(buf));
	memset(buf, 0, sizeof(buf));
	spin_unlock_irqrestore(&rand_lock, flags);

	if (! been_seeded) {
		u32 ent32;

		been_seeded = 1;
		bytesLeft = 0;		/* Destroy any unrandom cache we might have */
		printk(KERN_INFO "FIPS_random: seeding...\n");
		get_random_bytes(&ent32, sizeof(ent32));
		prandom_seed(ent32);
	}

	return 0;
}

static ssize_t random_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *ppos)
{
	size_t ret;
	struct inode *inode = file->f_path.dentry->d_inode;

	ret = write_pool(buffer, count);
	if (ret)
		return ret;

	inode->i_mtime = current_fs_time(inode->i_sb);
	mark_inode_dirty(inode);
	return (ssize_t)count;
}

static ssize_t FIPS_test(unsigned char __user *arg, int iterations)
{
	struct crypto_blkcipher *test_aes_blkcipher = NULL;
	struct rand_fips_test rnd_test;
	int ret = 0;

	if (copy_from_user(&rnd_test, arg, sizeof(rnd_test)))
		return -EFAULT;

	test_aes_blkcipher = crypto_alloc_blkcipher("ecb(aes)", 0,
								CRYPTO_ALG_ASYNC);
	if (IS_ERR(test_aes_blkcipher))
	{
		printk(KERN_ERR "%s: failed to load cipher for 'ecb(aes)': %ld\n", __FUNCTION__,
		       PTR_ERR(test_aes_blkcipher));

		return -EFAULT;
	}

	ret = crypto_blkcipher_setkey(test_aes_blkcipher, rnd_test.key,
				sizeof(rnd_test.key));
	if (ret)
	{
		printk(KERN_ERR "%s: setkey() failed flags=%x\n", __FUNCTION__,
				crypto_blkcipher_get_flags(test_aes_blkcipher));

		ret = -EKEYREJECTED;
		goto error;
	}

	while (iterations-- > 0)
	{
		FIPS_gen_rand_one_block(test_aes_blkcipher, rnd_test.result,
			rnd_test.datetime, rnd_test.seed);

		if (iterations > 0)
		{
			int index = sizeof(rnd_test.datetime) - 1;

			/* Increment date/time vector by 1 for each iteration */
			rnd_test.datetime[index]++;
			/* An overflow occurred if new byte value is 0x00 */
			while (rnd_test.datetime[index] == 0x00 && index > 0)
			{
				rnd_test.datetime[--index]++;
			}
			/* Give other processes a chance at doing stuff */
			yield();
		}
	}

	if (copy_to_user(arg, &rnd_test, sizeof(rnd_test)))
		ret = -EFAULT;

error:
	crypto_free_blkcipher(test_aes_blkcipher);

	return ret;
}

static ssize_t FIPS_var_seed_test(unsigned char __user *arg)
{
	return FIPS_test(arg, 1);
}

static ssize_t FIPS_monte_carlo_test(unsigned char __user *arg)
{
	return FIPS_test(arg, 10000);
}

static long random_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	int size, ent_count;
	int __user *p = (int __user *)arg;
	int retval;

	switch (cmd) {
	case RNDGETENTCNT:
		ent_count = INT_MAX;
		if (put_user(ent_count, p))
			return -EFAULT;
		return 0;
	case RNDADDTOENTCNT:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		if (get_user(ent_count, p))
			return -EFAULT;
		return 0;
	case RNDADDENTROPY:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		if (get_user(ent_count, p++))
			return -EFAULT;
		if (ent_count < 0)
			return -EINVAL;
		if (get_user(size, p++))
			return -EFAULT;
		retval = write_pool((const char __user *)p, size);
		if (retval < 0)
			return retval;
		return 0;
	case RNDZAPENTCNT:
	case RNDCLEARPOOL:
		/* Clear the entropy pool counters. */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		return 0;
	case RNDFIPSVST:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		retval = FIPS_var_seed_test((unsigned char __user *)arg);
		if (retval < 0)
			return retval;
		return 0;
	case RNDFIPSMCT:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		retval = FIPS_monte_carlo_test((unsigned char __user *)arg);
		if (retval < 0)
			return retval;
		return 0;
	default:
		return -EINVAL;
	}
}

const struct file_operations random_fops = {
	.read  = random_read,
	.write = random_write,
	.poll  = random_poll,
	.unlocked_ioctl = random_ioctl,
};

const struct file_operations urandom_fops = {
	.read  = urandom_read,
	.write = random_write,
	.unlocked_ioctl = random_ioctl,
};

SYSCALL_DEFINE3(getrandom, char __user *, buf, size_t, count,
		unsigned int, flags)
{
	if (flags & ~(GRND_NONBLOCK|GRND_RANDOM))
		return -EINVAL;

	if (count > INT_MAX)
		count = INT_MAX;

	if (flags & GRND_RANDOM)
		return _random_read(buf, count);

#if 0
	if (unlikely(nonblocking_pool.initialized == 0)) {
		if (flags & GRND_NONBLOCK)
			return -EAGAIN;
		wait_event_interruptible(urandom_init_wait,
					 nonblocking_pool.initialized);
		if (signal_pending(current))
			return -ERESTARTSYS;
	}
#endif
	return urandom_read(NULL, buf, count, NULL);
}

/***************************************************************
 * Random UUID interface
 *
 * Used here for a Boot ID, but can be useful for other kernel
 * drivers.
 ***************************************************************/

/*
 * Generate random UUID
 */
void generate_random_uuid(unsigned char uuid_out[16])
{
	get_random_bytes(uuid_out, 16);
	/* Set UUID version to 4 --- truely random generation */
	uuid_out[6] = (uuid_out[6] & 0x0F) | 0x40;
	/* Set the UUID variant to DCE */
	uuid_out[8] = (uuid_out[8] & 0x3F) | 0x80;
}
EXPORT_SYMBOL(generate_random_uuid);

/********************************************************************
 *
 * Sysctl interface
 *
 ********************************************************************/

#ifdef CONFIG_SYSCTL

#include <linux/sysctl.h>

static char sysctl_bootid[16];

/*
 * These functions is used to return both the bootid UUID, and random
 * UUID.  The difference is in whether table->data is NULL; if it is,
 * then a new UUID is generated and returned to the user.
 *
 * If the user accesses this via the proc interface, it will be returned
 * as an ASCII string in the standard UUID format.  If accesses via the
 * sysctl system call, it is returned as 16 bytes of binary data.
 */
static int proc_do_uuid(struct ctl_table *table, int write,
			void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct ctl_table fake_table;
	unsigned char buf[64], tmp_uuid[16], *uuid;

	uuid = table->data;
	if (!uuid) {
		uuid = tmp_uuid;
		uuid[8] = 0;
	}
	if (uuid[8] == 0)
		generate_random_uuid(uuid);

	sprintf(buf, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
		"%02x%02x%02x%02x%02x%02x",
		uuid[0],  uuid[1],  uuid[2],  uuid[3],
		uuid[4],  uuid[5],  uuid[6],  uuid[7],
		uuid[8],  uuid[9],  uuid[10], uuid[11],
		uuid[12], uuid[13], uuid[14], uuid[15]);
	fake_table.data = buf;
	fake_table.maxlen = sizeof(buf);

	return proc_dostring(&fake_table, write, buffer, lenp, ppos);
}

struct ctl_table random_table[] = {
	{
		.procname	= "boot_id",
		.data		= &sysctl_bootid,
		.maxlen		= 16,
		.mode		= 0444,
		.proc_handler	= &proc_do_uuid,
	},
	{
		.procname	= "uuid",
		.maxlen		= 16,
		.mode		= 0444,
		.proc_handler	= &proc_do_uuid,
	},
	{ }
};
#endif 	/* CONFIG_SYSCTL */

/********************************************************************
 *
 * Random funtions for networking
 *
 ********************************************************************/

/* This should not be decreased so low that ISNs wrap too fast. */
#define REKEY_INTERVAL (300 * HZ)
/*
 * Bit layout of the tcp sequence numbers (before adding current time):
 * bit 24-31: increased after every key exchange
 * bit 0-23: hash(source,dest)
 *
 * The implementation is similar to the algorithm described
 * in the Appendix of RFC 1185, except that
 * - it uses a 1 MHz clock instead of a 250 kHz clock
 * - it performs a rekey every 5 minutes, which is equivalent
 * 	to a (source,dest) tulple dependent forward jump of the
 * 	clock by 0..2^(HASH_BITS+1)
 *
 * Thus the average ISN wraparound time is 68 minutes instead of
 * 4.55 hours.
 *
 * SMP cleanup and lock avoidance with poor man's RCU.
 * 			Manfred Spraul <manfred@colorfullife.com>
 *
 */
#define COUNT_BITS 8
#define COUNT_MASK ((1 << COUNT_BITS) - 1)
#define HASH_BITS 24
#define HASH_MASK ((1 << HASH_BITS) - 1)

static struct keydata {
	__u32 count; /* already shifted to the final position */
	__u32 secret[12];
} ____cacheline_aligned ip_keydata[2];

static unsigned int ip_cnt;

static void rekey_seq_generator(struct work_struct *work);

static DECLARE_DELAYED_WORK(rekey_work, rekey_seq_generator);

/*
 * Lock avoidance:
 * The ISN generation runs lockless - it's just a hash over random data.
 * State changes happen every 5 minutes when the random key is replaced.
 * Synchronization is performed by having two copies of the hash function
 * state and rekey_seq_generator always updates the inactive copy.
 * The copy is then activated by updating ip_cnt.
 * The implementation breaks down if someone blocks the thread
 * that processes SYN requests for more than 5 minutes. Should never
 * happen, and even if that happens only a not perfectly compliant
 * ISN is generated, nothing fatal.
 */
static void rekey_seq_generator(struct work_struct *work)
{
	struct keydata *keyptr = &ip_keydata[1 ^ (ip_cnt & 1)];

	get_random_bytes(keyptr->secret, sizeof(keyptr->secret));
	keyptr->count = (ip_cnt & COUNT_MASK) << HASH_BITS;
	smp_wmb();
	ip_cnt++;
	schedule_delayed_work(&rekey_work, REKEY_INTERVAL);
}

static inline struct keydata *get_keyptr(void)
{
	struct keydata *keyptr = &ip_keydata[ip_cnt & 1];

	smp_rmb();

	return keyptr;
}

static __init int seqgen_init(void)
{
	rekey_seq_generator(NULL);
	return 0;
}
late_initcall(seqgen_init);

static u32 random_int_secret[MD5_MESSAGE_BYTES / 4] ____cacheline_aligned;

int __init random_int_secret_init(void)
{
	get_random_bytes(random_int_secret, sizeof(random_int_secret));
	return 0;
}
late_initcall(random_int_secret_init);

/*
 * Get a random word for internal kernel use only. Similar to urandom but
 * with the goal of minimal entropy pool depletion. As a result, the random
 * value is not cryptographically secure but for several uses the cost of
 * depleting entropy is too high
 */
DEFINE_PER_CPU(__u32 [MD5_DIGEST_WORDS], get_random_int_hash);
unsigned int get_random_int(void)
{
	__u32 *hash = get_cpu_var(get_random_int_hash);
	unsigned int ret;

	hash[0] += current->pid + jiffies + get_cycles();
	md5_transform(hash, random_int_secret);
	ret = hash[0];
	put_cpu_var(get_random_int_hash);

	return ret;
}

/*
 * randomize_range() returns a start address such that
 *
 *    [...... <range> .....]
 *  start                  end
 *
 * a <range> with size "len" starting at the return value is inside in the
 * area defined by [start, end], but is otherwise randomized.
 */
unsigned long
randomize_range(unsigned long start, unsigned long end, unsigned long len)
{
	unsigned long range = end - len - start;

	if (end <= start + len)
		return 0;
	return PAGE_ALIGN(get_random_int() % range + start);
}

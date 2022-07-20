/*
 * $Id: radix.h,v 1.12 2001/11/13 19:24:34 hno Exp $
 */

#ifndef SQUID_RADIX_H
#define	SQUID_RADIX_H

/*
 * Copyright (c) 1988, 1989, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)radix.h     8.2 (Berkeley) 10/31/94
 */

#undef RN_DEBUG
/*
 * Radix search tree node layout.
 */

struct squid_radix_node {
    struct squid_radix_mask *rn_mklist;	/* list of masks contained in subtree */
    struct squid_radix_node *rn_p;	/* parent */
    short rn_b;			/* bit offset; -1-index(netmask) */
    char rn_bmask;		/* node: mask for bit test */
    unsigned char rn_flags;	/* enumerated next */
#define RNF_NORMAL	1	/* leaf contains normal route */
#define RNF_ROOT	2	/* leaf is root leaf for tree */
#define RNF_ACTIVE	4	/* This node is alive (for rtfree) */
    union {
	struct {		/* leaf only data: */
	    char *rn_Key;	/* object of search */
	    char *rn_Mask;	/* netmask, if present */
	    struct squid_radix_node *rn_Dupedkey;
	} rn_leaf;
	struct {		/* node only data: */
	    int rn_Off;		/* where to start compare */
	    struct squid_radix_node *rn_L;	/* progeny */
	    struct squid_radix_node *rn_R;	/* progeny */
	} rn_node;
    } rn_u;
#ifdef RN_DEBUG
    int rn_info;
    struct squid_radix_node *rn_twin;
    struct squid_radix_node *rn_ybro;
#endif
};

#define rn_dupedkey rn_u.rn_leaf.rn_Dupedkey
#define rn_key rn_u.rn_leaf.rn_Key
#define rn_mask rn_u.rn_leaf.rn_Mask
#define rn_off rn_u.rn_node.rn_Off
#define rn_l rn_u.rn_node.rn_L
#define rn_r rn_u.rn_node.rn_R

/*
 * Annotations to tree concerning potential routes applying to subtrees.
 */

extern struct squid_radix_mask {
    short rm_b;			/* bit offset; -1-index(netmask) */
    char rm_unused;		/* cf. rn_bmask */
    unsigned char rm_flags;	/* cf. rn_flags */
    struct squid_radix_mask *rm_mklist;	/* more masks to try */
    union {
	char *rmu_mask;		/* the mask */
	struct squid_radix_node *rmu_leaf;	/* for normal routes */
    } rm_rmu;
    int rm_refs;		/* # of references to this struct */
}         *squid_rn_mkfreelist;

#define rm_mask rm_rmu.rmu_mask
#define rm_leaf rm_rmu.rmu_leaf	/* extra field would make 32 bytes */

#define squid_MKGet(m) {\
	if (squid_rn_mkfreelist) {\
		m = squid_rn_mkfreelist; \
		squid_rn_mkfreelist = (m)->rm_mklist; \
	} else \
		squid_R_Malloc(m, struct squid_radix_mask *, sizeof (*(m))); }\

#define squid_MKFree(m) { (m)->rm_mklist = squid_rn_mkfreelist; squid_rn_mkfreelist = (m);}

struct squid_radix_node_head {
    struct squid_radix_node *rnh_treetop;
    int rnh_addrsize;		/* permit, but not require fixed keys */
    int rnh_pktsize;		/* permit, but not require fixed keys */
    struct squid_radix_node *(*rnh_addaddr)	/* add based on sockaddr */
               (void *v, void *mask,
	    struct squid_radix_node_head * head, struct squid_radix_node nodes[]);
    struct squid_radix_node *(*rnh_addpkt)	/* add based on packet hdr */
               (void *v, void *mask,
	    struct squid_radix_node_head * head, struct squid_radix_node nodes[]);
    struct squid_radix_node *(*rnh_deladdr)	/* remove based on sockaddr */
               (void *v, void *mask, struct squid_radix_node_head * head);
    struct squid_radix_node *(*rnh_delpkt)	/* remove based on packet hdr */
               (void *v, void *mask, struct squid_radix_node_head * head);
    struct squid_radix_node *(*rnh_matchaddr)		/* locate based on sockaddr */
               (void *v, struct squid_radix_node_head * head);
    struct squid_radix_node *(*rnh_lookup)	/* locate based on sockaddr */
               (void *v, void *mask, struct squid_radix_node_head * head);
    struct squid_radix_node *(*rnh_matchpkt)	/* locate based on packet hdr */
               (void *v, struct squid_radix_node_head * head);
    int (*rnh_walktree)		/* traverse tree */
        (struct squid_radix_node_head * head, int (*f) (struct squid_radix_node *, void *), void *w);
    struct squid_radix_node rnh_nodes[3];	/* empty tree for common case */
};


#define squid_Bcmp(a, b, n) memcmp(((char *)(a)), ((char *)(b)), (n))
#define squid_Bcopy(a, b, n) memcpy(((char *)(b)), ((char *)(a)), (unsigned)(n))
#define squid_Bzero(p, n) memset((char *)(p),'\0', (int)(n))
#define squid_R_Malloc(p, t, n) (p = (t) xmalloc((unsigned int)(n)))
#define squid_Free(p) xfree((char *)p)

extern void squid_rn_init (void);
extern int squid_rn_inithead(void **, int);
extern int squid_rn_refines(void *, void *);
extern int squid_rn_walktree(struct squid_radix_node_head *, int (*)(struct squid_radix_node *, void *), void *);
extern struct squid_radix_node *squid_rn_addmask(void *, int, int);
extern struct squid_radix_node *squid_rn_addroute(void *, void *, struct squid_radix_node_head *, struct squid_radix_node[2]);
extern struct squid_radix_node *squid_rn_delete(void *, void *, struct squid_radix_node_head *);
extern struct squid_radix_node *squid_rn_insert(void *, struct squid_radix_node_head *, int *, struct squid_radix_node[2]);
extern struct squid_radix_node *squid_rn_match(void *, struct squid_radix_node_head *);
extern struct squid_radix_node *squid_rn_newpair(void *, int, struct squid_radix_node[2]);
extern struct squid_radix_node *squid_rn_search(void *, struct squid_radix_node *);
extern struct squid_radix_node *squid_rn_search_m(void *, struct squid_radix_node *, void *);
extern struct squid_radix_node *squid_rn_lookup(void *, void *, struct squid_radix_node_head *);
#define min(x,y) ((x)<(y)? (x) : (y))

#endif /* SQUID_RADIX_H */

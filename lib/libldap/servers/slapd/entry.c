/* entry.c - routines for dealing with entries */
/* $OpenLDAP: pkg/ldap/servers/slapd/entry.c,v 1.90.2.10 2003/05/07 22:29:10 hyc Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "ldif.h"

static unsigned char	*ebuf;	/* buf returned by entry2str		 */
static unsigned char	*ecur;	/* pointer to end of currently used ebuf */
static int		emaxsize;/* max size of ebuf			 */

/*
 * Empty root entry
 */
const Entry slap_entry_root = { NOID, { 0, "" }, { 0, "" }, NULL, 0, { 0, "" }, NULL };

int entry_destroy(void)
{
	if ( ebuf ) free( ebuf );
	ebuf = NULL;
	ecur = NULL;
	emaxsize = 0;
	return 0;
}


Entry *
str2entry( char *s )
{
	int rc;
	Entry		*e;
	char		*type;
	struct berval	vals[2];
	AttributeDescription *ad;
	const char *text;
	char	*next;

	/*
	 * LDIF is used as the string format.
	 * An entry looks like this:
	 *
	 *	dn: <dn>\n
	 *	[<attr>:[:] <value>\n]
	 *	[<tab><continuedvalue>\n]*
	 *	...
	 *
	 * If a double colon is used after a type, it means the
	 * following value is encoded as a base 64 string.  This
	 * happens if the value contains a non-printing character
	 * or newline.
	 */

#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, DETAIL1, "str2entry: \"%s\"\n", s ? s : "NULL", 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "=> str2entry\n",
		s ? s : "NULL", 0, 0 );
#endif

	/* initialize reader/writer lock */
	e = (Entry *) ch_calloc( 1, sizeof(Entry) );

	if( e == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, ERR, "str2entry: entry allocation failed.\n", 0, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
		    "<= str2entry NULL (entry allocation failed)\n",
		    0, 0, 0 );
#endif
		return( NULL );
	}

	/* initialize entry */
	e->e_id = NOID;

	/* dn + attributes */
	vals[0].bv_len = 0;
	vals[1].bv_val = NULL;

	next = s;
	while ( (s = ldif_getline( &next )) != NULL ) {
		if ( *s == '\n' || *s == '\0' ) {
			break;
		}

		if ( ldif_parse_line( s, &type, &vals[0].bv_val, &vals[0].bv_len ) != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, DETAIL1, "str2entry:  NULL (parse_line)\n",0, 0, 0 );
#else
			Debug( LDAP_DEBUG_TRACE,
			    "<= str2entry NULL (parse_line)\n", 0, 0, 0 );
#endif
			continue;
		}

		if ( strcasecmp( type, "dn" ) == 0 ) {
			free( type );

			if ( e->e_dn != NULL ) {
#ifdef NEW_LOGGING
				LDAP_LOG( OPERATION, DETAIL1, "str2entry: "
					"entry %ld has multiple DNs \"%s\" and \"%s\"\n",
					(long) e->e_id, e->e_dn, vals[0].bv_val );
#else
				Debug( LDAP_DEBUG_ANY, "str2entry: "
					"entry %ld has multiple DNs \"%s\" and \"%s\"\n",
				    (long) e->e_id, e->e_dn, vals[0].bv_val );
#endif
				free( vals[0].bv_val );
				entry_free( e );
				return NULL;
			}

			rc = dnPrettyNormal( NULL, &vals[0], &e->e_name, &e->e_nname );
			if( rc != LDAP_SUCCESS ) {
#ifdef NEW_LOGGING
				LDAP_LOG( OPERATION, DETAIL1, 
					"str2entry: entry %ld has invalid DN \"%s\"\n",
					(long) e->e_id, vals[0].bv_val, 0 );
#else
				Debug( LDAP_DEBUG_ANY, "str2entry: "
					"entry %ld has invalid DN \"%s\"\n",
					(long) e->e_id, vals[0].bv_val, 0 );
#endif
				entry_free( e );
				free( vals[0].bv_val );
				return NULL;
			}
			free( vals[0].bv_val );
			continue;
		}

		ad = NULL;
		rc = slap_str2ad( type, &ad, &text );

		if( rc != LDAP_SUCCESS ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, DETAIL1, 
				"str2entry:  str2ad(%s): %s\n", type, text, 0 );
#else
			Debug( slapMode & SLAP_TOOL_MODE
				? LDAP_DEBUG_ANY : LDAP_DEBUG_TRACE,
				"<= str2entry: str2ad(%s): %s\n", type, text, 0 );
#endif
			if( slapMode & SLAP_TOOL_MODE ) {
				entry_free( e );
				free( vals[0].bv_val );
				free( type );
				return NULL;
			}

			rc = slap_str2undef_ad( type, &ad, &text );
			if( rc != LDAP_SUCCESS ) {
#ifdef NEW_LOGGING
				LDAP_LOG( OPERATION, DETAIL1, 
					"str2entry: str2undef_ad(%s): %s\n", type, text, 0 );
#else
				Debug( LDAP_DEBUG_ANY,
					"<= str2entry: str2undef_ad(%s): %s\n",
						type, text, 0 );
#endif
				entry_free( e );
				free( vals[0].bv_val );
				free( type );
				return NULL;
			}
		}

		if( slapMode & SLAP_TOOL_MODE ) {
			struct berval pval;
			slap_syntax_validate_func *validate =
				ad->ad_type->sat_syntax->ssyn_validate;
			slap_syntax_transform_func *pretty =
				ad->ad_type->sat_syntax->ssyn_pretty;

			if( pretty ) {
				rc = pretty( ad->ad_type->sat_syntax,
					&vals[0], &pval );

			} else if( validate ) {
				/*
			 	 * validate value per syntax
			 	 */
				rc = validate( ad->ad_type->sat_syntax, &vals[0] );

			} else {
#ifdef NEW_LOGGING
				LDAP_LOG( OPERATION, INFO, 
					"str2entry: no validator for syntax %s\n", 
					ad->ad_type->sat_syntax->ssyn_oid, 0, 0 );
#else
				Debug( LDAP_DEBUG_ANY,
					"str2entry: no validator for syntax %s\n",
					ad->ad_type->sat_syntax->ssyn_oid, 0, 0 );
#endif
				entry_free( e );
				free( vals[0].bv_val );
				free( type );
				return NULL;
			}

			if( rc != 0 ) {
#ifdef NEW_LOGGING
				LDAP_LOG( OPERATION, ERR, 
					"str2entry:  invalid value for syntax %s\n",
					ad->ad_type->sat_syntax->ssyn_oid, 0, 0 );
#else
				Debug( LDAP_DEBUG_ANY,
					"str2entry: invalid value for syntax %s\n",
					ad->ad_type->sat_syntax->ssyn_oid, 0, 0 );
#endif
				entry_free( e );
				free( vals[0].bv_val );
				free( type );
				return NULL;
			}

			if( pretty ) {
				free( vals[0].bv_val );
				vals[0] = pval;
			}
		}

		rc = attr_merge( e, ad, vals );
		if( rc != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, DETAIL1,
				"str2entry:  NULL (attr_merge)\n" , 0, 0, 0 );
#else
			Debug( LDAP_DEBUG_ANY,
			    "<= str2entry NULL (attr_merge)\n", 0, 0, 0 );
#endif
			entry_free( e );
			free( vals[0].bv_val );
			free( type );
			return( NULL );
		}

		free( type );
		free( vals[0].bv_val );
	}

	/* check to make sure there was a dn: line */
	if ( e->e_dn == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, 
			"str2entry:  entry %ld has no dn.\n", (long) e->e_id, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "str2entry: entry %ld has no dn\n",
		    (long) e->e_id, 0, 0 );
#endif
		entry_free( e );
		return( NULL );
	}

#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, DETAIL2,
		"str2entry(%s) -> 0x%lx\n", e->e_dn, (unsigned long)e, 0 );
#else
	Debug(LDAP_DEBUG_TRACE, "<= str2entry(%s) -> 0x%lx\n",
		e->e_dn, (unsigned long) e, 0 );
#endif
	return( e );
}


#define GRABSIZE	BUFSIZ

#define MAKE_SPACE( n )	{ \
		while ( ecur + (n) > ebuf + emaxsize ) { \
			ptrdiff_t	offset; \
			offset = (int) (ecur - ebuf); \
			ebuf = (unsigned char *) ch_realloc( (char *) ebuf, \
			    emaxsize + GRABSIZE ); \
			emaxsize += GRABSIZE; \
			ecur = ebuf + offset; \
		} \
	}

char *
entry2str(
    Entry	*e,
    int		*len )
{
	Attribute	*a;
	struct berval	*bv;
	int		i;
	ber_len_t tmplen;

	assert( e != NULL );

	/*
	 * In string format, an entry looks like this:
	 *	dn: <dn>\n
	 *	[<attr>: <value>\n]*
	 */

	ecur = ebuf;

	/* put the dn */
	if ( e->e_dn != NULL ) {
		/* put "dn: <dn>" */
		tmplen = e->e_name.bv_len;
		MAKE_SPACE( LDIF_SIZE_NEEDED( 2, tmplen ));
		ldif_sput( (char **) &ecur, LDIF_PUT_VALUE, "dn", e->e_dn, tmplen );
	}

	/* put the attributes */
	for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
		/* put "<type>:[:] <value>" line for each value */
		for ( i = 0; a->a_vals[i].bv_val != NULL; i++ ) {
			bv = &a->a_vals[i];
			tmplen = a->a_desc->ad_cname.bv_len;
			MAKE_SPACE( LDIF_SIZE_NEEDED( tmplen, bv->bv_len ));
			ldif_sput( (char **) &ecur, LDIF_PUT_VALUE,
				a->a_desc->ad_cname.bv_val,
			    bv->bv_val, bv->bv_len );
		}
	}
	MAKE_SPACE( 1 );
	*ecur = '\0';
	*len = ecur - ebuf;

	return( (char *) ebuf );
}

void
entry_free( Entry *e )
{
	/* free an entry structure */
	assert( e != NULL );

	/* e_private must be freed by the caller */
	assert( e->e_private == NULL );
	e->e_private = NULL;

	/* free DNs */
	if ( e->e_dn != NULL ) {
		free( e->e_dn );
		e->e_dn = NULL;
	}
	if ( e->e_ndn != NULL ) {
		free( e->e_ndn );
		e->e_ndn = NULL;
	}

	if ( e->e_bv.bv_val != NULL ) {
		free( e->e_bv.bv_val );
		e->e_bv.bv_val = NULL;
	}

	/* free attributes */
	attrs_free( e->e_attrs );
	e->e_attrs = NULL;

	free( e );
}

/*
 * These routines are used only by Backend.
 *
 * the Entry has three entry points (ways to find things):
 *
 *	by entry	e.g., if you already have an entry from the cache
 *			and want to delete it. (really by entry ptr)
 *	by dn		e.g., when looking for the base object of a search
 *	by id		e.g., for search candidates
 *
 * these correspond to three different avl trees that are maintained.
 */

int
entry_cmp( Entry *e1, Entry *e2 )
{
	return SLAP_PTRCMP( e1, e2 );
}

int
entry_dn_cmp( const void *v_e1, const void *v_e2 )
{
	/* compare their normalized UPPERCASED dn's */
	const Entry *e1 = v_e1, *e2 = v_e2;

	return ber_bvcmp( &e1->e_nname, &e2->e_nname );
}

int
entry_id_cmp( const void *v_e1, const void *v_e2 )
{
	const Entry *e1 = v_e1, *e2 = v_e2;
	return( e1->e_id < e2->e_id ? -1 : (e1->e_id > e2->e_id ? 1 : 0) );
}

#ifdef SLAPD_BDB

/* This is like a ber_len */
static ber_len_t
entry_lenlen(ber_len_t len)
{
	if (len <= 0x7f)
		return 1;
	if (len <= 0xff)
		return 2;
	if (len <= 0xffff)
		return 3;
	if (len <= 0xffffff)
		return 4;
	return 5;
}

static void
entry_putlen(unsigned char **buf, ber_len_t len)
{
	ber_len_t lenlen = entry_lenlen(len);

	if (lenlen == 1) {
		**buf = (unsigned char) len;
	} else {
		int i;
		**buf = 0x80 | ((unsigned char) lenlen - 1);
		for (i=lenlen-1; i>0; i--) {
			(*buf)[i] = (unsigned char) len;
			len >>= 8;
		}
	}
	*buf += lenlen;
}

static ber_len_t
entry_getlen(unsigned char **buf)
{
	ber_len_t len;
	int i;

	len = *(*buf)++;
	if (len <= 0x7f)
		return len;
	i = len & 0x7f;
	len = 0;
	for (;i > 0; i--) {
		len <<= 8;
		len |= *(*buf)++;
	}
	return len;
}

/* Flatten an Entry into a buffer. The buffer is filled with just the
 * strings/bervals of all the entry components. Each field is preceded
 * by its length, encoded the way ber_put_len works. Every field is NUL
 * terminated.  The entire buffer size is precomputed so that a single
 * malloc can be performed. The entry size is also recorded,
 * to aid in entry_decode.
 */
int entry_encode(Entry *e, struct berval *bv)
{
	ber_len_t siz = sizeof(Entry);
	ber_len_t len, dnlen, ndnlen;
	int i;
	Attribute *a;
	unsigned char *ptr;

#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, DETAIL1, "entry_encode: id: 0x%08lx  \"%s\"\n",
		(long) e->e_id, e->e_dn, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "=> entry_encode(0x%08lx): %s\n",
		(long) e->e_id, e->e_dn, 0 );
#endif
	dnlen = e->e_name.bv_len;
	ndnlen = e->e_nname.bv_len;
	len = dnlen + ndnlen + 2;	/* two trailing NUL bytes */
	len += entry_lenlen(dnlen);
	len += entry_lenlen(ndnlen);
	for (a=e->e_attrs; a; a=a->a_next) {
		/* For AttributeDesc, we only store the attr name */
		siz += sizeof(Attribute);
		len += a->a_desc->ad_cname.bv_len+1;
		len += entry_lenlen(a->a_desc->ad_cname.bv_len);
		for (i=0; a->a_vals[i].bv_val; i++) {
			siz += sizeof(struct berval);
			len += a->a_vals[i].bv_len + 1;
			len += entry_lenlen(a->a_vals[i].bv_len);
		}
		len += entry_lenlen(i);
		siz += sizeof(struct berval);	/* empty berval at end */
	}
	len += 1;	/* NUL byte at end */
	len += entry_lenlen(siz);
	bv->bv_len = len;
	bv->bv_val = ch_malloc(len);
	ptr = (unsigned char *)bv->bv_val;
	entry_putlen(&ptr, siz);
	entry_putlen(&ptr, dnlen);
	AC_MEMCPY(ptr, e->e_dn, dnlen);
	ptr += dnlen;
	*ptr++ = '\0';
	entry_putlen(&ptr, ndnlen);
	AC_MEMCPY(ptr, e->e_ndn, ndnlen);
	ptr += ndnlen;
	*ptr++ = '\0';

	for (a=e->e_attrs; a; a=a->a_next) {
		entry_putlen(&ptr, a->a_desc->ad_cname.bv_len);
		AC_MEMCPY(ptr, a->a_desc->ad_cname.bv_val,
			a->a_desc->ad_cname.bv_len);
		ptr += a->a_desc->ad_cname.bv_len;
		*ptr++ = '\0';
		if (a->a_vals) {
		    for (i=0; a->a_vals[i].bv_val; i++);
		    entry_putlen(&ptr, i);
		    for (i=0; a->a_vals[i].bv_val; i++) {
			entry_putlen(&ptr, a->a_vals[i].bv_len);
			AC_MEMCPY(ptr, a->a_vals[i].bv_val,
				a->a_vals[i].bv_len);
			ptr += a->a_vals[i].bv_len;
			*ptr++ = '\0';
		    }
		}
	}
	*ptr = '\0';
	return 0;
}

/* Retrieve an Entry that was stored using entry_encode above.
 * We malloc a single block with the size stored above for the Entry
 * and all of its Attributes. We also must lookup the stored
 * attribute names to get AttributeDescriptions. To detect if the
 * attributes of an Entry are later modified, we note that e->e_attr
 * is always a constant offset from (e).
 *
 * Note: everything is stored in a single contiguous block, so
 * you can not free individual attributes or names from this
 * structure. Attempting to do so will likely corrupt memory.
 */
int entry_decode(struct berval *bv, Entry **e)
{
	int i, j;
	int rc;
	Attribute *a;
	Entry *x;
	const char *text;
	AttributeDescription *ad;
	unsigned char *ptr = (unsigned char *)bv->bv_val;
	BerVarray bptr;

	i = entry_getlen(&ptr);
	x = ch_calloc(1, i);
	i = entry_getlen(&ptr);
	x->e_name.bv_val = ptr;
	x->e_name.bv_len = i;
	ptr += i+1;
	i = entry_getlen(&ptr);
	x->e_nname.bv_val = ptr;
	x->e_nname.bv_len = i;
	ptr += i+1;
#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, DETAIL2, "entry_decode: \"%s\"\n", x->e_dn, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE,
	    "entry_decode: \"%s\"\n",
	    x->e_dn, 0, 0 );
#endif
	x->e_bv = *bv;

	/* A valid entry must have at least one attr, so this
	 * pointer can never be NULL
	 */
	x->e_attrs = (Attribute *)(x+1);
	bptr = (BerVarray)x->e_attrs;
	a = NULL;

	while ((i = entry_getlen(&ptr))) {
		struct berval bv;
		bv.bv_len = i;
		bv.bv_val = ptr;
		if (a) {
			a->a_next = (Attribute *)bptr;
		}
		a = (Attribute *)bptr;
		ad = NULL;
		rc = slap_bv2ad( &bv, &ad, &text );

		if( rc != LDAP_SUCCESS ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, INFO, 
				"entry_decode: str2ad(%s): %s\n", ptr, text, 0 );
#else
			Debug( LDAP_DEBUG_TRACE,
				"<= entry_decode: str2ad(%s): %s\n", ptr, text, 0 );
#endif
			rc = slap_bv2undef_ad( &bv, &ad, &text );

			if( rc != LDAP_SUCCESS ) {
#ifdef NEW_LOGGING
				LDAP_LOG( OPERATION, INFO, 
					"entry_decode:  str2undef_ad(%s): %s\n", ptr, text, 0 );
#else
				Debug( LDAP_DEBUG_ANY,
					"<= entry_decode: str2undef_ad(%s): %s\n",
						ptr, text, 0 );
#endif
				return rc;
			}
		}
		ptr += i + 1;
		a->a_desc = ad;
		bptr = (BerVarray)(a+1);
		a->a_vals = bptr;
		a->a_flags = 0;
		j = entry_getlen(&ptr);

		while (j) {
			i = entry_getlen(&ptr);
			bptr->bv_len = i;
			bptr->bv_val = (char *)ptr;
			ptr += i+1;
			bptr++;
			j--;
		}
		bptr->bv_val = NULL;
		bptr->bv_len = 0;
		bptr++;
	}
	if (a)
		a->a_next = NULL;
#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, DETAIL1, "entry_decode:  %s\n", x->e_dn, 0, 0 );
#else
	Debug(LDAP_DEBUG_TRACE, "<= entry_decode(%s)\n",
		x->e_dn, 0, 0 );
#endif
	*e = x;
	return 0;
}
#endif

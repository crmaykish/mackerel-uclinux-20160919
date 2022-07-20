/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldbm/proto-back-ldbm.h,v 1.69.2.1 2003/02/09 16:31:38 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef _PROTO_BACK_LDBM
#define _PROTO_BACK_LDBM

#include <ldap_cdefs.h>

#include "external.h"

LDAP_BEGIN_DECL

/*
 * alias.c
 */
Entry *deref_internal_r LDAP_P((
	Backend *be,
	Entry *e,
	struct berval *dn,
	int *err,
	Entry **matched,
	const char **text ));

#define deref_entry_r( be, e, err, matched, text ) \
	deref_internal_r( be, e, NULL, err, matched, text )
#define deref_dn_r( be, dn, err, matched, text ) \
	deref_internal_r( be, NULL, dn, err, matched, text)

/*
 * attr.c
 */

void attr_mask LDAP_P(( struct ldbminfo *li,
	AttributeDescription *desc,
	slap_mask_t *indexmask ));

int attr_index_config LDAP_P(( struct ldbminfo *li,
	const char *fname, int lineno,
	int argc, char **argv ));
void attr_index_destroy LDAP_P(( Avlnode *tree ));

/*
 * cache.c
 */

int cache_add_entry_rw LDAP_P(( Cache *cache, Entry *e, int rw ));
int cache_update_entry LDAP_P(( Cache *cache, Entry *e ));
void cache_return_entry_rw LDAP_P(( Cache *cache, Entry *e, int rw ));
#define cache_return_entry_r(c, e) cache_return_entry_rw((c), (e), 0)
#define cache_return_entry_w(c, e) cache_return_entry_rw((c), (e), 1)
void cache_entry_commit LDAP_P(( Entry *e ));

ID cache_find_entry_ndn2id LDAP_P(( Backend *be, Cache *cache, struct berval *ndn ));
Entry * cache_find_entry_id LDAP_P(( Cache *cache, ID id, int rw ));
int cache_delete_entry LDAP_P(( Cache *cache, Entry *e ));
void cache_release_all LDAP_P(( Cache *cache ));

/*
 * dbcache.c
 */

DBCache * ldbm_cache_open LDAP_P(( Backend *be,
	const char *name, const char *suffix, int flags ));
void ldbm_cache_close LDAP_P(( Backend *be, DBCache *db ));
void ldbm_cache_really_close LDAP_P(( Backend *be, DBCache *db ));
void ldbm_cache_flush_all LDAP_P(( Backend *be ));
void ldbm_cache_sync LDAP_P(( Backend *be ));
#if 0 /* replaced by macro */
Datum ldbm_cache_fetch LDAP_P(( DBCache *db, Datum key ));
#else /* 1 */
#define ldbm_cache_fetch( db, key )	ldbm_fetch( (db)->dbc_db, (key) )
#endif /* 1 */
int ldbm_cache_store LDAP_P(( DBCache *db, Datum key, Datum data, int flags ));
int ldbm_cache_delete LDAP_P(( DBCache *db, Datum key ));
void *ldbm_cache_sync_daemon LDAP_P(( void *));

/*
 * dn2id.c
 */

int dn2id_add LDAP_P(( Backend *be, struct berval *dn, ID id ));
int dn2id LDAP_P(( Backend *be, struct berval *dn, ID *idp ));
int dn2idl LDAP_P(( Backend *be, struct berval *dn, int prefix, ID_BLOCK **idlp ));
int dn2id_delete LDAP_P(( Backend *be, struct berval *dn, ID id ));

Entry * dn2entry_rw LDAP_P(( Backend *be, struct berval *dn, Entry **matched, int rw ));
#define dn2entry_r(be, dn, m) dn2entry_rw((be), (dn), (m), 0)
#define dn2entry_w(be, dn, m) dn2entry_rw((be), (dn), (m), 1)

/*
 * entry.c
 */
int ldbm_back_entry_release_rw LDAP_P(( Backend *be,
	Connection *conn, Operation *op,
	Entry *e, int rw ));

/*
 * filterindex.c
 */

ID_BLOCK * filter_candidates LDAP_P(( Backend *be, Filter *f ));

/*
 * id2children.c
 */

int id2children_add LDAP_P(( Backend *be, Entry *p, Entry *e ));
int id2children_remove LDAP_P(( Backend *be, Entry *p, Entry *e ));
int has_children LDAP_P(( Backend *be, Entry *p ));

/*
 * id2entry.c
 */

int id2entry_add LDAP_P(( Backend *be, Entry *e ));
int id2entry_delete LDAP_P(( Backend *be, Entry *e ));

Entry * id2entry_rw LDAP_P(( Backend *be, ID id, int rw )); 
#define id2entry_r(be, id)	id2entry_rw((be), (id), 0)
#define id2entry_w(be, id)	id2entry_rw((be), (id), 1)

/*
 * idl.c
 */

ID_BLOCK * idl_alloc LDAP_P(( unsigned int nids ));
ID_BLOCK * idl_allids LDAP_P(( Backend *be ));
void idl_free LDAP_P(( ID_BLOCK *idl ));
ID_BLOCK * idl_fetch LDAP_P(( Backend *be, DBCache *db, Datum key ));
int idl_insert_key LDAP_P(( Backend *be, DBCache *db, Datum key, ID id ));
int idl_insert LDAP_P(( ID_BLOCK **idl, ID id, unsigned int maxids ));
int idl_delete_key LDAP_P(( Backend *be, DBCache *db, Datum key, ID id ));
ID_BLOCK * idl_intersection LDAP_P(( Backend *be, ID_BLOCK *a, ID_BLOCK *b ));
ID_BLOCK * idl_union LDAP_P(( Backend *be, ID_BLOCK *a, ID_BLOCK *b ));
ID_BLOCK * idl_notin LDAP_P(( Backend *be, ID_BLOCK *a, ID_BLOCK *b ));
ID idl_firstid LDAP_P(( ID_BLOCK *idl, ID *cursor ));
ID idl_nextid LDAP_P(( ID_BLOCK *idl, ID *cursor ));

/*
 * index.c
 */
extern int
index_is_indexed LDAP_P((
	Backend *be,
	AttributeDescription *desc ));

extern int
index_param LDAP_P((
	Backend *be,
	AttributeDescription *desc,
	int ftype,
	char **dbname,
	slap_mask_t *mask,
	struct berval *prefix ));

extern int
index_values LDAP_P((
	Backend *be,
	AttributeDescription *desc,
	BerVarray vals,
	ID id,
	int op ));

int index_entry LDAP_P(( Backend *be, int r, Entry *e, Attribute *ap ));
#define index_entry_add(be,e,ap) index_entry((be),SLAP_INDEX_ADD_OP,(e),(ap))
#define index_entry_del(be,e,ap) index_entry((be),SLAP_INDEX_DELETE_OP,(e),(ap))


/*
 * key.c
 */
extern int
key_change LDAP_P((
    Backend		*be,
    DBCache	*db,
    struct berval *k,
    ID			id,
    int			op ));
extern int
key_read LDAP_P((
    Backend	*be,
	DBCache *db,
    struct berval *k,
	ID_BLOCK **idout ));

/*
 * passwd.c
 */
extern BI_op_extended ldbm_back_exop_passwd;

/*
 * modify.c
 * These prototypes are placed here because they are used by modify and
 * modify rdn which are implemented in different files. 
 *
 * We need ldbm_internal_modify here because of LDAP modrdn & modify use 
 * it. If we do not add this, there would be a bunch of code replication 
 * here and there and of course the likelihood of bugs increases.
 * Juan C. Gomez (gomez@engr.sgi.com) 05/18/99
 * 
 */

/* returns LDAP error code indicating error OR SLAPD_ABANDON */
int ldbm_modify_internal LDAP_P((Backend *be,
	Connection *conn, Operation *op,
	const char *dn, Modifications *mods, Entry *e,
	const char **text, char *textbuf, size_t textlen ));

/*
 * nextid.c
 */

int next_id LDAP_P(( Backend *be, ID *idp ));
int next_id_get LDAP_P(( Backend *be, ID *idp ));
int next_id_write LDAP_P(( Backend *be, ID id ));

LDAP_END_DECL
#endif

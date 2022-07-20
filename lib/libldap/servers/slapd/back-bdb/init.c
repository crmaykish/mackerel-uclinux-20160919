/* init.c - initialize bdb backend */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-bdb/init.c,v 1.75.2.23 2003/03/29 15:45:43 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/stdlib.h>

#include "back-bdb.h"
#include "external.h"
#include <lutil.h>

static struct bdbi_database {
	char *file;
	char *name;
	int type;
	int flags;
} bdbi_databases[] = {
	{ "id2entry" BDB_SUFFIX, "id2entry", DB_BTREE, 0 },
#ifdef BDB_HIER
	{ "id2parent" BDB_SUFFIX, "id2parent", DB_BTREE, 0 },
#else
	{ "dn2id" BDB_SUFFIX, "dn2id", DB_BTREE, 0 },
#endif
	{ NULL, NULL, 0, 0 }
};

struct berval bdb_uuid = { 0, NULL };

typedef void * db_malloc(size_t);
typedef void * db_realloc(void *, size_t);

#if 0
static int
bdb_open( BackendInfo *bi )
{
	return 0;
}

static int
bdb_destroy( BackendInfo *bi )
{
	return 0;
}

static int
bdb_close( BackendInfo *bi )
{
	/* terminate the underlying database system */
	return 0;
}
#endif

static int
bdb_db_init( BackendDB *be )
{
	struct bdb_info	*bdb;

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_BDB, ENTRY, "bdb_db_init", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_ANY,
		"bdb_db_init: Initializing BDB database\n",
		0, 0, 0 );
#endif

	/* indicate system schema supported */
	be->be_flags |=
#ifdef BDB_SUBENTRIES
		SLAP_BFLAG_SUBENTRIES |
#endif
#ifdef BDB_ALIASES
		SLAP_BFLAG_ALIASES |
#endif
		SLAP_BFLAG_REFERRALS;

	/* allocate backend-database-specific stuff */
	bdb = (struct bdb_info *) ch_calloc( 1, sizeof(struct bdb_info) );

	/* DBEnv parameters */
	bdb->bi_dbenv_home = ch_strdup( SLAPD_DEFAULT_DB_DIR );
	bdb->bi_dbenv_xflags = 0;
	bdb->bi_dbenv_mode = SLAPD_DEFAULT_DB_MODE;

	bdb->bi_cache.c_maxsize = DEFAULT_CACHE_SIZE;

	bdb->bi_lock_detect = DB_LOCK_DEFAULT;
	bdb->bi_search_stack_depth = DEFAULT_SEARCH_STACK_DEPTH;
	bdb->bi_search_stack = NULL;

#if defined(LDAP_CLIENT_UPDATE) || defined(LDAP_SYNC)
	LDAP_LIST_INIT (&bdb->psearch_list);
#endif

	ldap_pvt_thread_mutex_init( &bdb->bi_lastid_mutex );
	ldap_pvt_thread_mutex_init( &bdb->bi_cache.lru_mutex );
	ldap_pvt_thread_rdwr_init ( &bdb->bi_cache.c_rwlock );
#ifdef BDB_HIER
	ldap_pvt_thread_rdwr_init( &bdb->bi_tree_rdwr );
#endif

	be->be_private = bdb;

	return 0;
}

int
bdb_bt_compare(
	DB *db, 
	const DBT *usrkey,
	const DBT *curkey
)
{
	unsigned char *u, *c;
	int i, x;

	u = usrkey->data;
	c = curkey->data;

#ifdef WORDS_BIGENDIAN
	for( i = 0; i < (int)sizeof(ID); i++)
#else
	for( i = sizeof(ID)-1; i >= 0; i--)
#endif
	{
		x = u[i] - c[i];
		if( x ) return x;
	}

	return 0;
}

static int
bdb_db_open( BackendDB *be )
{
	int rc, i;
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;
	u_int32_t flags;
#ifdef HAVE_EBCDIC
	char path[MAXPATHLEN];
#endif

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_BDB, ARGS, 
		"bdb_db_open: %s\n", be->be_suffix[0].bv_val, 0, 0 );
#else
	Debug( LDAP_DEBUG_ARGS,
		"bdb_db_open: %s\n",
		be->be_suffix[0].bv_val, 0, 0 );
#endif

#ifndef BDB_MULTIPLE_SUFFIXES
	if ( be->be_suffix[1].bv_val ) {
#ifdef NEW_LOGGING
	LDAP_LOG( BACK_BDB, ERR, 
		"bdb_db_open: only one suffix allowed\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_ANY,
		"bdb_db_open: only one suffix allowed\n", 0, 0, 0 );
#endif
		return -1;
	}
#endif
	/* we should check existance of dbenv_home and db_directory */

	rc = db_env_create( &bdb->bi_dbenv, 0 );
	if( rc != 0 ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_BDB, ERR, 
			"bdb_db_open: db_env_create failed: %s (%d)\n", 
			db_strerror(rc), rc, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
			"bdb_db_open: db_env_create failed: %s (%d)\n",
			db_strerror(rc), rc, 0 );
#endif
		return rc;
	}

	flags = DB_INIT_MPOOL | DB_THREAD | DB_CREATE
		| DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_TXN;
	
#if 0
	/* Never do automatic recovery, must perform it manually.
	 * Otherwise restarting with gentlehup will corrupt the
	 * database.
	 */
	if( !(slapMode & SLAP_TOOL_MODE) ) flags |= DB_RECOVER;
#endif

	bdb->bi_dbenv->set_errpfx( bdb->bi_dbenv, be->be_suffix[0].bv_val );
	bdb->bi_dbenv->set_errcall( bdb->bi_dbenv, bdb_errcall );
	bdb->bi_dbenv->set_lk_detect( bdb->bi_dbenv, bdb->bi_lock_detect );

#ifdef SLAP_IDL_CACHE
	if ( bdb->bi_idl_cache_max_size ) {
		ldap_pvt_thread_mutex_init( &bdb->bi_idl_tree_mutex );
		bdb->bi_idl_cache_size = 0;
	}
#endif

#ifdef BDB_SUBDIRS
	{
		char dir[MAXPATHLEN], *ptr;
		
		if (bdb->bi_dbenv_home[0] == '.') {
			/* If home is a relative path, relative subdirs
			 * are just concat'd by BDB. We don't want the
			 * path to be concat'd twice, e.g.
			 * ./test-db/./test-db/tmp
			 */
			ptr = dir;
		} else {
			ptr = lutil_strcopy( dir, bdb->bi_dbenv_home );
			*ptr++ = LDAP_DIRSEP[0];
#ifdef HAVE_EBCDIC
			__atoe( dir );
#endif
		}

		strcpy( ptr, BDB_TMP_SUBDIR );
#ifdef HAVE_EBCDIC
		__atoe( ptr );
#endif
		rc = bdb->bi_dbenv->set_tmp_dir( bdb->bi_dbenv, dir );
		if( rc != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_BDB, ERR, 
				"bdb_db_open: set_tmp_dir(%s) failed: %s (%d)\n", 
				dir, db_strerror(rc), rc );
#else
			Debug( LDAP_DEBUG_ANY,
				"bdb_db_open: set_tmp_dir(%s) failed: %s (%d)\n",
				dir, db_strerror(rc), rc );
#endif
			return rc;
		}

		strcpy( ptr, BDB_LG_SUBDIR );
#ifdef HAVE_EBCDIC
		__atoe( ptr );
#endif
		rc = bdb->bi_dbenv->set_lg_dir( bdb->bi_dbenv, dir );
		if( rc != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_BDB, ERR, 
				"bdb_db_open: set_lg_dir(%s) failed: %s (%d)\n", 
				dir, db_strerror(rc), rc );
#else
			Debug( LDAP_DEBUG_ANY,
				"bdb_db_open: set_lg_dir(%s) failed: %s (%d)\n",
				dir, db_strerror(rc), rc );
#endif
			return rc;
		}

		strcpy( ptr, BDB_DATA_SUBDIR );
#ifdef HAVE_EBCDIC
		__atoe( ptr );
#endif
		rc = bdb->bi_dbenv->set_data_dir( bdb->bi_dbenv, dir );
		if( rc != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_BDB, ERR, 
				"bdb_db_open: set_data_dir(%s) failed: %s (%d)\n",
				dir, db_strerror(rc), rc );
#else
			Debug( LDAP_DEBUG_ANY,
				"bdb_db_open: set_data_dir(%s) failed: %s (%d)\n",
				dir, db_strerror(rc), rc );
#endif
			return rc;
		}
	}
#endif

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_BDB, DETAIL1, 
		"bdb_db_open: dbenv_open %s\n", bdb->bi_dbenv_home, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE,
		"bdb_db_open: dbenv_open(%s)\n",
		bdb->bi_dbenv_home, 0, 0);
#endif

#ifdef HAVE_EBCDIC
	strcpy( path, bdb->bi_dbenv_home );
	__atoe( path );
	rc = bdb->bi_dbenv->open( bdb->bi_dbenv,
		path,
		flags,
		bdb->bi_dbenv_mode );
#else
	rc = bdb->bi_dbenv->open( bdb->bi_dbenv,
		bdb->bi_dbenv_home,
		flags,
		bdb->bi_dbenv_mode );
#endif
	if( rc != 0 ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_BDB, ERR, 
			"bdb_db_open: dbenv_open failed: %s (%d)\n", 
			db_strerror(rc), rc, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
			"bdb_db_open: dbenv_open failed: %s (%d)\n",
			db_strerror(rc), rc, 0 );
#endif
		return rc;
	}

	if( bdb->bi_dbenv_xflags != 0 ) {
		rc = bdb->bi_dbenv->set_flags( bdb->bi_dbenv,
			bdb->bi_dbenv_xflags, 1);
		if( rc != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_BDB, ERR, 
				"bdb_db_open: dbenv_set_flags failed: %s (%d)\n", 
				db_strerror(rc), rc, 0 );
#else
			Debug( LDAP_DEBUG_ANY,
				"bdb_db_open: dbenv_set_flags failed: %s (%d)\n",
				db_strerror(rc), rc, 0 );
#endif
			return rc;
		}
	}

	flags = DB_THREAD | DB_CREATE | bdb->bi_db_opflags;

	bdb->bi_databases = (struct bdb_db_info **) ch_malloc(
		BDB_INDICES * sizeof(struct bdb_db_info *) );

	/* open (and create) main database */
	for( i = 0; bdbi_databases[i].name; i++ ) {
		struct bdb_db_info *db;

		db = (struct bdb_db_info *) ch_calloc(1, sizeof(struct bdb_db_info));

		rc = db_create( &db->bdi_db, bdb->bi_dbenv, 0 );
		if( rc != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_BDB, ERR, 
				"bdb_db_open: db_create(%s) failed: %s (%d)\n", 
				bdb->bi_dbenv_home, db_strerror(rc), rc );
#else
			Debug( LDAP_DEBUG_ANY,
				"bdb_db_open: db_create(%s) failed: %s (%d)\n",
				bdb->bi_dbenv_home, db_strerror(rc), rc );
#endif
			return rc;
		}

		if( i == BDB_ID2ENTRY ) {
			rc = db->bdi_db->set_bt_compare( db->bdi_db,
				bdb_bt_compare );
			rc = db->bdi_db->set_pagesize( db->bdi_db,
				BDB_ID2ENTRY_PAGESIZE );
		} else {
#ifdef BDB_HIER
			rc = db->bdi_db->set_bt_compare( db->bdi_db,
				bdb_bt_compare );
#else
			rc = db->bdi_db->set_flags( db->bdi_db, 
				DB_DUP | DB_DUPSORT );
			rc = db->bdi_db->set_dup_compare( db->bdi_db,
				bdb_bt_compare );
#endif
			rc = db->bdi_db->set_pagesize( db->bdi_db,
				BDB_PAGESIZE );
		}

#ifdef HAVE_EBCDIC
		strcpy( path, bdbi_databases[i].file );
		__atoe( path );
		rc = DB_OPEN( db->bdi_db, NULL,
			path,
		/*	bdbi_databases[i].name, */ NULL,
			bdbi_databases[i].type,
			bdbi_databases[i].flags | flags | DB_AUTO_COMMIT,
			bdb->bi_dbenv_mode );
#else
		rc = DB_OPEN( db->bdi_db, NULL,
			bdbi_databases[i].file,
		/*	bdbi_databases[i].name, */ NULL,
			bdbi_databases[i].type,
			bdbi_databases[i].flags | flags | DB_AUTO_COMMIT,
			bdb->bi_dbenv_mode );
#endif

		if( rc != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_BDB, ERR, 
				"bdb_db_open: db_create(%s) failed: %s (%d)\n", 
				bdb->bi_dbenv_home, db_strerror(rc), rc );
#else
			Debug( LDAP_DEBUG_ANY,
				"bdb_db_open: db_open(%s) failed: %s (%d)\n",
				bdb->bi_dbenv_home, db_strerror(rc), rc );
#endif
			return rc;
		}

		db->bdi_name = bdbi_databases[i].name;
		bdb->bi_databases[i] = db;
	}

	bdb->bi_databases[i] = NULL;
	bdb->bi_ndatabases = i;

	/* get nextid */
	rc = bdb_last_id( be, NULL );
	if( rc != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_BDB, ERR, 
				"bdb_db_open: last_id(%s) failed: %s (%d)\n", 
				bdb->bi_dbenv_home, db_strerror(rc), rc );
#else
		Debug( LDAP_DEBUG_ANY,
			"bdb_db_open: last_id(%s) failed: %s (%d)\n",
			bdb->bi_dbenv_home, db_strerror(rc), rc );
#endif
		return rc;
	}

	/* <insert> open (and create) index databases */
#ifdef BDB_HIER
	rc = bdb_build_tree( be );
#endif
	return 0;
}

static int
bdb_db_close( BackendDB *be )
{
	int rc;
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;
	struct bdb_db_info *db;
#ifdef SLAP_IDL_CACHE
	bdb_idl_cache_entry_t *entry, *next_entry;
#endif

	while( bdb->bi_ndatabases-- ) {
		db = bdb->bi_databases[bdb->bi_ndatabases];
		rc = db->bdi_db->close( db->bdi_db, 0 );
		/* Lower numbered names are not strdup'd */
		if( bdb->bi_ndatabases >= BDB_NDB )
			free( db->bdi_name );
		free( db );
	}
	free( bdb->bi_databases );
	bdb_attr_index_destroy( bdb->bi_attrs );

	bdb_cache_release_all (&bdb->bi_cache);

#ifdef SLAP_IDL_CACHE
	ldap_pvt_thread_mutex_lock ( &bdb->bi_idl_tree_mutex );
	entry = bdb->bi_idl_lru_head;
	while ( entry != NULL ) {
		next_entry = entry->idl_lru_next;
		avl_delete( &bdb->bi_idl_tree, (caddr_t) entry, bdb_idl_entry_cmp );
		free( entry->idl );
		free( entry->kstr.bv_val );
		free( entry );
		entry = next_entry;
	}
	ldap_pvt_thread_mutex_unlock ( &bdb->bi_idl_tree_mutex );
#endif

	return 0;
}

static int
bdb_db_destroy( BackendDB *be )
{
	int rc;
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;

	/* close db environment */
	if( bdb->bi_dbenv ) {
		/* force a checkpoint */
		rc = TXN_CHECKPOINT( bdb->bi_dbenv, 0, 0, DB_FORCE );
		if( rc != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_BDB, ERR, 
				"bdb_db_destroy: txn_checkpoint failed: %s (%d)\n",
				db_strerror(rc), rc, 0 );
#else
			Debug( LDAP_DEBUG_ANY,
				"bdb_db_destroy: txn_checkpoint failed: %s (%d)\n",
				db_strerror(rc), rc, 0 );
#endif
		}

		bdb_cache_release_all (&bdb->bi_cache);

		rc = bdb->bi_dbenv->close( bdb->bi_dbenv, 0 );
		bdb->bi_dbenv = NULL;
		if( rc != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_BDB, ERR, 
				"bdb_db_destroy: close failed: %s (%d)\n", 
				db_strerror(rc), rc, 0 );
#else
			Debug( LDAP_DEBUG_ANY,
				"bdb_db_destroy: close failed: %s (%d)\n",
				db_strerror(rc), rc, 0 );
#endif
			return rc;
		}
	}

	if( bdb->bi_dbenv_home ) ch_free( bdb->bi_dbenv_home );

#ifdef BDB_HIER
	ldap_pvt_thread_rdwr_destroy( &bdb->bi_tree_rdwr );
#endif
	ldap_pvt_thread_rdwr_destroy ( &bdb->bi_cache.c_rwlock );
	ldap_pvt_thread_mutex_destroy( &bdb->bi_cache.lru_mutex );
	ldap_pvt_thread_mutex_destroy( &bdb->bi_lastid_mutex );

	ch_free( bdb );
	be->be_private = NULL;

	return 0;
}

#ifdef SLAPD_BDB_DYNAMIC
int back_bdb_LTX_init_module( int argc, char *argv[] ) {
	BackendInfo bi;

	memset( &bi, '\0', sizeof(bi) );
	bi.bi_type = "bdb";
	bi.bi_init = bdb_initialize;

	backend_add( &bi );
	return 0;
}
#endif /* SLAPD_BDB_DYNAMIC */

int
bdb_initialize(
	BackendInfo	*bi
)
{
	static char *controls[] = {
		LDAP_CONTROL_MANAGEDSAIT,
		LDAP_CONTROL_NOOP,
#ifdef LDAP_CONTROL_PAGEDRESULTS
		LDAP_CONTROL_PAGEDRESULTS,
#endif
 		LDAP_CONTROL_VALUESRETURNFILTER,
#ifdef LDAP_CONTROL_SUBENTRIES
		LDAP_CONTROL_SUBENTRIES,
#endif
#ifdef LDAP_CLIENT_UPDATE
		LDAP_CONTROL_CLIENT_UPDATE,
#endif
		NULL
	};

	bi->bi_controls = controls;

	/* initialize the underlying database system */
#ifdef NEW_LOGGING
	LDAP_LOG( BACK_BDB, ENTRY, "bdb_db_initialize\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "bdb_initialize: initialize BDB backend\n",
		0, 0, 0 );
#endif

	{	/* version check */
		int major, minor, patch;
		char *version = db_version( &major, &minor, &patch );
#ifdef HAVE_EBCDIC
		char v2[1024];

		/* All our stdio does an ASCII to EBCDIC conversion on
		 * the output. Strings from the BDB library are already
		 * in EBCDIC; we have to go back and forth...
		 */
		strcpy( v2, version );
		__etoa( v2 );
		version = v2;
#endif

		if( major != DB_VERSION_MAJOR ||
			minor != DB_VERSION_MINOR ||
			patch < DB_VERSION_PATCH )
		{
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_BDB, ERR, 
				"bdb_db_initialize: version mismatch: "
				"\texpected: %s \tgot: %s\n", DB_VERSION_STRING, version, 0 );
#else
			Debug( LDAP_DEBUG_ANY,
				"bdb_initialize: version mismatch\n"
				"\texpected: " DB_VERSION_STRING "\n"
				"\tgot: %s \n", version, 0, 0 );
#endif
		}

#ifdef NEW_LOGGING
		LDAP_LOG( BACK_BDB, DETAIL1, 
			"bdb_db_initialize: %s\n", version, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "bdb_initialize: %s\n",
			version, 0, 0 );
#endif
	}

	db_env_set_func_free( ber_memfree );
	db_env_set_func_malloc( (db_malloc *)ber_memalloc );
	db_env_set_func_realloc( (db_realloc *)ber_memrealloc );
#ifndef NO_THREAD
	/* This is a no-op on a NO_THREAD build. Leave the default
	 * alone so that BDB will sleep on interprocess conflicts.
	 */
	db_env_set_func_yield( ldap_pvt_thread_yield );
#endif

	{
		static char uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];

		bdb_uuid.bv_len = lutil_uuidstr( uuidbuf, sizeof( uuidbuf ));
		bdb_uuid.bv_val = uuidbuf;
	}

	bi->bi_open = 0;
	bi->bi_close = 0;
	bi->bi_config = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = bdb_db_init;
	bi->bi_db_config = bdb_db_config;
	bi->bi_db_open = bdb_db_open;
	bi->bi_db_close = bdb_db_close;
	bi->bi_db_destroy = bdb_db_destroy;

	bi->bi_op_add = bdb_add;
	bi->bi_op_bind = bdb_bind;
	bi->bi_op_compare = bdb_compare;
	bi->bi_op_delete = bdb_delete;
	bi->bi_op_modify = bdb_modify;
	bi->bi_op_modrdn = bdb_modrdn;
	bi->bi_op_search = bdb_search;

	bi->bi_op_unbind = 0;

#ifdef LDAP_CLIENT_UPDATE
	bi->bi_op_abandon = bdb_abandon;
	bi->bi_op_cancel = bdb_cancel;
#else
	bi->bi_op_abandon = 0;
	bi->bi_op_cancel = 0;
#endif

	bi->bi_extended = bdb_extended;

#if 1
	/*
	 * these routines (and their callers) are not yet designed
	 * to work with transaction.  Using them may cause deadlock.
	 */
	bi->bi_acl_group = bdb_group;
	bi->bi_acl_attribute = bdb_attribute;
#else
	bi->bi_acl_group = 0;
	bi->bi_acl_attribute = 0;
#endif

	bi->bi_chk_referrals = bdb_referrals;
	bi->bi_operational = bdb_operational;
	bi->bi_has_subordinates = bdb_hasSubordinates;
	bi->bi_entry_release_rw = bdb_entry_release;

	/*
	 * hooks for slap tools
	 */
	bi->bi_tool_entry_open = bdb_tool_entry_open;
	bi->bi_tool_entry_close = bdb_tool_entry_close;
	bi->bi_tool_entry_first = bdb_tool_entry_next;
	bi->bi_tool_entry_next = bdb_tool_entry_next;
	bi->bi_tool_entry_get = bdb_tool_entry_get;
	bi->bi_tool_entry_put = bdb_tool_entry_put;
	bi->bi_tool_entry_reindex = bdb_tool_entry_reindex;
	bi->bi_tool_sync = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	return 0;
}

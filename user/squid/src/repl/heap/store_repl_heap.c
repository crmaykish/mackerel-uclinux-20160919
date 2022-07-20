
/*
 * $Id: store_repl_heap.c,v 1.8.2.2 2004/08/05 20:23:01 hno Exp $
 *
 * DEBUG: section ?     HEAP based removal policies
 * AUTHOR: Henrik Nordstrom
 *
 * Based on the ideas of the heap policy implemented by John Dilley of
 * Hewlett Packard. Rewritten from scratch when modularizing the removal
 * policy implementation of Squid.
 *
 * For details on the original heap policy work and the thinking behind see
 * http://www.hpl.hp.com/techreports/1999/HPL-1999-69.html
 *
 * 
 * SQUID Web Proxy Cache          http://www.squid-cache.org/
 * ----------------------------------------------------------
 *
 *  Squid is the result of efforts by numerous individuals from
 *  the Internet community; see the CONTRIBUTORS file for full
 *  details.   Many organizations have provided support for Squid's
 *  development; see the SPONSORS file for full details.  Squid is
 *  Copyrighted (C) 2001 by the Regents of the University of
 *  California; see the COPYRIGHT file for full details.  Squid
 *  incorporates software developed and/or copyrighted by other
 *  sources; see the CREDITS file for full details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */

#include "squid.h"
#include "heap.h"
#include "store_heap_replacement.h"

REMOVALPOLICYCREATE createRemovalPolicy_heap;

static int nr_heap_policies = 0;

typedef struct _HeapPolicyData HeapPolicyData;
struct _HeapPolicyData {
    RemovalPolicy *policy;
    heap *heap;
    heap_key_func *keyfunc;
    int count;
    int nwalkers;
    enum heap_entry_type {
	TYPE_UNKNOWN = 0, TYPE_STORE_ENTRY, TYPE_STORE_MEM
    } type;
};

/* Hack to avoid having to remember the RemovalPolicyNode location.
 * Needed by the purge walker.
 */
static enum heap_entry_type
heap_guessType(StoreEntry * entry, RemovalPolicyNode * node)
{
    if (node == &entry->repl)
	return TYPE_STORE_ENTRY;
    if (entry->mem_obj && node == &entry->mem_obj->repl)
	return TYPE_STORE_MEM;
    fatal("Heap Replacement: Unknown StoreEntry node type");
    return TYPE_UNKNOWN;
}
#define SET_POLICY_NODE(entry,value) \
    switch(heap->type) { \
    case TYPE_STORE_ENTRY: entry->repl.data = value; break ; \
    case TYPE_STORE_MEM: entry->mem_obj->repl.data = value ; break ; \
    default: break; \
    }

static void
heap_add(RemovalPolicy * policy, StoreEntry * entry, RemovalPolicyNode * node)
{
    HeapPolicyData *heap = policy->_data;
    assert(!node->data);
    if (EBIT_TEST(entry->flags, ENTRY_SPECIAL))
	return;			/* We won't manage these.. they messes things up */
    node->data = heap_insert(heap->heap, entry);
    heap->count += 1;
    if (!heap->type)
	heap->type = heap_guessType(entry, node);
    /* Add a little more variance to the aging factor */
    heap->heap->age += heap->heap->age / 100000000;
}

static void
heap_remove(RemovalPolicy * policy, StoreEntry * entry,
    RemovalPolicyNode * node)
{
    HeapPolicyData *heap = policy->_data;
    heap_node *hnode = node->data;
    if (!hnode)
	return;
    heap_delete(heap->heap, hnode);
    node->data = NULL;
    heap->count -= 1;
}

static void
heap_referenced(RemovalPolicy * policy, const StoreEntry * entry,
    RemovalPolicyNode * node)
{
    HeapPolicyData *heap = policy->_data;
    heap_node *hnode = node->data;
    if (!hnode)
	return;
    heap_update(heap->heap, hnode, (StoreEntry *) entry);
}

/** RemovalPolicyWalker **/

typedef struct _HeapWalkData HeapWalkData;
struct _HeapWalkData {
    int current;
};

static const StoreEntry *
heap_walkNext(RemovalPolicyWalker * walker)
{
    HeapWalkData *heap_walk = walker->_data;
    RemovalPolicy *policy = walker->_policy;
    HeapPolicyData *heap = policy->_data;
    StoreEntry *entry;
    if (heap_walk->current >= heap_nodes(heap->heap))
	return NULL;		/* done */
    entry = (StoreEntry *) heap_peep(heap->heap, heap_walk->current++);
    return entry;
}

static void
heap_walkDone(RemovalPolicyWalker * walker)
{
    RemovalPolicy *policy = walker->_policy;
    HeapPolicyData *heap = policy->_data;
    assert(strcmp(policy->_type, "heap") == 0);
    assert(heap->nwalkers > 0);
    heap->nwalkers -= 1;
    safe_free(walker->_data);
    cbdataFree(walker);
}

static RemovalPolicyWalker *
heap_walkInit(RemovalPolicy * policy)
{
    HeapPolicyData *heap = policy->_data;
    RemovalPolicyWalker *walker;
    HeapWalkData *heap_walk;
    heap->nwalkers += 1;
    walker = cbdataAlloc(RemovalPolicyWalker);
    heap_walk = xcalloc(1, sizeof(*heap_walk));
    heap_walk->current = 0;
    walker->_policy = policy;
    walker->_data = heap_walk;
    walker->Next = heap_walkNext;
    walker->Done = heap_walkDone;
    return walker;
}

/** RemovalPurgeWalker **/

typedef struct _HeapPurgeData HeapPurgeData;
struct _HeapPurgeData {
    link_list *locked_entries;
    heap_key min_age;
};

static StoreEntry *
heap_purgeNext(RemovalPurgeWalker * walker)
{
    HeapPurgeData *heap_walker = walker->_data;
    RemovalPolicy *policy = walker->_policy;
    HeapPolicyData *heap = policy->_data;
    StoreEntry *entry;
    heap_key age;
  try_again:
    if (!heap_nodes(heap->heap) > 0)
	return NULL;		/* done */
    age = heap_peepminkey(heap->heap);
    entry = heap_extractmin(heap->heap);
    if (storeEntryLocked(entry)) {
	storeLockObject(entry);
	linklistPush(&heap_walker->locked_entries, entry);
	goto try_again;
    }
    heap_walker->min_age = age;
    SET_POLICY_NODE(entry, NULL);
    return entry;
}

static void
heap_purgeDone(RemovalPurgeWalker * walker)
{
    HeapPurgeData *heap_walker = walker->_data;
    RemovalPolicy *policy = walker->_policy;
    HeapPolicyData *heap = policy->_data;
    StoreEntry *entry;
    assert(strcmp(policy->_type, "heap") == 0);
    assert(heap->nwalkers > 0);
    heap->nwalkers -= 1;
    if (heap_walker->min_age > 0) {
	heap->heap->age = heap_walker->min_age;
	debug(81, 3) ("heap_purgeDone: Heap age set to %f\n",
	    (double) heap->heap->age);
    }
    /*
     * Reinsert the locked entries
     */
    while ((entry = linklistShift(&heap_walker->locked_entries))) {
	heap_node *node = heap_insert(heap->heap, entry);
	SET_POLICY_NODE(entry, node);
	storeUnlockObject(entry);
    }
    safe_free(walker->_data);
    cbdataFree(walker);
}

static RemovalPurgeWalker *
heap_purgeInit(RemovalPolicy * policy, int max_scan)
{
    HeapPolicyData *heap = policy->_data;
    RemovalPurgeWalker *walker;
    HeapPurgeData *heap_walk;
    heap->nwalkers += 1;
    walker = cbdataAlloc(RemovalPurgeWalker);
    heap_walk = xcalloc(1, sizeof(*heap_walk));
    heap_walk->min_age = 0.0;
    heap_walk->locked_entries = NULL;
    walker->_policy = policy;
    walker->_data = heap_walk;
    walker->max_scan = max_scan;
    walker->Next = heap_purgeNext;
    walker->Done = heap_purgeDone;
    return walker;
}

static void
heap_free(RemovalPolicy * policy)
{
    HeapPolicyData *heap = policy->_data;
    /* Make some verification of the policy state */
    assert(strcmp(policy->_type, "heap") == 0);
    assert(heap->nwalkers);
    assert(heap->count);
    /* Ok, time to destroy this policy */
    safe_free(policy->_data);
    memset(policy, 0, sizeof(*policy));
    cbdataFree(policy);
}

RemovalPolicy *
createRemovalPolicy_heap(wordlist * args)
{
    RemovalPolicy *policy;
    HeapPolicyData *heap_data;
    const char *keytype;
    /* Allocate the needed structures */
    policy = cbdataAlloc(RemovalPolicy);
    heap_data = xcalloc(1, sizeof(*heap_data));
    /* Initialize the policy data */
    heap_data->policy = policy;
    if (args) {
	keytype = args->key;
	args = args->next;
    } else {
	debug(81, 1) ("createRemovalPolicy_heap: No key type specified. Using LRU\n");
	keytype = "LRU";
    }
    if (!strcmp(keytype, "GDSF"))
	heap_data->keyfunc = HeapKeyGen_StoreEntry_GDSF;
    else if (!strcmp(keytype, "LFUDA"))
	heap_data->keyfunc = HeapKeyGen_StoreEntry_LFUDA;
    else if (!strcmp(keytype, "LRU"))
	heap_data->keyfunc = HeapKeyGen_StoreEntry_LRU;
    else {
	debug(81, 0) ("createRemovalPolicy_heap: Unknown key type \"%s\". Using LRU\n",
	    keytype);
	heap_data->keyfunc = HeapKeyGen_StoreEntry_LRU;
    }
    /* No additional arguments expected */
    assert(!args);
    heap_data->heap = new_heap(1000, heap_data->keyfunc);
    heap_data->heap->age = 1.0;
    /* Populate the policy structure */
    policy->_type = "heap";
    policy->_data = heap_data;
    policy->Free = heap_free;
    policy->Add = heap_add;
    policy->Remove = heap_remove;
    policy->Referenced = NULL;
    policy->Dereferenced = heap_referenced;
    policy->WalkInit = heap_walkInit;
    policy->PurgeInit = heap_purgeInit;
    /* Increase policy usage count */
    nr_heap_policies += 0;
    return policy;
}

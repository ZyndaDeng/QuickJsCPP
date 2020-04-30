#pragma once
#include "cutils.h"

#define JS_PROP_INITIAL_SIZE 2
#define JS_PROP_INITIAL_HASH_SIZE 4 /* must be a power of two */
#define JS_ARRAY_INITIAL_SIZE 2

typedef struct JSShapeProperty {
    uint32_t hash_next : 26; /* 0 if last in list */
    uint32_t flags : 6;   /* JS_PROP_XXX */
    JSAtom atom; /* JS_ATOM_NULL = free property entry */
} JSShapeProperty;

struct JSShape {
    uint32_t prop_hash_end[0]; /* hash table of size hash_mask + 1
                                  before the start of the structure. */
    JSRefCountHeader header; /* must come first, 32-bit */
    JSGCHeader gc_header; /* must come after JSRefCountHeader, 8-bit */
    /* true if the shape is inserted in the shape hash table. If not,
       JSShape.hash is not valid */
    uint8_t is_hashed;
    /* If true, the shape may have small array index properties 'n' with 0
       <= n <= 2^31-1. If false, the shape is guaranteed not to have
       small array index properties */
    uint8_t has_small_array_index;
    uint32_t hash; /* current hash value */
    uint32_t prop_hash_mask;
    int prop_size; /* allocated properties */
    int prop_count;
    JSShape* shape_hash_next; /* in JSRuntime.shape_hash[h] list */
    JSObject* proto;
    JSShapeProperty prop[0]; /* prop_size elements */
};

/* same magic hash multiplier as the Linux kernel */
static uint32_t shape_hash(uint32_t h, uint32_t val)
{
    return (h + val) * 0x9e370001;
}

/* truncate the shape hash to 'hash_bits' bits */
static uint32_t get_shape_hash(uint32_t h, int hash_bits)
{
    return h >> (32 - hash_bits);
}

static JSShape* js_dup_shape(JSShape* sh)
{
    sh->header.ref_count++;
    return sh;
}

static inline size_t get_shape_size(size_t hash_size, size_t prop_size)
{
    return hash_size * sizeof(uint32_t) + sizeof(JSShape) +
        prop_size * sizeof(JSShapeProperty);
}


static inline JSShape* get_shape_from_alloc(void* sh_alloc, size_t hash_size)
{
    return (JSShape*)(void*)((uint32_t*)sh_alloc + hash_size);
}

static inline void* get_alloc_from_shape(JSShape* sh)
{
    return sh->prop_hash_end - ((intptr_t)sh->prop_hash_mask + 1);
}

static inline JSShapeProperty* get_shape_prop(JSShape* sh)
{
    return sh->prop;
}
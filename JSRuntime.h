#pragma once
#include "cutils.h"
#include "JSValue.h"
#include "list.h"
#include "JSModule.h"

typedef struct JSMallocState {
    size_t malloc_count;
    size_t malloc_size;
    size_t malloc_limit;
    void* opaque; /* user opaque */
} JSMallocState;

typedef struct JSMallocFunctions {
    void* (*js_malloc)(JSMallocState* s, size_t size);
    void (*js_free)(JSMallocState* s, void* ptr);
    void* (*js_realloc)(JSMallocState* s, void* ptr, size_t size);
    size_t(*js_malloc_usable_size)(const void* ptr);
} JSMallocFunctions;

class JSString;
typedef  JSString JSAtomStruct;
class JSContext;
class JSClass;
class JSRuntime;
struct JSShape;
typedef int JSInterruptHandler(JSRuntime* rt, void* opaque);

class JSRuntime {
	friend class JSContext;
public:
	static JSRuntime* JS_NewRuntime(void);

	JSContext* JS_NewContext();
    void js_free_rt( void* ptr);
protected:
	void __JS_FreeValueRT(JSValue v);
	void JS_FreeAtomStruct(JSAtomStruct* p);
    void free_object(JSObject* p);
    void free_object2( JSObject* p);
    void free_object_struct(JSObject* p);
    void free_function_bytecode( JSFunctionBytecode* b);
    JSShape* find_hashed_shape_proto( JSObject* proto);
    void js_shape_hash_link(JSShape* sh);
    int resize_shape_hash( int new_shape_hash_bits);
    void* js_malloc_rt( size_t size)
    {
        return mf.js_malloc(&malloc_state, size);
    }
    void js_trigger_gc(size_t size);
    void js_free_shape(JSShape* sh);

    JSMallocFunctions mf;
    JSMallocState malloc_state;
    const char* rt_info;

    int atom_hash_size; /* power of two */
    int atom_count;
    int atom_size;
    int atom_count_resize; /* resize hash table at this count */
    uint32_t* atom_hash;
    JSAtomStruct** atom_array;
    int atom_free_index; /* 0 = none */

    int class_count;    /* size of class_array */
    JSClass* class_array;

    struct list_head context_list; /* list of JSContext.link */
    /* list of allocated objects (used by the garbage collector) */
    struct list_head obj_list; /* list of JSObject.link */
    size_t malloc_gc_threshold;
#ifdef DUMP_LEAKS
    struct list_head string_list; /* list of JSString.link */
#endif
    struct list_head tmp_obj_list; /* used during gc */
    struct list_head free_obj_list; /* used during gc */
    struct list_head* el_next; /* used during gc */

    JSInterruptHandler* interrupt_handler;
    void* interrupt_opaque;

    struct list_head job_list; /* list of JSJobEntry.link */

    JSModuleNormalizeFunc* module_normalize_func;
    JSModuleLoaderFunc* module_loader_func;
    void* module_loader_opaque;

    BOOL in_gc_sweep : 8;
    BOOL can_block : 8; /* TRUE if Atomics.wait can block */

    /* Shape hash table */
    int shape_hash_bits;
    int shape_hash_size;
    int shape_hash_count; /* number of hashed shapes */
    JSShape** shape_hash;
#ifdef CONFIG_BIGNUM
    bf_context_t bf_ctx;
#endif
};
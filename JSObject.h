#pragma once
#include "RecyclableObject.h"
#include "JSFunction.h"
#include "JSValue.h"
#include "JSShape.h"
/* flags for object properties */
#define JS_PROP_CONFIGURABLE  (1 << 0)
#define JS_PROP_WRITABLE      (1 << 1)
#define JS_PROP_ENUMERABLE    (1 << 2)
#define JS_PROP_C_W_E         (JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE | JS_PROP_ENUMERABLE)
#define JS_PROP_LENGTH        (1 << 3) /* used internally in Arrays */
#define JS_PROP_TMASK         (3 << 4) /* mask for NORMAL, GETSET, VARREF, AUTOINIT */
#define JS_PROP_NORMAL         (0 << 4)
#define JS_PROP_GETSET         (1 << 4)
#define JS_PROP_VARREF         (2 << 4) /* used internally */
#define JS_PROP_AUTOINIT       (3 << 4) /* used internally */

/* flags for JS_DefineProperty */
#define JS_PROP_HAS_SHIFT        8
#define JS_PROP_HAS_CONFIGURABLE (1 << 8)
#define JS_PROP_HAS_WRITABLE     (1 << 9)
#define JS_PROP_HAS_ENUMERABLE   (1 << 10)
#define JS_PROP_HAS_GET          (1 << 11)
#define JS_PROP_HAS_SET          (1 << 12)
#define JS_PROP_HAS_VALUE        (1 << 13)

/* flags for object properties */
#define JS_PROP_CONFIGURABLE  (1 << 0)
#define JS_PROP_WRITABLE      (1 << 1)
#define JS_PROP_ENUMERABLE    (1 << 2)
#define JS_PROP_C_W_E         (JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE | JS_PROP_ENUMERABLE)
#define JS_PROP_LENGTH        (1 << 3) /* used internally in Arrays */
#define JS_PROP_TMASK         (3 << 4) /* mask for NORMAL, GETSET, VARREF, AUTOINIT */
#define JS_PROP_NORMAL         (0 << 4)
#define JS_PROP_GETSET         (1 << 4)
#define JS_PROP_VARREF         (2 << 4) /* used internally */
#define JS_PROP_AUTOINIT       (3 << 4) /* used internally */

/* flags for JS_DefineProperty */
#define JS_PROP_HAS_SHIFT        8
#define JS_PROP_HAS_CONFIGURABLE (1 << 8)
#define JS_PROP_HAS_WRITABLE     (1 << 9)
#define JS_PROP_HAS_ENUMERABLE   (1 << 10)
#define JS_PROP_HAS_GET          (1 << 11)
#define JS_PROP_HAS_SET          (1 << 12)
#define JS_PROP_HAS_VALUE        (1 << 13)

/* throw an exception if false would be returned
   (JS_DefineProperty/JS_SetProperty) */
#define JS_PROP_THROW            (1 << 14)
   /* throw an exception if false would be returned in strict mode
      (JS_SetProperty) */
#define JS_PROP_THROW_STRICT     (1 << 15)

#define JS_PROP_NO_ADD           (1 << 16) /* internal use */
#define JS_PROP_NO_EXOTIC        (1 << 17) /* internal use */

typedef struct JSProperty {
    union {
        JSValue value;      /* JS_PROP_NORMAL */
        struct {            /* JS_PROP_GETSET */
            JSObject* getter; /* NULL if undefined */
            JSObject* setter; /* NULL if undefined */
        } getset;
        JSVarRef* var_ref;  /* JS_PROP_VARREF */
        struct {            /* JS_PROP_AUTOINIT */
            int (*init_func)(JSContext* ctx, JSObject* obj,
                JSAtom prop, void* opaque);
            void* opaque;
        } init;
    } u;
} JSProperty;
class JSObject :public RecyclableObject {
	friend class JSRuntime;
    friend class JSContext;
protected:
    JSGCHeader gc_header; /* must come after JSRefCountHeader, 8-bit */
    uint8_t extensible : 1;
    uint8_t free_mark : 1; /* only used when freeing objects with cycles */
    uint8_t is_exotic : 1; /* TRUE if object has exotic property handlers */
    uint8_t fast_array : 1; /* TRUE if u.array is used for get/put */
    uint8_t is_constructor : 1; /* TRUE if object is a constructor function */
    uint8_t is_uncatchable_error : 1; /* if TRUE, error is not catchable */
    uint8_t is_class : 1; /* TRUE if object is a class constructor */
    uint8_t tmp_mark : 1; /* used in JS_WriteObjectRec() */
    uint16_t class_id; /* see JS_CLASS_x */
    /* byte offsets: 8/8 */
    struct list_head link; /* object list */
    /* byte offsets: 16/24 */
    JSShape* shape; /* prototype and property names + flag */
    JSProperty* prop; /* array of properties */
    /* byte offsets: 24/40 */
    struct JSMapRecord* first_weak_ref; /* XXX: use a bit and an external hash table? */
    /* byte offsets: 28/48 */
    union {
        void* opaque;
        struct JSBoundFunction* bound_function; /* JS_CLASS_BOUND_FUNCTION */
        struct JSCFunctionDataRecord* c_function_data_record; /* JS_CLASS_C_FUNCTION_DATA */
        struct JSForInIterator* for_in_iterator; /* JS_CLASS_FOR_IN_ITERATOR */
        struct JSArrayBuffer* array_buffer; /* JS_CLASS_ARRAY_BUFFER, JS_CLASS_SHARED_ARRAY_BUFFER */
        struct JSTypedArray* typed_array; /* JS_CLASS_UINT8C_ARRAY..JS_CLASS_DATAVIEW */
#ifdef CONFIG_BIGNUM
        struct JSFloatEnv* float_env; /* JS_CLASS_FLOAT_ENV */
#endif
        struct JSMapState* map_state;   /* JS_CLASS_MAP..JS_CLASS_WEAKSET */
        struct JSMapIteratorData* map_iterator_data; /* JS_CLASS_MAP_ITERATOR, JS_CLASS_SET_ITERATOR */
        struct JSArrayIteratorData* array_iterator_data; /* JS_CLASS_ARRAY_ITERATOR, JS_CLASS_STRING_ITERATOR */
        struct JSGeneratorData* generator_data; /* JS_CLASS_GENERATOR */
        struct JSProxyData* proxy_data; /* JS_CLASS_PROXY */
        struct JSPromiseData* promise_data; /* JS_CLASS_PROMISE */
        struct JSPromiseFunctionData* promise_function_data; /* JS_CLASS_PROMISE_RESOLVE_FUNCTION, JS_CLASS_PROMISE_REJECT_FUNCTION */
        struct JSAsyncFunctionData* async_function_data; /* JS_CLASS_ASYNC_FUNCTION_RESOLVE, JS_CLASS_ASYNC_FUNCTION_REJECT */
        struct JSAsyncFromSyncIteratorData* async_from_sync_iterator_data; /* JS_CLASS_ASYNC_FROM_SYNC_ITERATOR */
        struct JSAsyncGeneratorData* async_generator_data; /* JS_CLASS_ASYNC_GENERATOR */
        struct { /* JS_CLASS_BYTECODE_FUNCTION: 12/24 bytes */
            /* also used by JS_CLASS_GENERATOR_FUNCTION, JS_CLASS_ASYNC_FUNCTION and JS_CLASS_ASYNC_GENERATOR_FUNCTION */
            struct JSFunctionBytecode* function_bytecode;
            JSVarRef** var_refs;
            JSObject* home_object; /* for 'super' access */
        } func;
        struct { /* JS_CLASS_C_FUNCTION: 8/12 bytes */
            JSCFunctionType c_function;
            uint8_t length;
            uint8_t cproto;
            int16_t magic;
        } cfunc;
        /* array part for fast arrays and typed arrays */
        struct { /* JS_CLASS_ARRAY, JS_CLASS_ARGUMENTS, JS_CLASS_UINT8C_ARRAY..JS_CLASS_FLOAT64_ARRAY */
            union {
                uint32_t size;          /* JS_CLASS_ARRAY, JS_CLASS_ARGUMENTS */
                struct JSTypedArray* typed_array; /* JS_CLASS_UINT8C_ARRAY..JS_CLASS_FLOAT64_ARRAY */
            } u1;
            union {
                JSValue* values;        /* JS_CLASS_ARRAY, JS_CLASS_ARGUMENTS */
                void* ptr;              /* JS_CLASS_UINT8C_ARRAY..JS_CLASS_FLOAT64_ARRAY */
                int8_t* int8_ptr;       /* JS_CLASS_INT8_ARRAY */
                uint8_t* uint8_ptr;     /* JS_CLASS_UINT8_ARRAY, JS_CLASS_UINT8C_ARRAY */
                int16_t* int16_ptr;     /* JS_CLASS_INT16_ARRAY */
                uint16_t* uint16_ptr;   /* JS_CLASS_UINT16_ARRAY */
                int32_t* int32_ptr;     /* JS_CLASS_INT32_ARRAY */
                uint32_t* uint32_ptr;   /* JS_CLASS_UINT32_ARRAY */
                int64_t* int64_ptr;     /* JS_CLASS_INT64_ARRAY */
                uint64_t* uint64_ptr;   /* JS_CLASS_UINT64_ARRAY */
                float* float_ptr;       /* JS_CLASS_FLOAT32_ARRAY */
                double* double_ptr;     /* JS_CLASS_FLOAT64_ARRAY */
            } u;
            uint32_t count; /* <= 2^31-1. 0 for a detached typed array */
        } array;    /* 12/20 bytes */
        JSRegExp regexp;    /* JS_CLASS_REGEXP: 8/16 bytes */
        JSValue object_data;    /* for JS_SetObjectData(): 8/16/16 bytes */
    } u;
    /* byte sizes: 40/48/72 */
};

static uint32_t shape_initial_hash(JSObject* proto)
{
    uint32_t h;
    h = shape_hash(1, (uintptr_t)proto);
    if (sizeof(proto) > 4)
        h = shape_hash(h, (uint64_t)(uintptr_t)proto >> 32);
    return h;
}
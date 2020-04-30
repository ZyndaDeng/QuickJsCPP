#include "JSRuntime.h"
#include "JSString.h"
#include "JSObject.h"

void JSRuntime::js_free_rt(void* ptr)
{
    mf.js_free(&this->malloc_state, ptr);
}

void JSRuntime::__JS_FreeValueRT(JSValue v)
{
    uint32_t tag = JS_VALUE_GET_TAG(v);

#ifdef DUMP_FREE
    {
        printf("Freeing ");
        if (tag == JS_TAG_OBJECT) {
            JS_DumpObject(rt, JS_VALUE_GET_OBJ(v));
        }
        else {
            JS_DumpValueShort(rt, v);
            printf("\n");
        }
    }
#endif

    switch (tag) {
    case JS_TAG_STRING:
    {
        JSString* p = JS_VALUE_GET_STRING(v);
        if (p->atom_type) {
            JS_FreeAtomStruct( p);
        }
        else {
#ifdef DUMP_LEAKS
            list_del(&p->link);
#endif
            js_free_rt( p);
        }
    }
    break;
    case JS_TAG_OBJECT:
        free_object(JS_VALUE_GET_OBJ(v));
        break;
    case JS_TAG_FUNCTION_BYTECODE:
        free_function_bytecode( JS_VALUE_GET_PTR(v));
        break;
    case JS_TAG_SHAPE:
    case JS_TAG_ASYNC_FUNCTION:
    case JS_TAG_VAR_REF:
    case JS_TAG_MODULE:
        abort(); /* never freed here */
        break;
#ifdef CONFIG_BIGNUM
    case JS_TAG_BIG_INT:
    case JS_TAG_BIG_FLOAT:
    {
        JSBigFloat* bf = JS_VALUE_GET_PTR(v);
        bf_delete(&bf->num);
        js_free_rt(rt, bf);
    }
    break;
#endif
    case JS_TAG_SYMBOL:
    {
        JSAtomStruct* p = JS_VALUE_GET_PTR(v);
        JS_FreeAtomStruct(p);
    }
    break;
    default:
        printf("__JS_FreeValue: unknown tag=%d\n", tag);
        abort();
    }
}

void JSRuntime::JS_FreeAtomStruct(JSAtomStruct* p)
{
#if 0   /* JS_ATOM_NULL is not refcounted: __JS_AtomIsConst() includes 0 */
    if (unlikely(i == JS_ATOM_NULL)) {
        p->header.ref_count = INT32_MAX / 2;
        return;
    }
#endif
    uint32_t i = p->hash_next;  /* atom_index */
    if (p->atom_type != JS_ATOM_TYPE_SYMBOL) {
        JSAtomStruct* p0, * p1;
        uint32_t h0;

        h0 = p->hash & (atom_hash_size - 1);
        i = atom_hash[h0];
        p1 = atom_array[i];
        if (p1 == p) {
           atom_hash[h0] = p1->hash_next;
        }
        else {
            for (;;) {
                assert(i != 0);
                p0 = p1;
                i = p1->hash_next;
                p1 = atom_array[i];
                if (p1 == p) {
                    p0->hash_next = p1->hash_next;
                    break;
                }
            }
        }
    }
    /* insert in free atom list */
    atom_array[i] = atom_set_free(rt->atom_free_index);
    atom_free_index = i;
    /* free the string structure */
#ifdef DUMP_LEAKS
    list_del(&p->link);
#endif
    js_free_rt( p);
    rt->atom_count--;
    assert(rt->atom_count >= 0);
}

void JSRuntime::free_object(JSObject* p)
{
}

void JSRuntime::free_object2(JSObject* p)
{
}

void JSRuntime::free_object_struct( JSObject* p)
{
    int i;
    JSClassFinalizer* finalizer;
    JSShape* sh;
    JSShapeProperty* pr;

    /* free all the fields */
    sh = p->shape;
    pr = get_shape_prop(sh);
    for (i = 0; i < sh->prop_count; i++) {
        free_property(rt, &p->prop[i], pr->flags);
        pr++;
    }
    js_free_rt( p->prop);
    js_free_shape(rt, sh);

    /* fail safe */
    p->shape = NULL;
    p->prop = NULL;

    if (unlikely(p->first_weak_ref)) {
        reset_weak_ref(rt, p);
    }

    finalizer = rt->class_array[p->class_id].finalizer;
    if (finalizer)
        (*finalizer)(rt, JS_MKPTR(JS_TAG_OBJECT, p));

    /* fail safe */
    p->class_id = 0;
    p->u.opaque = NULL;
    p->u.func.var_refs = NULL;
    p->u.func.home_object = NULL;
}

JSShape* JSRuntime::find_hashed_shape_proto(JSObject* proto)
{
    JSShape* sh1;
    uint32_t h, h1;

    h = shape_initial_hash(proto);
    h1 = get_shape_hash(h, shape_hash_bits);
    for (sh1 = shape_hash[h1]; sh1 != NULL; sh1 = sh1->shape_hash_next) {
        if (sh1->hash == h &&
            sh1->proto == proto &&
            sh1->prop_count == 0) {
            return sh1;
        }
    }
    return NULL;
}

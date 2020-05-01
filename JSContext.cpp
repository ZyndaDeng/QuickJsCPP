#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
//#include <sys/time.h>
#include <time.h>
#include <fenv.h>
#include <math.h>
#if defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__linux__)
#include <malloc.h>
#endif

#include "JSContext.h"
#include "JSRuntime.h"
#include "JSString.h"
#include "JSObject.h"
#include "JSClass.h"


typedef union JSFloat64Union {
    double d;
    uint64_t u64;
    uint32_t u32[2];
} JSFloat64Union;

#define HINT_STRING  0
#define HINT_NUMBER  1
#define HINT_NONE    2
#ifdef CONFIG_BIGNUM
#define HINT_INTEGER 3
#endif
/* don't try Symbol.toPrimitive */
#define HINT_FORCE_ORDINARY (1 << 4)

#ifdef CONFIG_BIGNUM
typedef struct JSFloatEnv {
    limb_t prec;
    bf_flags_t flags;
    unsigned int status;
} JSFloatEnv;

/* the same structure is used for big integers and big floats. Big
   integers are never infinite or NaNs */
typedef struct JSBigFloat {
    JSRefCountHeader header; /* must come first, 32-bit */
    bf_t num;
} JSBigFloat;
#endif

inline void JSContext::JS_FreeValue( JSValue v)
{
    if (JS_VALUE_HAS_REF_COUNT(v)) {
        JSRefCountHeader* p = &(JS_VALUE_GET_GC(v))->header;
        if (--p->ref_count <= 0) {
            rt->__JS_FreeValueRT(v);
        }
    }
}

int JSContext::JS_ToInt32(int32_t* pres, JSValueConst val)
{
    return JS_ToInt32Free( pres, JS_DupValue( val));
}

int JSContext::JS_ToInt64(int64_t* pres, JSValueConst val)
{
    return JS_ToInt64Free( pres, JS_DupValue( val));
}

#define MAX_SAFE_INTEGER (((int64_t)1 << 53) - 1)
int JSContext::JS_ToIndex(uint64_t* plen, JSValueConst val)
{
    int64_t v;
    if (JS_ToInt64Sat( &v, val))
        return -1;
    if (v < 0 || v > MAX_SAFE_INTEGER) {
        JS_ThrowRangeError( "invalid array index");
        *plen = 0;
        return -1;
    }
    *plen = v;
    return 0;
}

int JSContext::JS_ToFloat64(double* pres, JSValueConst val)
{
    return 0;
}

int JSContext::JS_ToBoolFree(JSValueConst val)
{
    uint32_t tag = JS_VALUE_GET_TAG(val);
    switch (tag) {
    case JS_TAG_INT:
        return JS_VALUE_GET_INT(val) != 0;
    case JS_TAG_BOOL:
    case JS_TAG_NULL:
    case JS_TAG_UNDEFINED:
        return JS_VALUE_GET_INT(val);
    case JS_TAG_EXCEPTION:
        return -1;
    case JS_TAG_STRING:
    {
        BOOL ret = JS_VALUE_GET_STRING(val)->len != 0;
        JS_FreeValue( val);
        return ret;
    }
#ifdef CONFIG_BIGNUM
    case JS_TAG_BIG_INT:
    case JS_TAG_BIG_FLOAT:
    {
        JSBigFloat* p = (JSBigFloat*)JS_VALUE_GET_PTR(val);
        BOOL ret;
        ret = p->num.expn != BF_EXP_ZERO && p->num.expn != BF_EXP_NAN;
        JS_FreeValue( val);
        return ret;
    }
#endif
    default:
        if (JS_TAG_IS_FLOAT64(tag)) {
            double d = JS_VALUE_GET_FLOAT64(val);
            return !isnan(d) && d != 0;
        }
        else {
            JS_FreeValue( val);
            return TRUE;
        }
    }
}
int JSContext::JS_ToInt64Free(int64_t* pres, JSValue val)
{
    uint32_t tag;
    int64_t ret;

 redo:
    tag = JS_VALUE_GET_NORM_TAG(val);
    switch(tag) {
    case JS_TAG_INT:
    case JS_TAG_BOOL:
    case JS_TAG_NULL:
    case JS_TAG_UNDEFINED:
        ret = JS_VALUE_GET_INT(val);
        break;
    case JS_TAG_FLOAT64:
        {
            JSFloat64Union u;
            double d;
            int e;
            d = JS_VALUE_GET_FLOAT64(val);
            u.d = d;
            /* we avoid doing fmod(x, 2^64) */
            e = (u.u64 >> 52) & 0x7ff;
            if (likely(e <= (1023 + 62))) {
                /* fast case */
                ret = (int64_t)d;
            } else if (e <= (1023 + 62 + 53)) {
                uint64_t v;
                /* remainder modulo 2^64 */
                v = (u.u64 & (((uint64_t)1 << 52) - 1)) | ((uint64_t)1 << 52);
                ret = v << ((e - 1023) - 52);
                /* take the sign into account */
                if (u.u64 >> 63)
                    ret = -ret;
            } else {
                ret = 0; /* also handles NaN and +inf */
            }
        }
        break;
#ifdef CONFIG_BIGNUM
    case JS_TAG_BIG_FLOAT:
    to_bf:
        {
            JSBigFloat *p = JS_VALUE_GET_PTR(val);
            bf_get_int64(&ret, &p->num, BF_GET_INT_MOD);
            JS_FreeValue(ctx, val);
        }
        break;
    case JS_TAG_BIG_INT:
        if (is_bignum_mode(ctx))
            goto to_bf;
        /* fall thru */
#endif
    default:
        val = JS_ToNumberFree( val);
        if (JS_IsException(val)) {
            *pres = 0;
            return -1;
        }
        goto redo;
    }
    *pres = ret;
    return 0;
}

int JSContext::JS_ToInt64SatFree(int64_t *pres, JSValue val){
    uint32_t tag;

 redo:
    tag = JS_VALUE_GET_NORM_TAG(val);
    switch(tag) {
    case JS_TAG_INT:
    case JS_TAG_BOOL:
    case JS_TAG_NULL:
    case JS_TAG_UNDEFINED:
        *pres = JS_VALUE_GET_INT(val);
        return 0;
    case JS_TAG_EXCEPTION:
        *pres = 0;
        return -1;
    case JS_TAG_FLOAT64:
        {
            double d = JS_VALUE_GET_FLOAT64(val);
            if (isnan(d)) {
                *pres = 0;
            } else {
                if (d < INT64_MIN)
                    *pres = INT64_MIN;
                else if (d > INT64_MAX)
                    *pres = INT64_MAX;
                else
                    *pres = (int64_t)d;
            }
        }
        return 0;
#ifdef CONFIG_BIGNUM
    case JS_TAG_BIG_FLOAT:
    to_bf:
        {
            JSBigFloat *p = JS_VALUE_GET_PTR(val);
            bf_get_int64(pres, &p->num, 0);
            JS_FreeValue(ctx, val);
        }
        return 0;
    case JS_TAG_BIG_INT:
        if (is_bignum_mode(ctx))
            goto to_bf;
        /* fall thru */
#endif
    default:
        val = JS_ToNumberFree( val);
        if (JS_IsException(val)) {
            *pres = 0;
            return -1;
        }
        goto redo;
    }
}

int JSContext::JS_ToInt32Free(int32_t* pres, JSValue val)
{
    uint32_t tag;
    int32_t ret;

redo:
    tag = JS_VALUE_GET_NORM_TAG(val);
    switch (tag) {
    case JS_TAG_INT:
    case JS_TAG_BOOL:
    case JS_TAG_NULL:
    case JS_TAG_UNDEFINED:
        ret = JS_VALUE_GET_INT(val);
        break;
    case JS_TAG_FLOAT64:
    {
        JSFloat64Union u;
        double d;
        int e;
        d = JS_VALUE_GET_FLOAT64(val);
        u.d = d;
        /* we avoid doing fmod(x, 2^32) */
        e = (u.u64 >> 52) & 0x7ff;
        if (likely(e <= (1023 + 30))) {
            /* fast case */
            ret = (int32_t)d;
        }
        else if (e <= (1023 + 30 + 53)) {
            uint64_t v;
            /* remainder modulo 2^32 */
            v = (u.u64 & (((uint64_t)1 << 52) - 1)) | ((uint64_t)1 << 52);
            v = v << ((e - 1023) - 52 + 32);
            ret = v >> 32;
            /* take the sign into account */
            if (u.u64 >> 63)
                ret = -ret;
        }
        else {
            ret = 0; /* also handles NaN and +inf */
        }
    }
    break;
#ifdef CONFIG_BIGNUM
    case JS_TAG_BIG_FLOAT:
    to_bf:
    {
        JSBigFloat* p = (JSBigFloat*)JS_VALUE_GET_PTR(val);
        bf_get_int32(&ret, &p->num, BF_GET_INT_MOD);
        JS_FreeValue( val);
    }
    break;
    case JS_TAG_BIG_INT:
        if (is_bignum_mode(ctx))
            goto to_bf;
        /* fall thru */
#endif
    default:
        val = JS_ToNumberFree( val);
        if (JS_IsException(val)) {
            *pres = 0;
            return -1;
        }
        goto redo;
    }
    *pres = ret;
    return 0;
}
#ifdef CONFIG_BIGNUM
int JSContext::JS_ToBigInt64Free(int64_t* pres, JSValue val)
{
    bf_t a_s, * a;

    a = JS_ToBigIntFree( &a_s, val);
    if (!a) {
        *pres = 0;
        return -1;
    }
    bf_get_int64(pres, a, BF_GET_INT_MOD);
    JS_FreeBigInt( a, &a_s);
    return 0;
}

void JSContext::JS_FreeBigInt(bf_t* a, bf_t* buf)
{
}

bf_t* JSContext::JS_ToBigIntFree( bf_t* buf, JSValue val)
{
    uint32_t tag;
    bf_t* r;
    JSBigFloat* p;

redo:
    tag = JS_VALUE_GET_NORM_TAG(val);
    switch (tag) {
    case JS_TAG_INT:
    case JS_TAG_NULL:
    case JS_TAG_UNDEFINED:
        if (!is_bignum_mode(ctx))
            goto fail;
        /* fall tru */
    case JS_TAG_BOOL:
        r = buf;
        bf_init(ctx->bf_ctx, r);
        bf_set_si(r, JS_VALUE_GET_INT(val));
        break;
    case JS_TAG_FLOAT64:
    {
        double d = JS_VALUE_GET_FLOAT64(val);
        if (!is_bignum_mode(ctx))
            goto fail;
        if (!isfinite(d))
            goto fail;
        r = buf;
        bf_init(ctx->bf_ctx, r);
        d = trunc(d);
        bf_set_float64(r, d);
    }
    break;
    case JS_TAG_BIG_INT:
        p = JS_VALUE_GET_PTR(val);
        r = &p->num;
        break;
    case JS_TAG_BIG_FLOAT:
        if (!is_bignum_mode(ctx))
            goto fail;
        p = JS_VALUE_GET_PTR(val);
        if (!bf_is_finite(&p->num))
            goto fail;
        r = buf;
        bf_init(ctx->bf_ctx, r);
        bf_set(r, &p->num);
        bf_rint(r, BF_PREC_INF, BF_RNDZ);
        JS_FreeValue( val);
        break;
    case JS_TAG_STRING:
        val = JS_StringToBigIntErr(ctx, val);
        if (JS_IsException(val))
            return NULL;
        goto redo;
    case JS_TAG_OBJECT:
        val = JS_ToPrimitiveFree(ctx, val,
            is_bignum_mode(ctx) ? HINT_INTEGER : HINT_NUMBER);
        if (JS_IsException(val))
            return NULL;
        goto redo;
    default:
    fail:
        JS_FreeValue( val);
        JS_ThrowTypeError( "cannot convert to bigint");
        return NULL;
    }
    return r;
}
#endif

JSValue JSContext::JS_ToNumberHintFree(JSValue val, JSToNumberHintEnum flag)
{
    uint32_t tag;
    JSValue ret;
    int hint;

redo:
    tag = JS_VALUE_GET_NORM_TAG(val);
    switch (tag) {
#ifdef CONFIG_BIGNUM
    case JS_TAG_BIG_INT:
        if (flag == TON_FLAG_NUMBER && !is_bignum_mode(ctx)) {
            JS_FreeValue(ctx, val);
            return JS_ThrowTypeError(ctx, "cannot convert bigint to number");
        }
        /* fall thru */
    case JS_TAG_BIG_FLOAT:
#endif
    case JS_TAG_FLOAT64:
    case JS_TAG_INT:
    case JS_TAG_EXCEPTION:
        ret = val;
        break;
    case JS_TAG_BOOL:
    case JS_TAG_NULL:
        ret = JS_NewInt32( JS_VALUE_GET_INT(val));
        break;
    case JS_TAG_UNDEFINED:
        ret = JS_NAN;
        break;
    case JS_TAG_OBJECT:
#ifdef CONFIG_BIGNUM
        hint = flag == TON_FLAG_INTEGER ? HINT_INTEGER : HINT_NUMBER;
#else
        hint = HINT_NUMBER;
#endif
        val = JS_ToPrimitiveFree( val, hint);
        if (JS_IsException(val))
            return JS_EXCEPTION;
        goto redo;
    case JS_TAG_STRING:
    {
        const char* str;
        const char* p;

        str = JS_ToCString( val);
        JS_FreeValue( val);
        if (!str)
            return JS_EXCEPTION;
#ifdef CONFIG_BIGNUM
        {
            int flags;
            flags = BF_ATOF_BIN_OCT | BF_ATOF_NO_PREFIX_AFTER_SIGN |
                BF_ATOF_JS_QUIRKS | BF_ATOF_FLOAT64;
            if (is_bignum_mode(ctx))
                flags |= BF_ATOF_INT_PREC_INF;
            else
                flags |= BF_ATOF_ONLY_DEC_FLOAT;
            ret = js_atof(ctx, str, &p, 0, flags);
        }
#else
        ret = js_atod(ctx, str, &p, 0, ATOD_ACCEPT_BIN_OCT);
#endif
        p += skip_spaces(p);
        if (*p != '\0') {
            JS_FreeValue( ret);
            ret = JS_NAN;
        }
        JS_FreeCString( str);
    }
    break;
    case JS_TAG_SYMBOL:
        JS_FreeValue( val);
        return JS_ThrowTypeError( "cannot convert symbol to number");
    default:
        JS_FreeValue( val);
        ret = JS_NAN;
        break;
    }
    return ret;
}

JSValue JSContext::JS_ToPrimitiveFree(JSValue val, int hint)
{
    int i;
    BOOL force_ordinary;

    JSAtom method_name;
    JSValue method, ret;
    if (JS_VALUE_GET_TAG(val) != JS_TAG_OBJECT)
        return val;
    force_ordinary = hint & HINT_FORCE_ORDINARY;
    hint &= ~HINT_FORCE_ORDINARY;
    if (!force_ordinary) {
        method = JS_GetProperty(val, JS_ATOM_Symbol_toPrimitive);
        if (JS_IsException(method))
            goto exception;
        /* ECMA says *If exoticToPrim is not undefined* but tests in
           test262 use null as a non callable converter */
        if (!JS_IsUndefined(method) && !JS_IsNull(method)) {
            JSAtom atom;
            JSValue arg;
            switch (hint) {
            case HINT_STRING:
                atom = JS_ATOM_string;
                break;
            case HINT_NUMBER:
                atom = JS_ATOM_number;
                break;
            default:
            case HINT_NONE:
                atom = JS_ATOM_default;
                break;
#ifdef CONFIG_BIGNUM
            case HINT_INTEGER:
                atom = JS_ATOM_integer;
                break;
#endif
            }
            arg = JS_AtomToString( atom);
            ret = JS_CallFree( method, val, 1, (JSValueConst*)&arg);
            JS_FreeValue( arg);
            if (JS_IsException(ret))
                goto exception;
            JS_FreeValue( val);
            if (JS_VALUE_GET_TAG(ret) != JS_TAG_OBJECT)
                return ret;
            JS_FreeValue( ret);
            return JS_ThrowTypeError( "toPrimitive");
        }
    }
    if (hint != HINT_STRING)
        hint = HINT_NUMBER;
    for (i = 0; i < 2; i++) {
        if ((i ^ hint) == 0) {
            method_name = JS_ATOM_toString;
        }
        else {
            method_name = JS_ATOM_valueOf;
        }
        method = JS_GetProperty( val, method_name);
        if (JS_IsException(method))
            goto exception;
        if (JS_IsFunction( method)) {
            ret = JS_CallFree( method, val, 0, NULL);
            if (JS_IsException(ret))
                goto exception;
            if (JS_VALUE_GET_TAG(ret) != JS_TAG_OBJECT) {
                JS_FreeValue( val);
                return ret;
            }
            JS_FreeValue( ret);
        }
        else {
            JS_FreeValue( method);
        }
    }
    JS_ThrowTypeError( "toPrimitive");
exception:
    JS_FreeValue( val);
    return JS_EXCEPTION;
}

no_inline JSShape* JSContext::js_new_shape2(JSObject* proto, int hash_size, int prop_size)
{
    JSRuntime* rt = rt;
    void* sh_alloc;
    JSShape* sh;

    /* resize the shape hash table if necessary */
    if (2 * (rt->shape_hash_count + 1) > rt->shape_hash_size) {
        rt->resize_shape_hash( rt->shape_hash_bits + 1);
    }

    sh_alloc = js_malloc( get_shape_size(hash_size, prop_size));
    if (!sh_alloc)
        return NULL;
    sh = get_shape_from_alloc(sh_alloc, hash_size);
    sh->header.ref_count = 1;
    sh->gc_header.mark = 0;
    if (proto)
        JS_DupValue( JS_MKPTR(JS_TAG_OBJECT, proto));
    sh->proto = proto;
    memset(sh->prop_hash_end - hash_size, 0, sizeof(sh->prop_hash_end[0]) *
        hash_size);
    sh->prop_hash_mask = hash_size - 1;
    sh->prop_count = 0;
    sh->prop_size = prop_size;

    /* insert in the hash table */
    sh->hash = shape_initial_hash(proto);
    sh->is_hashed = TRUE;
    sh->has_small_array_index = FALSE;
    rt->js_shape_hash_link( sh);
    return sh;
}

void* JSContext::js_malloc(size_t size)
{
    void* ptr;
    ptr = rt->js_malloc_rt( size);
    if (unlikely(!ptr)) {
        JS_ThrowOutOfMemory();
        return NULL;
    }
    return ptr;
}

void JSContext::js_free(void* ptr)
{
    rt->js_free_rt(ptr);
}

JSValue JSContext::JS_ThrowError(JSErrorEnum error_num, const char* fmt, va_list ap)
{
    char buf[256];
    JSValue obj, ret;

    vsnprintf(buf, sizeof(buf), fmt, ap);
    obj = JS_NewObjectProtoClass(native_error_proto[error_num],
        JS_CLASS_ERROR);
    if (unlikely(JS_IsException(obj))) {
        /* out of memory: throw JS_NULL to avoid recursing */
        obj = JS_NULL;
    }
    else {
        JS_DefinePropertyValue( obj, JS_ATOM_message,
            JS_NewString( buf),
            JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    }
    ret = JS_Throw( obj);
    return ret;
}

JSValue JSContext::JS_NewObjectFromShape(JSShape* sh, JSClassID class_id)
{
    JSObject* p;

   rt-> js_trigger_gc( sizeof(JSObject));
    p = (JSObject*)js_malloc( sizeof(JSObject));
    if (unlikely(!p))
        goto fail;
    p->header.ref_count = 1;
    p->gc_header.mark = 0;
    p->class_id = class_id;
    p->extensible = TRUE;
    p->free_mark = 0;
    p->is_exotic = 0;
    p->fast_array = 0;
    p->is_constructor = 0;
    p->is_uncatchable_error = 0;
    p->is_class = 0;
    p->tmp_mark = 0;
    p->first_weak_ref = NULL;
    p->u.opaque = NULL;
    p->shape = sh;
    p->prop = (JSProperty*)js_malloc(sizeof(JSProperty) * sh->prop_size);
    if (unlikely(!p->prop)) {
        js_free( p);
    fail:
       rt-> js_free_shape( sh);
        return JS_EXCEPTION;
    }

    switch (class_id) {
    case JS_CLASS_OBJECT:
        break;
    case JS_CLASS_ARRAY:
    {
        JSProperty* pr;
        p->is_exotic = 1;
        p->fast_array = 1;
        p->u.array.u.values = NULL;
        p->u.array.count = 0;
        p->u.array.u1.size = 0;
        /* the length property is always the first one */
        if (likely(sh == array_shape)) {
            pr = &p->prop[0];
        }
        else {
            /* only used for the first array */
            /* cannot fail */
            pr = add_property( p, JS_ATOM_length,
                JS_PROP_WRITABLE | JS_PROP_LENGTH);
        }
        pr->u.value = JS_NewInt32( 0);
    }
    break;
    case JS_CLASS_C_FUNCTION:
        p->prop[0].u.value = JS_UNDEFINED;
        break;
    case JS_CLASS_ARGUMENTS:
    case JS_CLASS_UINT8C_ARRAY:
    case  JS_CLASS_INT8_ARRAY:
    case   JS_CLASS_UINT8_ARRAY:
    case JS_CLASS_INT16_ARRAY:
    case  JS_CLASS_UINT16_ARRAY:
    case  JS_CLASS_INT32_ARRAY:
    case  JS_CLASS_UINT32_ARRAY:
#ifdef CONFIG_BIGNUM
    case    JS_CLASS_BIG_INT64_ARRAY:
    case      JS_CLASS_BIG_UINT64_ARRAY:
#endif
    case   JS_CLASS_FLOAT32_ARRAY:
    case    JS_CLASS_FLOAT64_ARRAY:
        p->is_exotic = 1;
        p->fast_array = 1;
        p->u.array.u.ptr = NULL;
        p->u.array.count = 0;
        break;
    case JS_CLASS_DATAVIEW:
        p->u.array.u.ptr = NULL;
        p->u.array.count = 0;
        break;
    case JS_CLASS_NUMBER:
    case JS_CLASS_STRING:
    case JS_CLASS_BOOLEAN:
    case JS_CLASS_SYMBOL:
    case JS_CLASS_DATE:
#ifdef CONFIG_BIGNUM
    case JS_CLASS_BIG_INT:
    case JS_CLASS_BIG_FLOAT:
#endif
        p->u.object_data = JS_UNDEFINED;
        goto set_exotic;
    case JS_CLASS_REGEXP:
        p->u.regexp.pattern = NULL;
        p->u.regexp.bytecode = NULL;
        goto set_exotic;
    default:
    set_exotic:
        if (rt->class_array[class_id].exotic) {
            p->is_exotic = 1;
        }
        break;
    }
    list_add_tail(&p->link, &rt->obj_list);
    return JS_MKPTR(JS_TAG_OBJECT, p);
}

JSValue JSContext::JS_Throw(JSValue obj)
{
    JS_FreeValue(current_exception);
    current_exception = obj;
    exception_needs_backtrace = JS_IsError( obj);
    return JS_EXCEPTION;
}

JSValue JSContext::JS_GetException()
{
    JSValue val;
    val = current_exception;
   current_exception = JS_NULL;
    exception_needs_backtrace = FALSE;
    return val;
}

JS_BOOL JSContext::JS_IsError(JSValueConst val)
{
    JSObject* p;
    if (JS_VALUE_GET_TAG(val) != JS_TAG_OBJECT)
        return FALSE;
    p = JS_VALUE_GET_OBJ(val);
    if (p->class_id == JS_CLASS_ERROR)
        return TRUE;
    if (is_error_property_enabled) {
        /* check for a special property for test262 test suites */
        JSValue isError;
        isError = JS_GetPropertyStr( val, "isError");
        return JS_ToBoolFree(isError);
    }
    else {
        return FALSE;
    }
}

/* only used for test262 test suites */
void JSContext::JS_EnableIsErrorProperty(JS_BOOL enable)
{
    is_error_property_enabled = enable;
}



void JSContext::JS_ResetUncatchableError()
{
    JS_SetUncatchableError( current_exception, FALSE);
}

JSValue JSContext::JS_NewError()
{
    return JS_NewObjectClass( JS_CLASS_ERROR);
}

JSValue __js_printf_like(2, 3) JSContext::JS_ThrowSyntaxError(const char* fmt, ...)
{
    JSValue val;
    va_list ap;

    va_start(ap, fmt);
    val = JS_ThrowError( JS_SYNTAX_ERROR, fmt, ap);
    va_end(ap);
    return val;
}

JSValue __js_printf_like(2, 3) JSContext::JS_ThrowTypeError(const char* fmt, ...)
{
    JSValue val;
    va_list ap;

    va_start(ap, fmt);
    val = JS_ThrowError( JS_TYPE_ERROR, fmt, ap);
    va_end(ap);
    return val;
}

JSValue __js_printf_like(2, 3) JSContext::JS_ThrowReferenceError(const char* fmt, ...)
{
    JSValue val;
    va_list ap;

    va_start(ap, fmt);
    val = JS_ThrowError( JS_REFERENCE_ERROR, fmt, ap);
    va_end(ap);
    return val;
}

JSValue __js_printf_like(2, 3) JSContext::JS_ThrowRangeError(const char* fmt, ...)
{
    JSValue val;
    va_list ap;

    va_start(ap, fmt);
    val = JS_ThrowError( JS_RANGE_ERROR, fmt, ap);
    va_end(ap);
    return val;
}

JSValue __js_printf_like(2, 3) JSContext::JS_ThrowInternalError(const char* fmt, ...)
{
    JSValue val;
    va_list ap;

    va_start(ap, fmt);
    val = JS_ThrowError( JS_INTERNAL_ERROR, fmt, ap);
    va_end(ap);
    return val;
}

JSValue JSContext::JS_ThrowOutOfMemory()
{
    if (!in_out_of_memory) {
        in_out_of_memory = TRUE;
        JS_ThrowInternalError( "out of memory");
        in_out_of_memory = FALSE;
    }
    return JS_EXCEPTION;
}

JSValue JSContext::JS_NewObjectProtoClass(JSValueConst proto_val, JSClassID class_id)
{
    JSShape* sh;
    JSObject* proto;

    proto = get_proto_obj(proto_val);
    sh = rt->find_hashed_shape_proto( proto);
    if (likely(sh)) {
        sh = js_dup_shape(sh);
    }
    else {
        sh = js_new_shape( proto);
        if (!sh)
            return JS_EXCEPTION;
    }
    return JS_NewObjectFromShape( sh, class_id);
}

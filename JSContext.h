#pragma once
#include "cutils.h"
#include "JSValue.h"
#include "RecyclableObject.h"
#include "list.h"
#include "JSError.h"
#include "JSShape.h"
#include "JSObject.h"
#include "libbf.h"

#if defined(__GNUC__) || defined(__clang__)
#define js_likely(x)          __builtin_expect(!!(x), 1)
#define js_unlikely(x)        __builtin_expect(!!(x), 0)
#define js_force_inline       inline __attribute__((always_inline))
#define __js_printf_like(f, a)   __attribute__((format(printf, f, a)))
#else
#define js_likely(x)     (x)
#define js_unlikely(x)   (x)
#define js_force_inline  inline
#define __js_printf_like(a, b)
#endif

class JSRuntime;
class JSObject;
class JSClass;
class JSString;
class JSShape;
class JSStackFrame;
typedef uint32_t JSClassID;
typedef uint32_t JSAtom;
class JSContext {
public:
	void JS_FreeContext();
    inline JSRuntime* getRuntime() {
        return rt;
    }
    inline void* JS_GetContextOpaque() {
        return user_opaque;
    }
    inline void JS_SetContextOpaque(void* opaque) {
        user_opaque = opaque;
    }
    inline void JS_SetMaxStackSize(size_t stack_size) {
        stack_size = stack_size;
    }
    //赋值-------------
    //释放JSValue
    inline void JS_FreeValue( JSValue v);
    //引用JSValue
    inline JSValue JS_DupValue( JSValueConst v)
    {
        if (JS_VALUE_HAS_REF_COUNT(v)) {
            JSRefCountHeader* p = &(JS_VALUE_GET_GC(v))->header;
            p->ref_count++;
        }
        return (JSValue)v;
    }

    inline int JS_ToBool(JSValueConst val) {
        return JS_ToBoolFree(JS_DupValue( val));
    }
    int JS_ToInt32( int32_t* pres, JSValueConst val);
    inline int JS_ToUint32( uint32_t* pres, JSValueConst val)
    {
        return JS_ToInt32((int32_t*)pres, val);
    }
    int JS_ToInt64( int64_t* pres, JSValueConst val);
    int JS_ToIndex( uint64_t* plen, JSValueConst val);
    int JS_ToFloat64( double* pres, JSValueConst val);
    js_force_inline  JSValue JS_NewInt32( int32_t val)
    {
        return JS_MKVAL(JS_TAG_INT, val);
    }
    js_force_inline JSValue JS_NewBool(JS_BOOL val)
    {
        return JS_MKVAL(JS_TAG_BOOL, val);
    }
    js_force_inline JSValue JS_NewCatchOffset( int32_t val)
    {
        return JS_MKVAL(JS_TAG_CATCH_OFFSET, val);
    }
    JSValue JS_NewStringLen( const char* str1, int len1);
    JSValue JS_NewString( const char* str);
    JSValue JS_NewAtomString( const char* str);
    JSValue JS_NewObjectClass( int class_id)
    {
        return JS_NewObjectProtoClass(class_proto[class_id], class_id);
    }
    JS_BOOL JS_IsFunction( JSValueConst val);
    JS_BOOL JS_IsConstructor( JSValueConst val);
    JSValue JS_ToString( JSValueConst val);
    JSValue JS_ToPropertyKey( JSValueConst val);
    const char* JS_ToCStringLen( int* plen, JSValueConst val1, JS_BOOL cesu8);
     inline const char* JS_ToCString( JSValueConst val1)
    {
        return JS_ToCStringLen( NULL, val1, 0);
    }
    void JS_FreeCString( const char* ptr);
    //TODO: property
    js_force_inline JSValue JS_GetProperty( JSValueConst this_obj,
        JSAtom prop)
    {
        return JS_GetPropertyInternal( this_obj, prop, this_obj, 0);
    }
    JSValue JS_GetPropertyStr( JSValueConst this_obj,
        const char* prop);
    JSValue JS_GetPropertyUint32( JSValueConst this_obj,
        uint32_t idx);
    inline int JS_SetProperty( JSValueConst this_obj,
        JSAtom prop, JSValue val)
    {
        return JS_SetPropertyInternal( this_obj, prop, val, JS_PROP_THROW);
    }
    int JS_SetPropertyUint32(JSValueConst this_obj,
        uint32_t idx, JSValue val);
    int JS_SetPropertyInt64(JSValueConst this_obj,
        int64_t idx, JSValue val);
    int JS_SetPropertyStr(JSValueConst this_obj,
        const char* prop, JSValue val);
    int JS_HasProperty( JSValueConst this_obj, JSAtom prop);
    int JS_IsExtensible( JSValueConst obj);
    int JS_PreventExtensions( JSValueConst obj);
    int JS_DeleteProperty( JSValueConst obj, JSAtom prop, int flags);
    int JS_SetPrototype( JSValueConst obj, JSValueConst proto_val);
    JSValueConst JS_GetPrototype( JSValueConst val);

    JSValue JS_GetGlobalObject();
    int JS_IsInstanceOf( JSValueConst val, JSValueConst obj);
    int JS_DefineProperty( JSValueConst this_obj,
        JSAtom prop, JSValueConst val,
        JSValueConst getter, JSValueConst setter, int flags);
    int JS_DefinePropertyValue( JSValueConst this_obj,
        JSAtom prop, JSValue val, int flags);
    int JS_DefinePropertyValueUint32( JSValueConst this_obj,
        uint32_t idx, JSValue val, int flags);
    int JS_DefinePropertyValueStr( JSValueConst this_obj,
        const char* prop, JSValue val, int flags);
    int JS_DefinePropertyGetSet( JSValueConst this_obj,
        JSAtom prop, JSValue getter, JSValue setter,
        int flags);
    //Error
    JSValue JS_Throw( JSValue obj);
    JSValue JS_GetException();
    JS_BOOL JS_IsError( JSValueConst val);
    void JS_EnableIsErrorProperty( JS_BOOL enable);
    void JS_ResetUncatchableError();
    JSValue JS_NewError();
    JSValue __js_printf_like(2, 3) JS_ThrowSyntaxError( const char* fmt, ...);
    JSValue __js_printf_like(2, 3) JS_ThrowTypeError( const char* fmt, ...);
    JSValue __js_printf_like(2, 3) JS_ThrowReferenceError( const char* fmt, ...);
    JSValue __js_printf_like(2, 3) JS_ThrowRangeError(const char* fmt, ...);
    JSValue __js_printf_like(2, 3) JS_ThrowInternalError( const char* fmt, ...);
    JSValue JS_ThrowOutOfMemory();
protected:
    JSValue JS_NewObjectProtoClass( JSValueConst proto_val,
        JSClassID class_id);
    //JSValue转换Bool
     int JS_ToBoolFree(JSValueConst val);
     int JS_ToInt32Free(int32_t* pres, JSValue val);
     int JS_ToBigInt64Free(int64_t* pres, JSValue val);
     bf_t* JS_ToBigIntFree( bf_t* buf, JSValue val);
     void JS_FreeBigInt( bf_t* a, bf_t* buf);
     enum class JSToNumberHintEnum {
         TON_FLAG_NUMBER,
         TON_FLAG_INTEGER,
         TON_FLAG_NUMERIC,
     };
     inline JSValue JS_ToNumberFree(JSValue val) {
         return JS_ToNumberHintFree( val, JSToNumberHintEnum::TON_FLAG_NUMBER);
     }
     JSValue JS_ToNumberHintFree( JSValue val,
         JSToNumberHintEnum flag);
     JSValue JS_ToPrimitiveFree(JSValue val, int hint);

     //property
     JSValue JS_GetPropertyInternal(JSValueConst obj,
         JSAtom prop, JSValueConst this_obj,
         BOOL throw_ref_error);
     int JS_SetPropertyInternal( JSValueConst this_obj,
         JSAtom prop, JSValue val,
         int flags);
   
     void JS_SetUncatchableError(JSValueConst val, BOOL flag)
     {
         JSObject* p;
         if (JS_VALUE_GET_TAG(val) != JS_TAG_OBJECT)
             return;
         p = JS_VALUE_GET_OBJ(val);
         if (p->class_id == JS_CLASS_ERROR)
             p->is_uncatchable_error = flag;
     }

     JSShape* js_new_shape(JSObject* proto)
     {
         return js_new_shape2( proto, JS_PROP_INITIAL_HASH_SIZE,
             JS_PROP_INITIAL_SIZE);
     }
     no_inline JSShape* js_new_shape2( JSObject* proto,
         int hash_size, int prop_size);
     void* js_malloc(size_t size);
     void js_free(void* ptr);
   
     JSValue JS_ThrowError( JSErrorEnum error_num,
         const char* fmt, va_list ap);
     JSValue JS_NewObjectFromShape( JSShape* sh, JSClassID class_id);
     JSProperty* add_property(JSObject* p, JSAtom prop, int prop_flags);
     inline JSValue JS_AtomToString(JSAtom atom) {
         return __JS_AtomToValue( atom, TRUE);
     }
     inline JSValue JS_AtomToValue(JSAtom atom){
         return __JS_AtomToValue(atom, FALSE);
     }
     JSValue __JS_AtomToValue( JSAtom atom, BOOL force_string);
     JSValue JS_CallFree( JSValue func_obj, JSValueConst this_obj,
         int argc, JSValueConst* argv);

    JSRuntime* rt;
    struct list_head link;
    const uint8_t* stack_top;
    size_t stack_size; /* in bytes */

    JSValue current_exception;
    /* true if a backtrace needs to be added to the current exception
       (the backtrace generation cannot be done immediately in a bytecode
       function) */
    BOOL exception_needs_backtrace : 8;
    /* true if inside an out of memory error, to avoid recursing */
    BOOL in_out_of_memory : 8;
    uint16_t binary_object_count;
    int binary_object_size;

    JSShape* array_shape;   /* initial shape for Array objects */

    JSStackFrame* current_stack_frame;

    JSValue* class_proto;
    JSValue function_proto;
    JSValue function_ctor;
    JSValue regexp_ctor;
    JSValue native_error_proto[JS_NATIVE_ERROR_COUNT];
    JSValue iterator_proto;
    JSValue async_iterator_proto;
    JSValue array_proto_values;
    JSValue throw_type_error;

    JSValue global_obj; /* global object */
    JSValue global_var_obj; /* contains the global let/const definitions */

    uint64_t random_state;
#ifdef CONFIG_BIGNUM
    bf_context_t* bf_ctx;   /* points to rt->bf_ctx, shared by all contexts */
    JSFloatEnv fp_env; /* global FP environment */
#endif
    /* when the counter reaches zero, JSRutime.interrupt_handler is called */
    int interrupt_counter;
    BOOL is_error_property_enabled;

    struct list_head loaded_modules; /* list of JSModuleDef.link */

    /* if NULL, RegExp compilation is not supported */
    JSValue(*compile_regexp)(JSContext* ctx, JSValueConst pattern,
        JSValueConst flags);
    /* if NULL, eval is not supported */
    JSValue(*eval_internal)(JSContext* ctx, JSValueConst this_obj,
        const char* input, size_t input_len,
        const char* filename, int flags, int scope_idx);
    void* user_opaque;
};
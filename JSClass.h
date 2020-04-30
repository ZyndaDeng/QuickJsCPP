#pragma once
#include "cutils.h"
#include "JSValue.h"
#include "JSAtom.h"

class JSRuntime;
class JSContext;
typedef void JS_MarkFunc(JSRuntime* rt, JSValueConst val);
typedef void JSClassFinalizer(JSRuntime* rt, JSValue val);
typedef void JSClassGCMark(JSRuntime* rt, JSValueConst val,
    JS_MarkFunc* mark_func);
typedef JSValue JSClassCall(JSContext* ctx, JSValueConst func_obj,
    JSValueConst this_val, int argc, JSValueConst* argv);

struct JSPropertyDescriptor;
struct JSPropertyEnum;
typedef struct JSClassExoticMethods {
    /* Return -1 if exception (can only happen in case of Proxy object),
       FALSE if the property does not exists, TRUE if it exists. If 1 is
       returned, the property descriptor 'desc' is filled if != NULL. */
    int (*get_own_property)(JSContext* ctx, JSPropertyDescriptor* desc,
        JSValueConst obj, JSAtom prop);
    /* '*ptab' should hold the '*plen' property keys. Return 0 if OK,
       -1 if exception. The 'is_enumerable' field is ignored.
    */
    int (*get_own_property_names)(JSContext* ctx, JSPropertyEnum** ptab,
        uint32_t* plen,
        JSValueConst obj);
    /* return < 0 if exception, or TRUE/FALSE */
    int (*delete_property)(JSContext* ctx, JSValueConst obj, JSAtom prop);
    /* return < 0 if exception or TRUE/FALSE */
    int (*define_own_property)(JSContext* ctx, JSValueConst this_obj,
        JSAtom prop, JSValueConst val,
        JSValueConst getter, JSValueConst setter,
        int flags);
    /* The following methods can be emulated with the previous ones,
       so they are usually not needed */
       /* return < 0 if exception or TRUE/FALSE */
    int (*has_property)(JSContext* ctx, JSValueConst obj, JSAtom atom);
    JSValue(*get_property)(JSContext* ctx, JSValueConst obj, JSAtom atom,
        JSValueConst receiver);
    /* return < 0 if exception or TRUE/FALSE */
    int (*set_property)(JSContext* ctx, JSValueConst obj, JSAtom atom,
        JSValueConst value, JSValueConst receiver, int flags);
} JSClassExoticMethods;

struct JSClass {
    uint32_t class_id; /* 0 means free entry */
    JSAtom class_name;
    JSClassFinalizer* finalizer;
    JSClassGCMark* gc_mark;
    JSClassCall* call;
    /* pointers for exotic behavior, can be NULL if none are present */
    const JSClassExoticMethods* exotic;
};
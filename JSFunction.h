#pragma once
#include "cutils.h"
#include "JSValue.h"

class JSString;
typedef struct JSRegExp {
    JSString* pattern;
    JSString* bytecode; /* also contains the flags */
} JSRegExp;

/* C function definition */
typedef enum JSCFunctionEnum {  /* XXX: should rename for namespace isolation */
    JS_CFUNC_generic,
    JS_CFUNC_generic_magic,
    JS_CFUNC_constructor,
    JS_CFUNC_constructor_magic,
    JS_CFUNC_constructor_or_func,
    JS_CFUNC_constructor_or_func_magic,
    JS_CFUNC_f_f,
    JS_CFUNC_f_f_f,
    JS_CFUNC_getter,
    JS_CFUNC_setter,
    JS_CFUNC_getter_magic,
    JS_CFUNC_setter_magic,
    JS_CFUNC_iterator_next,
} JSCFunctionEnum;

class JSContext;

typedef JSValue JSCFunction(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
typedef JSValue JSCFunctionMagic(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic);
typedef JSValue JSCFunctionData(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic, JSValue* func_data);

typedef union JSCFunctionType {
    JSCFunction* generic;
    JSValue(*generic_magic)(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic);
    JSCFunction* constructor;
    JSValue(*constructor_magic)(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv, int magic);
    JSCFunction* constructor_or_func;
    double (*f_f)(double);
    double (*f_f_f)(double, double);
    JSValue(*getter)(JSContext* ctx, JSValueConst this_val);
    JSValue(*setter)(JSContext* ctx, JSValueConst this_val, JSValueConst val);
    JSValue(*getter_magic)(JSContext* ctx, JSValueConst this_val, int magic);
    JSValue(*setter_magic)(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic);
    JSValue(*iterator_next)(JSContext* ctx, JSValueConst this_val,
        int argc, JSValueConst* argv, int* pdone, int magic);
} JSCFunctionType;
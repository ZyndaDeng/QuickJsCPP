#pragma once
#include "TypeIds.h"
#include <stdint.h>

class JSString;
class JSObject;
//struct JSValue {
//	TypeIds tag;
//	union 
//	{
//		JSString*str;
//		JSObject* obj;
//		void* ptr;
//		int i;
//		bool b;
//	} u;
//
//	JSValue() :
//		tag(JS_TAG_UNDEFINED)
//	{
//		
//	}
//
//	JSValue(int value):
//		tag(JS_TAG_INT)
//	{
//		u.i = value;
//	}
//
//	JSValue(bool value) :
//		tag(JS_TAG_BOOL)
//	{
//		u.b = value;
//	}
//
//	JSValue(JSString* value) :
//		tag(JS_TAG_STRING)
//	{
//		u.str= value;
//	}
//
//	JSValue(JSObject* value) :
//		tag(JS_TAG_OBJECT)
//	{
//		u.obj = value;
//	}
//
//	inline JSString* getStr() {
//		return u.str;
//	}
//
//	inline JSObject* getObj() {
//		return u.obj;
//	}
//
//
//	inline int getInt() {
//		return u.i;
//	}
//
//	inline int getBool() {
//		return u.i;
//	}
//
//	const static JSValue Null;
//	const static JSValue Undefined;
//	const static JSValue Exception;
//};
typedef uint64_t JSValue;

#define JSValueConst JSValue

#define JS_FLOAT64_NAN NAN
#define JS_VALUE_IS_BOTH_INT(v1, v2) ((JS_VALUE_GET_TAG(v1) | JS_VALUE_GET_TAG(v2)) == 0)
#define JS_VALUE_IS_BOTH_FLOAT(v1, v2) (JS_TAG_IS_FLOAT64(JS_VALUE_GET_TAG(v1)) && JS_TAG_IS_FLOAT64(JS_VALUE_GET_TAG(v2)))

//#define JS_VALUE_GET_TAG(v) v.tag
//#define JS_VALUE_GET_INT(v) v.u.i
//#define JS_VALUE_GET_BOOL(v) v.u.b
//#define JS_VALUE_GET_VPTR(v) v.u.ptr
//#define JS_VALUE_GET_STRING(v) v.u.str
//#define JS_VALUE_GET_OBJ(v) v.u.obj
#define JS_VALUE_GET_TAG(v) (int)((v) >> 32)
#define JS_VALUE_GET_INT(v) (int)(v)
#define JS_VALUE_GET_BOOL(v) (int)(v)
#define JS_VALUE_GET_PTR(v) (void *)(intptr_t)(v)
#define JS_VALUE_HAS_REF_COUNT(v) ((unsigned)JS_VALUE_GET_TAG(v) >= (unsigned)JS_TAG_FIRST)
#define JS_VALUE_GET_GC(v)(RecyclableObject*)JS_VALUE_GET_PTR(v)
#define JS_VALUE_GET_OBJ(v) ((JSObject *)JS_VALUE_GET_PTR(v))
#define JS_VALUE_GET_STRING(v) ((JSString *)JS_VALUE_GET_PTR(v))

#define JS_TAG_IS_FLOAT64(tag) ((unsigned)((tag) - JS_TAG_FIRST) >= (JS_TAG_FLOAT64 - JS_TAG_FIRST))
#define JS_MKVAL(tag, val) (((uint64_t)(tag) << 32) | (uint32_t)(val))
#define JS_MKPTR(tag, ptr) (((uint64_t)(tag) << 32) | (uintptr_t)(ptr))

#define JS_FLOAT64_TAG_ADDEND (0x7ff80000 - JS_TAG_FIRST + 1) /* quiet NaN encoding */

/* special values */
#define JS_NULL      JS_MKVAL(JS_TAG_NULL, 0)
#define JS_UNDEFINED JS_MKVAL(JS_TAG_UNDEFINED, 0)
#define JS_FALSE     JS_MKVAL(JS_TAG_BOOL, 0)
#define JS_TRUE      JS_MKVAL(JS_TAG_BOOL, 1)
#define JS_EXCEPTION JS_MKVAL(JS_TAG_EXCEPTION, 0)
#define JS_UNINITIALIZED JS_MKVAL(JS_TAG_UNINITIALIZED, 0)

static inline double JS_VALUE_GET_FLOAT64(JSValue v)
{
	union {
		JSValue v;
		double d;
	} u;
	u.v = v;
	u.v += JS_FLOAT64_TAG_ADDEND ;
	return u.d;
}

/* same as JS_VALUE_GET_TAG, but return JS_TAG_FLOAT64 with NaN boxing */
static inline int JS_VALUE_GET_NORM_TAG(JSValue v)
{
	uint32_t tag;
	tag = JS_VALUE_GET_TAG(v);
	if (JS_TAG_IS_FLOAT64(tag))
		return JS_TAG_FLOAT64;
	else
		return tag;
}

JS_BOOL JS_IsNumber(JSValueConst v);

#define JS_NAN (0x7ff8000000000000 - ((uint64_t)JS_FLOAT64_TAG_ADDEND << 32))

static inline JS_BOOL JS_IsInteger(JSValueConst v)
{
	int tag = JS_VALUE_GET_TAG(v);
	return tag == JS_TAG_INT || tag == JS_TAG_BIG_INT;
}

static inline JS_BOOL JS_IsBigFloat(JSValueConst v)
{
	int tag = JS_VALUE_GET_TAG(v);
	return tag == JS_TAG_BIG_FLOAT;
}

static inline JS_BOOL JS_IsBool(JSValueConst v)
{
	return JS_VALUE_GET_TAG(v) == JS_TAG_BOOL;
}

static inline JS_BOOL JS_IsNull(JSValueConst v)
{
	return JS_VALUE_GET_TAG(v) == JS_TAG_NULL;
}

static inline JS_BOOL JS_IsUndefined(JSValueConst v)
{
	return JS_VALUE_GET_TAG(v) == JS_TAG_UNDEFINED;
}

static inline JS_BOOL JS_IsException(JSValueConst v)
{
	return js_unlikely(JS_VALUE_GET_TAG(v) == JS_TAG_EXCEPTION);
}

static inline JS_BOOL JS_IsUninitialized(JSValueConst v)
{
	return js_unlikely(JS_VALUE_GET_TAG(v) == JS_TAG_UNINITIALIZED);
}

static inline JS_BOOL JS_IsString(JSValueConst v)
{
	return JS_VALUE_GET_TAG(v) == JS_TAG_STRING;
}

static inline JS_BOOL JS_IsSymbol(JSValueConst v)
{
	return JS_VALUE_GET_TAG(v) == JS_TAG_SYMBOL;
}

static inline JS_BOOL JS_IsObject(JSValueConst v)
{
	return JS_VALUE_GET_TAG(v) == JS_TAG_OBJECT;
}

static JSObject* get_proto_obj(JSValueConst proto_val)
{
	if (JS_VALUE_GET_TAG(proto_val) != JS_TAG_OBJECT)
		return NULL;
	else
		return JS_VALUE_GET_OBJ(proto_val);
}
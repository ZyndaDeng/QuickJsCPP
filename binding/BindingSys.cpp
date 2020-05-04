#include "BindingSys.h"

JSClassID jsb_Class(JSContext* ctx, const char* className, const char* extend, JSClassCall constructor, const JSCFunctionListEntry* list, int listCount, JSClassFinalizer* finalizer)
{
	JSClassID id = -1;
    JSClassDef def{
					   className,
					  finalizer
    };
	JS_NewClassID(&id);
	jsb_registerClassID(className, id);
	int e = JS_NewClass(JS_GetRuntime(ctx), id, &def);
	if (e < 0)
	{
		JS_ThrowInternalError(ctx, "Cant register class %s", name);
		return id;//throw exception{};
	}
	JSValue proto = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, proto, list,
		listCount);
	if (extend) {
			JSValue ext = JS_GetClassProto(ctx, jsb_getClassID(extend));

	}
	JS_SetClassProto(ctx, id, proto);
	return id;
}

void jsb_func(JSContext* ctx, const char* funcName, JSCFunction func, int argCount)
{
}

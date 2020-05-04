#pragma once

#include "../quickjs/quickjs.h"
//using namespace Urho3D;

JSClassID jsb_getClassID(const char* className);
void  jsb_registerClassID(const char* className, JSClassID id);

//绑定类
JSClassID jsb_Class(JSContext *ctx, const char* className, const char* extend, JSClassCall constructor, const JSCFunctionListEntry* list,int listCount, JSClassFinalizer* finalizer);
//绑定函数
void jsb_func(JSContext* ctx, const char* funcName, JSCFunction func, int argCount);
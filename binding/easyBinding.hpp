#pragma once

extern "C"
{
#include "../quickjs/quickjs.h"
#include "../quickjs/quickjs-libc.h"
}

#include <vector>
#include <string_view>
#include <string>
#include <cassert>
#include <memory>
#include <cstddef>
#include <algorithm>
#include <tuple>
#include <functional>
#include <stdexcept>

#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define JSB_CFUNC_DEF(name, length, func1) JSCFunctionListEntry( name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, .u.func = { length, JS_CFUNC_generic, { .generic = func1 } } )

namespace jsb
{

    class Value
    {
    public:
        JSValue v;
        JSContext* ctx = nullptr;

    public:
        /** Use context.newValue(val) instead */
        //template <typename T>
        //Value(JSContext* ctx, T&& val) : ctx(ctx)
        //{
        //    v = js_traits<std::decay_t<T>>::wrap(ctx, std::forward<T>(val));
        //   /* if (JS_IsException(v))
        //        throw exception{};*/
        //}

        Value(JSContext* ctx, JSValue v) :
            ctx(ctx)
            , v(v) {

        }

        Value(const Value& rhs)
        {
            ctx = rhs.ctx;
            v = JS_DupValue(ctx, rhs.v);
        }

        Value(Value&& rhs)
        {
            std::swap(ctx, rhs.ctx);
            v = rhs.v;
        }

        Value& operator=(Value rhs)
        {
            std::swap(ctx, rhs.ctx);
            std::swap(v, rhs.v);
            return *this;
        }

        bool operator==(JSValueConst other) const
        {
            return JS_VALUE_GET_TAG(v) == JS_VALUE_GET_TAG(other) && JS_VALUE_GET_PTR(v) == JS_VALUE_GET_PTR(other);
        }

        bool operator!=(JSValueConst other) const { return !((*this) == other); }


        /** Returns true if 2 values are the same (equality for arithmetic types or point to the same object) */
        bool operator==(const Value& rhs) const
        {
            return ctx == rhs.ctx && (*this == rhs.v);
        }

        bool operator!=(const Value& rhs) const { return !((*this) == rhs); }


        ~Value()
        {
            if (ctx) JS_FreeValue(ctx, v);
        }

        bool isError() const { return JS_IsError(ctx, v); }

        Value getProperty(const char* k) {
            JSValue c = JS_GetPropertyStr(ctx, v, k);
            return Value(ctx, c);
        }

        void setProperty(const char* k, const Value& pv) {
            JS_SetPropertyStr(ctx, v, k,JS_DupValue(ctx,  pv.v));
        }

        void setProperty(const char* k, int iv) {
            JS_SetPropertyStr(ctx, v, k, JS_MKVAL(JS_TAG_INT, iv));
        }

      

        JSValue release() // dont call freevalue
        {
            ctx = nullptr;
            return v;
        }

        /** Implicit conversion to JSValue (rvalue only). Example: JSValue v = std::move(value); */
        operator JSValue()&& { return release(); }

        void addProperty(const JSCFunctionListEntry* list, int listCount) {
            JS_SetPropertyFunctionList(ctx, v, list,
                listCount);
        }

        void setPrototype(const Value& proto) {
            JS_SetPrototype(ctx, v, proto.v);
        }
       

    };

    /** A custom allocator that uses js_malloc_rt and js_free_rt
 */
        template <typename T>
    struct allocator
    {
        JSRuntime* rt;
        using value_type = T;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_copy_assignment = std::true_type;

        constexpr allocator(JSRuntime* rt) noexcept : rt{ rt }
        {}

        allocator(JSContext* ctx) noexcept : rt{ JS_GetRuntime(ctx) }
        {}

        template <class U>
        constexpr allocator(const allocator<U>& other) noexcept : rt{ other.rt }
        {}

        [[nodiscard]] T* allocate(std::size_t n)
        {
            if (auto p = static_cast<T*>(js_malloc_rt(rt, n * sizeof(T)))) return p;
            throw std::bad_alloc();
        }

        void deallocate(T* p, std::size_t) noexcept
        {
            js_free_rt(rt, p);
        }

        template <class U>
        bool operator ==(const allocator<U>& other) const
        {
            return rt == other.rt;
        }

        template <class U>
        bool operator !=(const allocator<U>& other) const
        {
            return rt != other.rt;
        }
    };

class Runtime
{
public:
    JSRuntime *rt;

    Runtime()
    {
        rt = JS_NewRuntime();
        if (!rt)
            throw std::runtime_error{"jsb: Cannot create runtime"};
    }

    // noncopyable
    Runtime(const Runtime &) = delete;

    ~Runtime()
    {
        JS_FreeRuntime(rt);
    }
};



class Context
{
public:
    JSContext* ctx;


    Context(Runtime& rt) : Context(rt.rt)
    {}
    Context(JSRuntime* rt)
    {
        ctx = JS_NewContext(rt);
        if (!ctx)
            throw std::runtime_error{ "jsb: Cannot create context" };
        //init();
    }

    ~Context() {
       // JS_FreeContext(ctx);
    }

    /** returns globalThis */
    Value global() const { return Value{ ctx, JS_GetGlobalObject(ctx) }; }

    /** returns new Object() */
    Value newObject() const { return Value{ ctx, JS_NewObject(ctx) }; }

protected:
    static Context* instance;
};

class JSBClass {
public:
    const char* name;
    Value prototype;
    //Module* module;
    Value ctor;
   const Context& context;
    //JSClassID id;
public:
    explicit JSBClass(const Context& context,JSClassID& id,  const char* name, JSCFunction* ctorFunc, JSClassFinalizer* finalizer, JSBClass* extend = nullptr) :
        name(name)
        , ctor(context.ctx, JS_NewCFunction(context.ctx, ctorFunc, name, 0))
        , prototype(context.newObject())
        , context(context)
    {
        JSClassDef def{
                           name,
                          finalizer
        };
        JS_NewClassID(&id);
        //jsb_registerClassID(name, id);
        int e = JS_NewClass(JS_GetRuntime(context.ctx), id, &def);
        if (extend) {
            prototype.setPrototype(extend->ctor);
        }
        JS_SetConstructor(context.ctx, ctor.v, prototype.v);
        JS_SetClassProto(context.ctx, id,JS_DupValue(context.ctx, ctor.v));
    }

    JSBClass(const JSBClass&) = delete;

    void setMembers(const JSCFunctionListEntry* list, int listCount) {
        prototype.addProperty(list, listCount);
    }

    void setStaticMembers(const JSCFunctionListEntry* list, int listCount) {
        ctor.addProperty(list, listCount);
    }
};

} // namespace jsb

#include "../easyBinding.hpp"

using namespace jsb;

class Point {
public:
	int x;
	int y;
	
public:
	Point():
		x(0)
		,y(0)
	{}

	Point(int x,int y) :
		x(x)
		, y(y)
	{}

	const char* getName() {
		return "Point";
	}
};

class Rect :public Point {
public:
	int w;
	int h;
public:
	Rect() :
		w(0)
		, h(0)
	{}

	int getRight() {
		return x + w;
	}
};

JSClassID js_point_id = 0;

void js_point_finalizer(JSRuntime* rt, JSValue val) {
	Point* p = (Point*)JS_GetOpaque(val, js_point_id);
	delete p;
}

JSValue js_point_ctor(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	if (argc == 2) {
		int x = JS_VALUE_GET_INT(argv[0]);
		int y = JS_VALUE_GET_INT(argv[1]);
		Point* p = new Point(x, y);
		JS_SetOpaque(this_val, p);
	}
	else {
		Point* p = new Point();
		JS_SetOpaque(this_val, p);
	}
	return this_val;
}
static JSValue js_point_getName(JSContext* ctx, JSValueConst this_val,
	int argc, JSValueConst* argv)
{
	Point* p = (Point*)JS_GetOpaque(this_val, js_point_id);
	if (p) {
		const char* name = p->getName();
		return JS_NewString(ctx, name);
	}
	else {
		return JS_EXCEPTION;
	}
	
}



#define JSB_CFUNC_DEF(name, length, func1) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0,  { length, JS_CFUNC_generic, {  func1 } } }

	void js_manual_package_api(const Context& ctx) {
		JSBClass c(ctx, js_point_id, "Point", js_point_ctor, js_point_finalizer);

	
		const JSCFunctionListEntry list[] = {
			 JSB_CFUNC_DEF("getName", 0, js_point_getName),
		};

		c.setMembers(list, countof(list));
		ctx.global().setProperty("Point", c.ctor);

		ctx.global().setProperty("vv", 34);
		ctx.global().setProperty("v2", ctx.newObject());
	}

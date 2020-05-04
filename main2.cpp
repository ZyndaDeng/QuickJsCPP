#pragma once

extern "C" {
#include "quickjs/quickjs.h"
#include "quickjs/quickjs-libc.h"
}

#include <algorithm>
#include <tuple>
#include <functional>
#include <unordered_map>
#include <cassert>
#include <memory>
#include <stdexcept>

#include "binding/easyBinding.hpp"
#include "binding/ApiEnter.h"
using namespace jsb;
int main(int argc, char** argv)
{
	Runtime rt;
	Context ctx(rt);

	/* loader for ES6 modules */
	JS_SetModuleLoaderFunc(rt.rt, NULL, js_module_loader, NULL);
	js_std_add_helpers(ctx.ctx, argc, argv);

	/* system modules */
	js_init_module_std(ctx.ctx, "std");
	js_init_module_os(ctx.ctx, "os");

	 jsapi_init_all( ctx);

	const char* filename = argv[1];
	if (!filename) {
		filename = "D:/main.js";
	}
	size_t buf_len;
	auto deleter = [ctx](void* p) { js_free(ctx.ctx, p); };
	auto buf = std::unique_ptr<uint8_t, decltype(deleter)>{ js_load_file(ctx.ctx, &buf_len, filename), deleter };
	if (!buf)
		throw std::runtime_error{ std::string{"evalFile: can't read file: "} +filename };
	
	JSValue v = JS_Eval(ctx.ctx, reinterpret_cast<char*>(buf.get()), buf_len, filename, JS_EVAL_TYPE_GLOBAL);

	if (v == JS_EXCEPTION) {
		JS_ThrowSyntaxError(ctx.ctx, "can not eval file");
	}
	//js_std_loop(ctx.ctx);

	//js_std_free_handlers(rt.rt);

	return 0;
}
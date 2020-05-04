#include "ApiEnter.h"
#include "autoApi.h"

void jsapi_init_all(const Context& ctx)
{
	js_manual_package_api(ctx);
}

#pragma once
#include "cutils.h"
#include <string>

typedef uint32_t JSAtom;

enum {
    JS_ATOM_NULL,
#define DEF(name, str) JS_ATOM_ ## name,
#include "quickjs-atom.h"

#undef DEF
    JS_ATOM_END,
};

static const std::string js_atom_init[] = {
#define DEF(name, str) #str ,
#include "quickjs-atom.h"
#undef DEF
};
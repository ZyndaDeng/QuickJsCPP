#pragma once
#include "cutils.h"
#include "list.h"
#include "RecyclableObject.h"
#include "JSAtom.h"
#include "JSValue.h"
struct JSModuleDef;
struct JSVarRef;
class JSContext;
typedef struct JSReqModuleEntry {
    JSAtom module_name;
    JSModuleDef* module; /* used using resolution */
} JSReqModuleEntry;

typedef enum JSExportTypeEnum {
    JS_EXPORT_TYPE_LOCAL,
    JS_EXPORT_TYPE_INDIRECT,
} JSExportTypeEnum;

typedef struct JSExportEntry {
    union {
        struct {
            int var_idx; /* closure variable index */
            JSVarRef* var_ref; /* if != NULL, reference to the variable */
        } local; /* for local export */
        int req_module_idx; /* module for indirect export */
    } u;
    JSExportTypeEnum export_type;
    JSAtom local_name; /* not used for local export after compilation */
    JSAtom export_name; /* exported variable name */
} JSExportEntry;

typedef struct JSStarExportEntry {
    int req_module_idx; /* in req_module_entries */
} JSStarExportEntry;

typedef struct JSImportEntry {
    int var_idx; /* closure variable index */
    JSAtom import_name;
    int req_module_idx; /* in req_module_entries */
} JSImportEntry;

typedef int JSModuleInitFunc(JSContext* ctx, JSModuleDef* m);
typedef char* JSModuleNormalizeFunc(JSContext* ctx,
    const char* module_base_name,
    const char* module_name, void* opaque);
typedef JSModuleDef* JSModuleLoaderFunc(JSContext* ctx,
    const char* module_name, void* opaque);

struct JSModuleDef {
    JSRefCountHeader header; /* must come first, 32-bit */
    JSAtom module_name;
    struct list_head link;

    JSReqModuleEntry* req_module_entries;
    int req_module_entries_count;
    int req_module_entries_size;

    JSExportEntry* export_entries;
    int export_entries_count;
    int export_entries_size;

    JSStarExportEntry* star_export_entries;
    int star_export_entries_count;
    int star_export_entries_size;

    JSImportEntry* import_entries;
    int import_entries_count;
    int import_entries_size;

    JSValue module_ns;
    JSValue func_obj; /* only used for JS modules */
    JSModuleInitFunc* init_func; /* only used for C modules */
    BOOL resolved;
    BOOL instantiated;
    BOOL evaluated;
};


#pragma once
#include "cutils.h"

typedef struct JSRefCountHeader {
    int ref_count;
} JSRefCountHeader;

typedef struct JSGCHeader {
    uint8_t mark;
} JSGCHeader;

class RecyclableObject {
    friend class JSContext;
protected:
    JSRefCountHeader header; /* must come first, 32-bit */
    //JSGCHeader gc_header; /* must come after JSRefCountHeader, 8-bit */
};
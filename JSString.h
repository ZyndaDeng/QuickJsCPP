#pragma once
#include "RecyclableObject.h"
#include "cutils.h"
#include "libregexp.h"
class JSString :public RecyclableObject{
    friend class JSRuntime;
    friend class JSContext;
protected:
    uint32_t len : 31;
    uint8_t is_wide_char : 1; /* 0 = 8 bits, 1 = 16 bits characters */
    uint32_t hash : 30;
    uint8_t atom_type : 2; /* != 0 if atom, JS_ATOM_TYPE_x */
    uint32_t hash_next; /* atom_index for JS_ATOM_TYPE_SYMBOL */
#ifdef DUMP_LEAKS
    struct list_head link; /* string list */
#endif
    union {
        uint8_t str8[0]; /* 8 bit strings will get an extra null terminator */
        uint16_t str16[0];
    } u;
};



BOOL lre_is_space(int c)
{
    int i, n, low, high;
    n = (countof(char_range_s) - 1) / 2;
    for (i = 0; i < n; i++) {
        low = char_range_s[2 * i + 1];
        if (c < low)
            return FALSE;
        high = char_range_s[2 * i + 2];
        if (c < high)
            return TRUE;
    }
    return FALSE;
}

static int skip_spaces(const char* pc)
{
    const uint8_t* p, * p_next, * p_start;
    uint32_t c;

    p = p_start = (const uint8_t*)pc;
    for (;;) {
        c = *p;
        if (c < 128) {
            if (!((c >= 0x09 && c <= 0x0d) || (c == 0x20)))
                break;
            p++;
        }
        else {
            c = unicode_from_utf8(p, UTF8_CHAR_LEN_MAX, &p_next);
            if (!lre_is_space(c))
                break;
            p = p_next;
        }
    }
    return p - p_start;
}
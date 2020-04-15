#include "bitmap.h"

int bitmap_get(void* bm, int ii)
{
    char* base = (char*)bm;
    return base[ii];
}

void bitmap_put(void* bm, int ii, int vv)
{
    char* base = (char*)bm;
    base[ii] = (char)vv;
}

void bitmap_print(void* bm, int size);
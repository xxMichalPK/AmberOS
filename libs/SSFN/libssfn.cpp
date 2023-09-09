#define SSFN_IMPLEMENTATION
#define SSFN_MAXLINES 0x5000
#include "libssfn.h"

namespace SSFN {

    Font::Font() {
        SSFN_memset(&this->ctx, 0, sizeof(ssfn_t));
    }

    Font::~Font() {
        ssfn_free(&this->ctx);
    }

    int Font::Load(const void *data) {
        return ssfn_load(&this->ctx, data);
    }

    int Font::Select(int family, char *name, int style, int size) {
        return ssfn_select(&this->ctx,family,name,style,size);
    }

    int Font::Render(ssfn_buf_t *dst, const char *str) {
        return ssfn_render(&this->ctx, dst, str);
    }

    int Font::BBox(const char *str, int *w, int *h, int *left, int *top) {
        return ssfn_bbox(&this->ctx,str,w,h,left,top);
    }

    ssfn_buf_t *Font::Text(const char *str, unsigned int fg) {
        return ssfn_text(&this->ctx, str, fg);
    }

    int Font::LineHeight() {
        return this->ctx.line ? this->ctx.line : this->ctx.size;
    }

    int Font::Mem() {
        return ssfn_mem(&this->ctx);
    }
    
}
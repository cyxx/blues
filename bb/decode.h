
#ifndef DECODE_H__
#define DECODE_H__

#include "intern.h"

extern void	decode_spr(const uint8_t *src, int src_pitch, int w, int h, int depth, uint8_t *dst, int dst_pitch, int dst_x, int dst_y, bool reverse_bitplanes);
extern void	decode_amiga_blk(const uint8_t *src, uint8_t *dst, int dst_pitch);
extern void	decode_amiga_gfx(uint8_t *dst, int dst_pitch, int w, int h, int depth, const uint8_t *src, const int src_pitch, uint8_t palette_mask, uint16_t gfx_mask);

#endif /* DECODE_H__ */


#ifndef DECODE_H__
#define DECODE_H__

#include "intern.h"

extern void	decode_ega_spr(const uint8_t *src, int src_pitch, int w, int h, uint8_t *dst, int dst_pitch, int dst_x, int dst_y);

#endif /* DECODE_H__ */

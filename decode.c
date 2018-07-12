
#include "decode.h"

void decode_ega_spr(const uint8_t *src, int src_pitch, int w, int h, uint8_t *dst, int dst_pitch, int dst_x, int dst_y) {
	assert((src_pitch & 7) == 0);
	src_pitch /= 8;
	assert((w & 7) == 0);
	w /= 8;
	const int bitplane_size = w * h;
	for (int y = 0; y < h; ++y) {
		const int y_offset = dst_y + y;
		for (int x = 0; x < w; ++x) {
			for (int i = 0; i < 8; ++i) {
				int color = 0;
				const int mask = 1 << (7 - i);
				for (int bit = 0; bit < 4; ++bit) {
					if (src[bit * bitplane_size] & mask) {
						color |= 1 << (3 - bit);
					}
				}
				if (color != 0) {
					const int x_offset = dst_x + (x * 8 + i);
					dst[y_offset * dst_pitch + x_offset] = color;
				}
			}
			++src;
		}
		src += src_pitch - w;
	}
}

void decode_amiga_planar8(const uint8_t *src, int w, int h, int depth, uint8_t *dst, int dst_pitch, int dst_x, int dst_y) {
	dst += dst_y * dst_pitch + dst_x;
	const int plane_size = w * h;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			for (int i = 0; i < 8; ++i) {
				int color = 0;
				const int mask = 1 << (7 - i);
				for (int bit = 0; bit < depth; ++bit) {
					if (src[bit * plane_size] & mask) {
						color |= 1 << bit;
					}
				}
				dst[x * 8 + i] = color;
			}
			++src;
		}
		dst += dst_pitch;
	}
}

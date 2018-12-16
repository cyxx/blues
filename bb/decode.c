
#include "decode.h"

void decode_spr(const uint8_t *src, int src_pitch, int w, int h, int depth, uint8_t *dst, int dst_pitch, int dst_x, int dst_y, bool reverse_bitplanes) {
	assert((src_pitch & 7) == 0);
	src_pitch /= 8;
	assert((w & 7) == 0);
	w /= 8;
	dst += dst_y * dst_pitch + dst_x;
	const int bitplane_size = src_pitch * h;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			for (int i = 0; i < 8; ++i) {
				int color = 0;
				const int mask = 1 << (7 - i);
				for (int bit = 0; bit < depth; ++bit) {
					if (src[bit * bitplane_size] & mask) {
						color |= 1 << (reverse_bitplanes ? bit : (3 - bit));
					}
				}
				dst[x * 8 + i] = color;
			}
			++src;
		}
		src += src_pitch - w;
		dst += dst_pitch;
	}
}

void decode_amiga_blk(const uint8_t *src, uint8_t *dst, int dst_pitch) {
	uint16_t data[4];
	for (int y = 0; y < 16; ++y) {
		for (int p = 0; p < 4; ++p) {
			data[p] = READ_BE_UINT16(src); src += 2;
		}
		for (int b = 0; b < 16; ++b) {
			const int mask = 1 << (15 - b);
			uint8_t color = 0;
			for (int p = 0; p < 4; ++p) {
				if (data[p] & mask) {
					color |= 1 << p;
				}
			}
			dst[b] = color;
		}
		dst += dst_pitch;
	}
}

void decode_amiga_gfx(uint8_t *dst, int dst_pitch, int w, int h, int depth, const uint8_t *src, int src_pitch, uint8_t palette_mask, uint16_t gfx_mask) {
	assert((src_pitch & 15) == 0);
	src_pitch /= 16;
	assert((w & 15) == 0);
	w /= 16;
	const int plane_size = src_pitch * 2 * h;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			for (int i = 0; i < 16; ++i) {
				const int mask = 1 << (15 - i);
				if (gfx_mask & mask) {
					int color = 0;
					for (int bit = 0; bit < depth; ++bit) {
						if (READ_BE_UINT16(src + bit * plane_size) & mask) {
							color |= 1 << bit;
						}
					}
					dst[x * 16 + i] = palette_mask | color;
				}
			}
			src += 2;
		}
		src += src_pitch - w;
		dst += dst_pitch;
	}
}

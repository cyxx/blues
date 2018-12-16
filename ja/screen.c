
/* screen drawing */

#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

#define MAX_SPRITES 288
#define MAX_SPRITESHEET_W 1024
#define MAX_SPRITESHEET_H 512

void video_draw_dot_pattern(int offset) {
	static const int W = 144;
	uint8_t *dst = g_res.vga + (GAME_SCREEN_H - PANEL_H) * GAME_SCREEN_W + offset;
	for (int y = 0; y < PANEL_H; ++y) {
		for (int x = 0; x < W; x += 2) {
			dst[x + (y & 1)] = 0;
		}
		dst += GAME_SCREEN_W;
	}
}

void video_draw_sprite(int num, int x, int y, int flag) {
	g_sys.render_add_sprite(RENDER_SPR_GAME, num, x, y, flag != 0);
}

void video_draw_string(const char *s, int offset, int hspace) {
	while (*s) {
		uint8_t code = *s++;
		if (code != 0x20) {
			if (code >= 0x41) {
				code -= 0x41;
			} else {
				code -= 0x16;
			}
			ja_decode_chr(g_res.font + code * 200, 200, g_res.tmp + 768 + offset, 320);
		}
		offset += hspace;
	}
}

void video_copy_vga(int size) {
	if (size == 0xB500) {
		memcpy(g_res.background, g_res.tmp + 768, 64000);
	} else {
		assert(size == 0x7D00);
		g_sys.set_screen_palette(g_res.tmp, 0, 256, 6);
		const uint8_t *src = g_res.tmp + 768;
		if (GAME_SCREEN_W * GAME_SCREEN_H == 64000) {
			memcpy(g_res.vga, src, 64000);
		} else {
			memset(g_res.vga, 0, GAME_SCREEN_W * GAME_SCREEN_H);
			for (int y = 0; y < MIN(200, GAME_SCREEN_H); ++y) {
				memcpy(g_res.vga + y * GAME_SCREEN_W, src, MIN(320, GAME_SCREEN_W));
				src += 320;
			}
		}
		g_sys.update_screen(g_res.vga, 0);
	}
}

void video_copy_backbuffer(int h) {
	for (int y = 0; y < MIN(200, GAME_SCREEN_H) - h; ++y) {
		for (int x = 0; x < GAME_SCREEN_W; x += 320) {
			memcpy(g_res.vga + y * GAME_SCREEN_W + x, g_res.background + y * 320, MIN(320, GAME_SCREEN_W - x));
		}
	}
}

void fade_in_palette() {
	if (!g_sys.input.quit) {
		g_sys.fade_in_palette();
	}
}

void fade_out_palette() {
	if (!g_sys.input.quit) {
		g_sys.fade_out_palette();
	}
}

void ja_decode_spr(const uint8_t *src, int w, int h, uint8_t *dst, int dst_pitch, uint8_t pal_mask) {
	const int bitplane_size = w * h;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			for (int i = 0; i < 8; ++i) {
				int color = 0;
				const int mask = 1 << (7 - i);
				for (int bit = 0; bit < 4; ++bit) {
					if (src[bit * bitplane_size] & mask) {
						color |= 1 << bit;
					}
				}
				if (color != 0) {
					dst[x * 8 + i] = pal_mask | color;
				}
			}
			++src;
		}
		dst += dst_pitch;
	}
}

void ja_decode_chr(const uint8_t *buffer, const int size, uint8_t *dst, int dst_pitch) {
	for (int y = 0; y < 25; ++y) {
		const int offset = y * 2;
		const uint16_t p[] = {
			READ_BE_UINT16(buffer + offset),
			READ_BE_UINT16(buffer + offset +  50),
			READ_BE_UINT16(buffer + offset + 100),
			READ_BE_UINT16(buffer + offset + 150)
		};
		for (int x = 0; x < 16; ++x) {
			const uint16_t mask = 1 << (15 - x);
			uint8_t color = 0;
			for (int b = 0; b < 4; ++b) {
				if (p[b] & mask) {
					color |= (1 << b);
				}
			}
			if (color != 0) {
				dst[x] = 0xD0 | color;
			}
		}
		dst += dst_pitch;
	}
}

void ja_decode_tile(const uint8_t *buffer, uint8_t pal_mask, uint8_t *dst, int dst_pitch, int x_offset, int y_offset) {
	int tile_w = 16;
	if (x_offset < 0) {
		tile_w += x_offset;
		buffer -= x_offset / 2;
		x_offset = 0;
	}
	if (x_offset + tile_w > TILEMAP_SCREEN_W) {
		tile_w = TILEMAP_SCREEN_W - x_offset;
	}
	if (tile_w <= 0) {
		return;
	}
	int tile_h = 16;
	if (y_offset < 0) {
		tile_h += y_offset;
		buffer -= y_offset * 8;
		y_offset = 0;
	}
	if (y_offset + tile_h > TILEMAP_SCREEN_H) {
		tile_h = TILEMAP_SCREEN_H - y_offset;
	}
	if (tile_h <= 0) {
		return;
	}
	dst += y_offset * dst_pitch + x_offset;
	for (int y = 0; y < tile_h; ++y) {
		uint8_t *p = dst;
		for (int x = 0; x < tile_w / 2; ++x) {
			const uint8_t color1 = buffer[x] >> 4;
			const uint8_t color2 = buffer[x] & 15;
			if (color1 != 0) {
				*p = pal_mask | color1;
			}
			++p;
			if (color2 != 0) {
				*p = pal_mask | color2;
			}
			++p;
		}
		buffer += 8;
		dst += dst_pitch;
	}
}

static void decode_motif_scanline(const uint8_t *src, uint8_t *dst, uint8_t color) {
	for (int x = 0; x < 40; ++x) {
		for (int b = 0; b < 8; ++b) {
			const uint8_t mask = 1 << (7 - b);
			if (src[x] & mask) {
				dst[x * 8 + b] = color;
			}
		}
	}
}

void ja_decode_motif(int num, uint8_t color) {
	const uint8_t *src = g_res.motif + 25 * 320 * num;
	int y = 0;
	for (int j = 0; j < 25; ++j) {
		for (int i = 0; i < 8; ++i) {
			decode_motif_scanline(src + i * 40 + j * 320, g_res.vga + y * GAME_SCREEN_W, color);
			++y;
		}
	}
}

void video_load_sprites() {
	struct sys_rect_t r[MAX_SPRITES];
	uint8_t *data = (uint8_t *)calloc(MAX_SPRITESHEET_W * MAX_SPRITESHEET_H, 1);
	if (data) {
		int current_x = 0;
		int max_w = 0;
		int current_y = 0;
		int max_h = 0;

		int total_size = 0;
		int count = 0;
		uint8_t value;
		for (int i = 0; (value = sprite_offsets[i] & 255) != 0; ++i, ++count) {

			value = (value >> 3) | ((value & 7) << 5);
			if ((value & 0xE0) != 0) {
				value &= ~0xE0;
				++value;
			}
			const int h = sprite_offsets[i] >> 8;
			const int w = value * 8;
			assert((sprite_offsets[i] & 255) == w);
			const int size = (h * value) * 4;

			if (current_x + w > MAX_SPRITESHEET_W) {
				current_y += max_h;
				if (current_x > max_w) {
					max_w = current_x;
				}
				current_x = 0;
				max_h = h;
			} else {
				if (h > max_h) {
					max_h = h;
				}
			}
			ja_decode_spr(g_res.sprites + total_size, w / 8, h, data + current_y * MAX_SPRITESHEET_W + current_x, MAX_SPRITESHEET_W, sprite_palettes[i] * 0x10);
			total_size += size;

			r[i].x = current_x;
			r[i].y = current_y;
			r[i].w = w;
			r[i].h = h;
			current_x += w;
		}
		assert(count <= MAX_SPRITES);
		assert(max_w <= MAX_SPRITESHEET_W);
		assert(current_y + max_h <= MAX_SPRITESHEET_H);
		g_sys.render_unload_sprites(RENDER_SPR_GAME);
		g_sys.render_load_sprites(RENDER_SPR_GAME, count, r, data, MAX_SPRITESHEET_W, current_y + max_h, 0, 0x0);
		free(data);
	}
}


/* screen drawing */

#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

#define MAX_SPRITES 480
#define MAX_SPRITESHEET_W 2048
#define MAX_SPRITESHEET_H 1024
#define MAX_FRONT_TILES 168

static void decode_planar(const uint8_t *src, uint8_t *dst, int dst_pitch, int w, int h, uint8_t transparent_color) {
	const int plane_size = h * w / 8;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w / 8; ++x) {
			for (int i = 0; i < 8; ++i) {
				const uint8_t mask = 1 << (7 - i);
				uint8_t color = 0;
				for (int b = 0; b < 4; ++b) {
					if (src[b * plane_size] & mask) {
						color |= (1 << b);
					}
				}
				if (color != transparent_color) {
					dst[x * 8 + i] = color;
				}
			}
			++src;
		}
		dst += dst_pitch;
	}
}

static void convert_planar_tile_4bpp(const uint8_t *src, uint8_t *dst, int dst_pitch) {
	static const int tile_h = 16;
	static const int tile_w = 16;
	static const int plane_size = 16 * (16 / 8);
	for (int y = 0; y < tile_h; ++y) {
		for (int x = 0; x < tile_w / 8; ++x) {
			for (int i = 0; i < 8; ++i) {
				const uint8_t mask = 1 << (7 - i);
				uint8_t color = 0;
				for (int b = 0; b < 4; ++b) {
					if (src[b * plane_size] & mask) {
						color |= (1 << b);
					}
				}
				if (i & 1) {
					dst[x * 4 + (i >> 1)] |= color;
				} else {
					dst[x * 4 + (i >> 1)] = color << 4;
				}
			}
			++src;
		}
		dst += dst_pitch;
	}
}

void video_draw_string(int offset, int hspace, const char *s) {
	offset += hspace;
	while (*s) {
		uint8_t code = *s++;
		if (code != 0x20) {
			code -= 0x30;
			if (code > 9) {
				code -= 2;
			}
			decode_planar(g_res.allfonts + code * 48, g_res.vga + offset * 8, 320, 8, 12, 0);
                }
		++offset;
        }
}

void video_draw_panel_number(int offset, int num) {
	const uint8_t *fnt = g_res.allfonts + 48 * 41 + 160 * 23;
	const int y = (offset * 8) / 320 + (GAME_SCREEN_H - 200);
	const int x = (offset * 8) % 320 + (GAME_SCREEN_W - 320) / 2;
	decode_planar(fnt + num * 96, g_res.vga + y * GAME_SCREEN_W + x, GAME_SCREEN_W, 16, 12, 0);
}

void video_draw_number(int offset, int num) {
	const uint8_t *fnt = g_res.allfonts + 0x1C70;
	const int y = (offset * 8) / 320;
	const int x = (offset * 8) % 320;
	decode_planar(fnt + num * 88, g_res.vga + y * GAME_SCREEN_W + x, GAME_SCREEN_W, 16, 11, 0);
}

void video_clear() {
	memset(g_res.vga, 0, GAME_SCREEN_W * GAME_SCREEN_H);
}

void video_copy_img(const uint8_t *src) {
	decode_planar(src, g_res.background, 320, 320, 200, 0xFF);
}

void video_draw_panel(const uint8_t *src) {
	const int h = GAME_SCREEN_H - PANEL_H;
	const int x = (GAME_SCREEN_W - 320) / 2;
	decode_planar(src, g_res.vga + h * GAME_SCREEN_W + x, GAME_SCREEN_W, 320, 23, 0xFF);
}

void video_draw_tile(const uint8_t *src, int x_offset, int y_offset) {
	int tile_w = 16;
	if (x_offset < 0) {
		tile_w += x_offset;
		src -= x_offset / 2;
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
		src -= y_offset * 8;
		y_offset = 0;
	}
	if (y_offset + tile_h > TILEMAP_SCREEN_H) {
		tile_h = TILEMAP_SCREEN_H - y_offset;
	}
	if (tile_h <= 0) {
		return;
	}
	uint8_t *dst = g_res.vga + y_offset * TILEMAP_SCREEN_W + x_offset;
	for (int y = 0; y < tile_h; ++y) {
		for (int x = 0; x < tile_w / 2; ++x) {
			const uint8_t color = src[x];
			const uint8_t c1 = color >> 4;
			if (c1 != 0) {
				dst[x * 2] = c1;
			}
			const uint8_t c2 = color & 15;
			if (c2 != 0) {
				dst[x * 2 + 1] = c2;
			}
		}
		src += 8;
		dst += TILEMAP_SCREEN_W;
	}
}

void video_convert_tiles(uint8_t *data, int len) {
	for (int offset = 0; offset < (len & ~127); offset += 128) {
		uint8_t buffer[16 * 8];
		convert_planar_tile_4bpp(data + offset, buffer, 8);
		memcpy(data + offset, buffer, 16 * 8);
	}
}

void video_load_front_tiles() {
	g_sys.render_unload_sprites(RENDER_SPR_FG);
	assert((g_res.frontlen & 127) == 0);
	const int count = g_res.frontlen / (16 * 8);
	assert(count <= MAX_FRONT_TILES);
	struct sys_rect_t r[MAX_FRONT_TILES];
	const int w = 256;
	const int h = 192;
	memset(g_res.vga, 0, w * h);
	int tile = 0;
	for (int y = 0; y < h; y += 16) {
		for (int x = 0; x < w; x += 16) {
			r[tile].x = x;
			r[tile].y = y;
			r[tile].w = 16;
			r[tile].h = 16;
			decode_planar(g_res.frontdat + tile * 16 * 8, g_res.vga + y * w + x, w, 16, 16, 0);
			++tile;
			if (tile == count) {
				g_sys.render_load_sprites(RENDER_SPR_FG, count, r, g_res.vga, w, h, 0, 0x0);
				return;
			}
		}
	}
}

void video_set_palette() {
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

void video_wait_vbl() {
}

void video_transition_close() {
}

void video_transition_open() {
}

void video_load_sprites() {
	const uint16_t *sprite_offsets = (const uint16_t *)spr_size_tbl;
	struct sys_rect_t r[MAX_SPRITES];
	uint8_t *data = (uint8_t *)calloc(MAX_SPRITESHEET_W * MAX_SPRITESHEET_H, 1);
	if (data) {
		int current_x = 0;
		int max_w = 0;
		int current_y = 0;
		int max_h = 0;

		int offset = 0;
		int count = 0;
		uint8_t value;
		for (int i = 0; count < g_res.spr_monsters_count; ++i) {

			if (g_res.dos_demo && (i >= 305 && i < 312)) {
				/* demo is missing 7 (monster) sprites compared to full game */
				continue;
			}
			const int j = i;

			value = sprite_offsets[j] & 255;
			value = (value >> 3) | ((value & 7) << 5);
			if ((value & 0xE0) != 0) {
				value &= ~0xE0;
				++value;
			}
			const int h = sprite_offsets[j] >> 8;
			const int w = value * 8;
			assert((sprite_offsets[j] & 255) == w);
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
			decode_planar(g_res.sprites + offset, data + current_y * MAX_SPRITESHEET_W + current_x, MAX_SPRITESHEET_W, w, h, 0xFF);
			offset += size;

			r[i].x = current_x;
			r[i].y = current_y;
			r[i].w = w;
			r[i].h = h;
			current_x += w;

			++count;
		}
		assert(count <= MAX_SPRITES);
		assert(max_w <= MAX_SPRITESHEET_W);
		assert(current_y + max_h <= MAX_SPRITESHEET_H);
		g_sys.render_unload_sprites(RENDER_SPR_GAME);
		g_sys.render_load_sprites(RENDER_SPR_GAME, count, r, data, MAX_SPRITESHEET_W, current_y + max_h, 0, 0x0);
		free(data);
		print_debug(DBG_SCREEN, "sprites total_size %d count %d", offset, count);
	}
}

void video_draw_sprite(int num, int x, int y, int flag) {
	g_sys.render_add_sprite(RENDER_SPR_GAME, num, x, y, flag != 0);
}

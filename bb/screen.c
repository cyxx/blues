
#include "decode.h"
#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

#define MAX_SPRITESHEET_W 512
#define MAX_SPRITESHEET_H 512

static int _offset_x, _offset_y;

void screen_init() {
	memset(g_res.vga, 0, GAME_SCREEN_W * GAME_SCREEN_H);
	_offset_x = (GAME_SCREEN_W > 320) ? (GAME_SCREEN_W - 320) / 2 : 0; // center horizontally
	_offset_y = (GAME_SCREEN_H > 200) ? (GAME_SCREEN_H - 200) : 0; // align to bottom
}

void screen_clear_sprites() {
	g_sys.render_clear_sprites();
}

static void add_game_sprite(int x, int y, int frame, int xflip) {
	const uint8_t *p = g_res.spr_frames[frame];
	if (!p) {
		print_warning("add_game_sprite missing frame %d", frame);
		return;
	}
	const int h = READ_LE_UINT16(p - 4);
	const int w = READ_LE_UINT16(p - 2);
	int spr_type = RENDER_SPR_GAME;
	if (frame >= SPRITES_COUNT) {
		spr_type = RENDER_SPR_LEVEL;
		frame -= SPRITES_COUNT;
	} else {
		if (y >= 161 && frame >= 120) {
			x += _offset_x;
			y += _offset_y;
		}
	}
	g_sys.render_add_sprite(spr_type, frame, x - w / 2, y - h, xflip);
}

void screen_add_sprite(int x, int y, int frame) {
	if (g_res.amiga_data) {
		switch (frame) {
		case 125: {
				extern const uint8_t spr71a6_amiga[];
				static const int w = 16;
				static const int h = 6;
				decode_amiga_gfx(g_res.vga + (y - h) * GAME_SCREEN_W + (x - w / 2), GAME_SCREEN_W, w, h, 4, spr71a6_amiga, w, 0x0, 0xFFFF);
			}
			return;
		case 126: {
				extern const uint8_t spr71da_amiga[];
				static const int w = 48;
				static const int h = 34;
				decode_amiga_gfx(g_res.vga + (y - h) * GAME_SCREEN_W + (x - w / 2), GAME_SCREEN_W, w, h, 4, spr71da_amiga, w, 0x0, 0xFFFF);
			}
			return;
		}
	}
	add_game_sprite(x, y, frame, 0);
}

void screen_redraw_sprites() {
}

void fade_out_palette() {
	// g_sys.fade_out_palette();
}

void screen_adjust_palette_color(int color, int b, int c) {
	if (!g_options.cga_colors) {
		g_res.palette[color * 3 + b] += c;
		screen_vsync();
		g_sys.set_screen_palette(g_res.palette, 0, 16, 8);
	}
}

void screen_vsync() {
}

void screen_draw_frame(const uint8_t *frame, int fh, int fw, int x, int y) {
	x += _offset_x;
	if (y == 161) {
		y += _offset_y;
	}
	y += fh + 2;
	if (g_options.amiga_status_bar || g_res.amiga_data) {
		if (frame == g_res.spr_frames[123] || frame == g_res.spr_frames[124]) { // top or bottom status bar
			for (int x = 0; x < GAME_SCREEN_W; x += 16) {
				decode_amiga_gfx(g_res.vga + y * GAME_SCREEN_W + x, GAME_SCREEN_W, 16, 12, 4, frame, 16, 0x20, 0xFFFF);
			}
		} else {
			decode_amiga_gfx(g_res.vga + y * GAME_SCREEN_W + x, GAME_SCREEN_W, 16, 12, 4, frame, 16, 0x20, 0xFFFF);
		}
	} else {
		const int h = READ_LE_UINT16(frame - 4);
		assert(fh <= h);
		const int w = READ_LE_UINT16(frame - 2);
		assert(fw <= w);
		decode_spr(frame, w, fw, h, 4, g_res.vga, GAME_SCREEN_W, x, y, false);
	}
}

void screen_flip() {
	g_sys.update_screen(g_res.vga, 1);
}

void screen_unk5() {
	// screen_do_transition2();
	screen_clear(0);
}

void screen_do_transition1(int a) {
	// print_warning("screen_do_transition1 %d", a);
	struct sys_rect_t s;
	s.h = GAME_SCREEN_H;
	s.w = GAME_SCREEN_W;
	if (a) {
		g_sys.transition_screen(&s, TRANSITION_CURTAIN, true);
	}
}

void screen_do_transition2() {
	// print_warning("screen_do_transition2");
	struct sys_rect_t s;
	s.h = GAME_SCREEN_H;
	s.w = GAME_SCREEN_W;
	g_sys.transition_screen(&s, TRANSITION_SQUARE, true);
}

void screen_clear(int a) {
	memset(g_res.vga, 0, GAME_SCREEN_W * GAME_SCREEN_H);
}

void screen_draw_tile(int tile, int type, int x, int y) {
	const uint8_t *src = g_res.tiles + tile * 16;
	if (type != 0) {
		src += 320;
	}
	x = g_vars.screen_tilemap_xoffset + x * 16;
	int tile_w = 16;
	if (x < 0) {
		tile_w += x;
		src -= x;
		x = 0;
	}
	if (x + tile_w > TILEMAP_SCREEN_W) {
		tile_w = TILEMAP_SCREEN_W - x;
	}
	if (tile_w <= 0) {
		return;
	}
	y = g_vars.screen_tilemap_yoffset + y * 16;
	int tile_h = 16;
	if (y < 0) {
		tile_h += y;
		src -= y * 640;
		y = 0;
	}
	if (y + tile_h > TILEMAP_SCREEN_H) {
		tile_h = TILEMAP_SCREEN_H - y;
	}
	if (tile_h <= 0) {
		return;
	}
	for (int i = 0; i < tile_h; ++i) {
		memcpy(g_res.vga + (TILEMAP_OFFSET_Y + y + i) * GAME_SCREEN_W + x, src, tile_w);
		src += 640;
	}
}

static void draw_number_amiga(int digit, int x, int y) {
	extern const uint8_t icon7086[]; // 01
	extern const uint8_t icon70ea[]; // 23
	extern const uint8_t icon714e[]; // 45
	extern const uint8_t icon71b2[]; // 67
	extern const uint8_t icon7216[]; // 89
	static const uint8_t *icons[] = { icon7086, icon70ea, icon714e, icon71b2, icon7216 };
	uint16_t mask = 0xFF00;
	if (digit & 1) {
		x -= 8;
		mask >>= 8;
	}
	decode_amiga_gfx(g_res.vga + y * GAME_SCREEN_W + x, GAME_SCREEN_W, 16, 12, 4, icons[digit >> 1], 16, 0x20, mask);
}

void screen_draw_number(int num, int x, int y, int color) {
	if (y >= 161) {
		x += _offset_x;
		y += _offset_y;
	}
	y += TILEMAP_OFFSET_Y;
	if (g_options.amiga_status_bar || g_res.amiga_data) {
		draw_number_amiga(num / 10, x - 8, y - 2);
		draw_number_amiga(num % 10, x,     y - 2);
	} else {
		extern const uint8_t font_data[];
		decode_spr(font_data + (num / 10) * 32, 8, 8, 8, 4, g_res.vga, GAME_SCREEN_W, x - 8, y, false);
		decode_spr(font_data + (num % 10) * 32, 8, 8, 8, 4, g_res.vga, GAME_SCREEN_W, x,     y, false);
	}
}

void screen_add_game_sprite1(int x, int y, int frame) {
	add_game_sprite(x, y + TILEMAP_OFFSET_Y, frame, 0);
}

void screen_add_game_sprite2(int x, int y, int frame) {
	add_game_sprite(x, y + TILEMAP_OFFSET_Y, frame, 1);
}

void screen_add_game_sprite3(int x, int y, int frame, int blinking_counter) {
//	print_warning("screen_add_game_sprite3");
}

void screen_add_game_sprite4(int x, int y, int frame, int blinking_counter) {
//	print_warning("screen_add_game_sprite4");
}

static void dither_graphics(uint8_t *dst, int dst_pitch, int w, int h, const uint8_t *dither_lut, uint8_t color_key) {
	for (int y = 0; y < h; ++y) {
		const uint8_t *p = dither_lut + (y & 1) * 0x10;
		for (int x = 0; x < w; ++x) {
			const uint8_t color = dst[x];
			if (color == 0) {
				dst[x] = color_key;
			} else {
				dst[x] = p[color];
			}
		}
		dst += dst_pitch;
	}
}

static void decode_graphics(int spr_type, int start, int end, const uint8_t *dither_lut) {
	struct sys_rect_t r[MAX_SPR_FRAMES];
	uint8_t *data = (uint8_t *)calloc(MAX_SPRITESHEET_W * MAX_SPRITESHEET_H, 1);
	if (data) {
		const uint8_t color_key = dither_lut ? 0x20 : 0;
		const int depth = g_res.amiga_data && (start == 0) ? 3 : 4;
		int current_x = 0;
		int max_w = 0;
		int current_y = 0;
		int max_h = 0;
		for (int i = start; i < end; ++i) {
			const uint8_t *ptr = g_res.spr_frames[i];
			if (!ptr) {
				memset(&r[i], 0, sizeof(struct sys_rect_t));
				continue;
			}
			const int h = READ_LE_UINT16(ptr - 4);
			const int w = READ_LE_UINT16(ptr - 2);
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
			decode_spr(ptr, w, w, h, depth, data, MAX_SPRITESHEET_W, current_x, current_y, g_res.amiga_data);
			if (dither_lut) {
				dither_graphics(data + current_y * MAX_SPRITESHEET_W + current_x, MAX_SPRITESHEET_W, w, h, dither_lut, color_key);
			}
			const int j = i - start;
			r[j].x = current_x;
			r[j].y = current_y;
			r[j].w = w;
			r[j].h = h;
			current_x += w;
			if (h > max_h) {
				max_h = h;
			}
		}
		if (g_res.amiga_data && start == 0) {
			for (int i = 0; i < MAX_SPRITESHEET_W * MAX_SPRITESHEET_H; ++i) {
				if (data[i] != 0) {
					data[i] |= 16;
				}
			}
		}
		assert(max_w <= MAX_SPRITESHEET_W);
		assert(current_y + max_h <= MAX_SPRITESHEET_H);
		g_sys.render_unload_sprites(spr_type);
		g_sys.render_load_sprites(spr_type, end - start, r, data, MAX_SPRITESHEET_W, current_y + max_h, color_key, false);
		free(data);
	}
}

void screen_load_graphics(const uint8_t *dither_lut_sqv, const uint8_t *dither_lut_avt) {
	if (g_res.spr_count <= SPRITES_COUNT) {
		decode_graphics(RENDER_SPR_GAME, 0, g_res.spr_count, dither_lut_sqv);
	} else {
		decode_graphics(RENDER_SPR_LEVEL, SPRITES_COUNT, g_res.spr_count, dither_lut_sqv);
		// foreground tiles
		const uint8_t color_key = dither_lut_avt ? 0x20 : 0;
		struct sys_rect_t r[MAX_SPR_FRAMES];
		static const int FG_TILE_W = 16;
		static const int FG_TILE_H = 16;
		uint8_t *data = (uint8_t *)malloc(g_res.avt_count * FG_TILE_W * FG_TILE_H);
		if (data) {
			const int pitch = g_res.avt_count * FG_TILE_W;
			for (int i = 0; i < g_res.avt_count; ++i) {
				decode_spr(g_res.avt[i], FG_TILE_W, FG_TILE_W, FG_TILE_H, 4, data, pitch, i * FG_TILE_W, 0, false);
				r[i].x = i * FG_TILE_W;
				r[i].y = 0;
				r[i].w = FG_TILE_W;
				r[i].h = FG_TILE_H;
			}
			if (dither_lut_avt) {
				dither_graphics(data, FG_TILE_W * g_res.avt_count, FG_TILE_W * g_res.avt_count, FG_TILE_H, dither_lut_avt, color_key);
			}
			g_sys.render_unload_sprites(RENDER_SPR_FG);
			g_sys.render_load_sprites(RENDER_SPR_FG, g_res.avt_count, r, data, g_res.avt_count * FG_TILE_W, FG_TILE_H, color_key, false);
			free(data);
		}
		// background tiles (Amiga) - re-arrange to match DOS .ck1/.ck2 layout
		if (g_res.amiga_data) {
			static const int BG_TILES_COUNT = 256;
			static const int W = 320 / 16;
			memcpy(g_res.vga, g_res.tiles, BG_TILES_COUNT * 16 * 8);
			for (int i = 0; i < 128; ++i) {
				const int y = (i / W);
				const int x = (i % W);
				decode_amiga_blk(g_res.vga + i * 16 * 8, g_res.tiles + (y * 640 + x) * 16, 640);
			}
			for (int i = 128; i < 256; ++i) {
				const int y = ((i - 128) / W);
				const int x = ((i - 128) % W) + W;
				decode_amiga_blk(g_res.vga + i * 16 * 8, g_res.tiles + (y * 640 + x) * 16, 640);
			}
		}
	}
}

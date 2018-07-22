
#include "decode.h"
#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

#define MAX_SPRITESHEET_W 512
#define MAX_SPRITESHEET_H 512

void screen_init() {
	memset(g_res.vga, 0, GAME_SCREEN_W * GAME_SCREEN_H);
}

void screen_clear_sprites() {
	render_clear_sprites();
}

static void add_game_sprite(int x, int y, int frame, int xflip) {
	const uint8_t *p = g_res.spr_frames[frame];
	const int h = READ_LE_UINT16(p - 4);
	const int w = READ_LE_UINT16(p - 2);
	int spr_type = RENDER_SPR_GAME;
	if (frame >= SPRITES_COUNT) {
		spr_type = RENDER_SPR_LEVEL;
		frame -= SPRITES_COUNT;
	}
	render_add_sprite(spr_type, frame, x - w / 2, y - h, xflip);
}

void screen_add_sprite(int x, int y, int frame) {
	add_game_sprite(x, y, frame, 0);
}

void screen_clear_last_sprite() {
}

void screen_redraw_sprites() {
}

void fade_in_palette() {
	g_sys.fade_in_palette();
}

void fade_out_palette() {
	// g_sys.fade_out_palette();
}

void screen_adjust_palette_color(int color, int b, int c) {
	g_res.palette[color * 3 + b] += c;
	screen_vsync();
	g_sys.set_screen_palette(g_res.palette, 16);
}

void screen_vsync() {
}

void screen_draw_frame(const uint8_t *frame, int fh, int fw, int x, int y) {
	if (GAME_SCREEN_W > 320) {
		x += (GAME_SCREEN_W - 320) / 2;
	}
	if (GAME_SCREEN_H > 200 && y == 161) { // align to the bottom
		y += GAME_SCREEN_H - 200;
	}
	y += fh + 2;
	if (g_options.amiga_status_bar) {
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

void screen_unk4() {
	memcpy(g_res.vga, g_res.tmp + 32000, 64000);
}

void screen_unk5() {
	screen_clear(0);
	screen_do_transition2();
	screen_clear(0);
}

void screen_unk6() {
	// g_vars.screen_draw_offset -= 12;
	// screen_do_transition2();
	// g_vars.screen_draw_offset += 12;
	g_sys.update_screen(g_res.vga, 1);
	memset(g_res.vga, 0, GAME_SCREEN_W * GAME_SCREEN_H);
}

static void screen_unk13(int a) {
}

void screen_do_transition1(int a) {
	int i, count, increment;
	if (a != 0) {
		i = 11;
		count = 0;
		increment = -1;
	} else {
		screen_clear(0);
		i = 0;
		count = 11;
		increment = 1;
	}
	while (i != count) {
		screen_unk13(i);
		screen_unk13(19 - i);
		screen_vsync();
		i += increment;
	}
}

void screen_do_transition2() {
	print_warning("screen_do_transition2");
}

void screen_clear(int a) {
	memset(g_res.vga, 0, GAME_SCREEN_W * GAME_SCREEN_H);
}

void screen_draw_tile(int tile, int dst, int type) {
	const int y = (dst / 640) * 16 + TILEMAP_OFFSET_Y;
	const int x = (dst % 640) / 2 * 16;
	const uint8_t *src = g_res.tiles + tile * 16;
	if (type == 4) {
		src += 320;
	}
	for (int i = 0; i < 16; ++i) {
		memcpy(g_res.vga + (y + i) * GAME_SCREEN_W + x, src, 16);
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
	y += TILEMAP_OFFSET_Y;
	if (g_options.amiga_status_bar) {
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

static void decode_graphics(int spr_type, int start, int end) {
	struct sys_rect_t r[MAX_SPR_FRAMES];
	uint8_t *data = (uint8_t *)calloc(MAX_SPRITESHEET_W * MAX_SPRITESHEET_H, 1);
	if (data) {
		const int depth = g_options.amiga_data && (start == 0) ? 3 : 4;
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
			const int j = i - start;
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
			decode_spr(ptr, w, w, h, depth, data, MAX_SPRITESHEET_W, current_x, current_y, g_options.amiga_data);
			r[j].x = current_x;
			r[j].y = current_y;
			r[j].w = w;
			r[j].h = h;
			current_x += w;
			if (h > max_h) {
				max_h = h;
			}
		}
		assert(max_w <= MAX_SPRITESHEET_W);
		assert(current_y + max_h <= MAX_SPRITESHEET_H);
		render_unload_sprites(spr_type);
		const int palette_offset = (g_options.amiga_data && start == 0) ? 16 : 0;
		render_load_sprites(spr_type, end - start, r, data, MAX_SPRITESHEET_W, current_y + max_h, palette_offset);
		free(data);
	}
}

void screen_load_graphics() {
	if (g_res.spr_count <= SPRITES_COUNT) {
		decode_graphics(RENDER_SPR_GAME, 0, g_res.spr_count);
	} else {
		decode_graphics(RENDER_SPR_LEVEL, SPRITES_COUNT, g_res.spr_count);
		// foreground tiles
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
			render_unload_sprites(RENDER_SPR_FG);
			render_load_sprites(RENDER_SPR_FG, g_res.avt_count, r, data, g_res.avt_count * FG_TILE_W, FG_TILE_H, 0);
			free(data);
		}
		// background tiles (Amiga) - re-arrange to match DOS .ck1/.ck2 layout
		if (g_options.amiga_data) {
			static const int BG_TILES_COUNT = 256;
			static const int W = 320 / 16;
			memcpy(g_res.tmp, g_res.tiles, BG_TILES_COUNT * 16 * 8);
			for (int i = 0; i < 128; ++i) {
				const int y = (i / W);
				const int x = (i % W);
				decode_amiga_blk(g_res.tmp + i * 16 * 8, g_res.tiles + (y * 640 + x) * 16, 640);
			}
			for (int i = 128; i < 256; ++i) {
				const int y = ((i - 128) / W);
				const int x = ((i - 128) % W) + W;
				decode_amiga_blk(g_res.tmp + i * 16 * 8, g_res.tiles + (y * 640 + x) * 16, 640);
			}
		}
	}
}

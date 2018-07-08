
#include "decode.h"
#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

void screen_init() {
	for (int i = 0; i < MAX_SPRITES; ++i) {
		g_vars.sprites[i] = &g_vars.sprites_table[i];
	}
	for (int i = 0; i < MAX_SPRITES - 1; ++i) {
		g_vars.sprites[i]->next_sprite = g_vars.sprites[i] + 1;
	}
	g_vars.sprites[MAX_SPRITES - 1]->next_sprite = 0;
}

void screen_clear_sprites() {
	g_vars.sprites_count = 0;
}

void screen_add_sprite(int x, int y, int frame) {
	assert(g_vars.sprites_count < MAX_SPRITES);
	struct sprite_t *spr = g_vars.sprites[g_vars.sprites_count];
	spr->xpos = x;
	spr->ypos = y;
	spr->frame = g_res.spr_frames[frame];
	spr->xflip = 0;
	spr->unk16 = 0;
	spr->next_sprite = spr + 1;
	++g_vars.sprites_count;
}

void screen_clear_last_sprite() {
	assert(g_vars.sprites_count >= 0);
	if (g_vars.sprites_count > 0) {
		g_vars.sprites[g_vars.sprites_count - 1]->next_sprite = 0;
	}
}

void screen_redraw_sprites() {
	for (int i = 0; i < g_vars.sprites_count; ++i) {
		struct sprite_t *spr = g_vars.sprites[i];
		const uint8_t *p = spr->frame;
		const int h = READ_LE_UINT16(p - 4);
		const int w = READ_LE_UINT16(p - 2);
		const int y = spr->ypos - h;
		const int x = spr->xpos - w / 2;
		decode_ega_spr(p, w, w, h, g_res.vga, GAME_SCREEN_W, x, y, spr->xflip);
	}
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
	screen_set_palette();
}

void screen_vsync() {
}

void screen_set_palette() {
	g_sys.set_screen_palette(g_res.palette, 16);
}

void screen_draw_frame(const uint8_t *frame, int a, int b, int c, int d) {
	const int h = READ_LE_UINT16(frame - 4);
	assert(a <= h);
	const int w = READ_LE_UINT16(frame - 2);
	assert(b <= w);
	const int x = c;
	const int y = d + a + 2;
	decode_ega_spr(frame, w, b, h, g_res.vga, GAME_SCREEN_W, x, y, 0);
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
	// _screen_draw_offset -= 12;
	// screen_do_transition2();
	// _screen_draw_offset += 12;
	g_sys.update_screen(g_res.vga, 1);
	memset(g_res.vga, 0, GAME_SCREEN_W * GAME_SCREEN_H);
}

void screen_copy_tilemap2(int a, int b, int c, int d) {
}

void screen_copy_tilemap(int a) {
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

void screen_clear(int a) {
	memset(g_res.vga, 0, GAME_SCREEN_W * GAME_SCREEN_H);
}

void screen_draw_tile(int tile, int dst, int type) {
	int y = (dst / 640) * 16 + TILEMAP_OFFSET_Y;
	int x = (dst % 640) / 2 * 16;
	const uint8_t *src = g_res.tiles + tile * 16;
	if (type == 4) {
		src += 320;
	}
	for (int i = 0; i < 16; ++i) {
		memcpy(g_res.vga + (y + i) * GAME_SCREEN_W + x, src, 16);
		src += 640;
	}
}

void screen_do_transition2() {
	print_warning("screen_do_transition2");
}

void screen_draw_number(int num, int x, int y, int color) {
	extern const uint8_t font_data[];
	y += TILEMAP_OFFSET_Y;
	decode_ega_spr(font_data + (num / 10) * 32, 8, 8, 8, g_res.vga, GAME_SCREEN_W, x - 8, y, 0);
	decode_ega_spr(font_data + (num % 10) * 32, 8, 8, 8, g_res.vga, GAME_SCREEN_W, x,     y, 0);
}

void screen_add_game_sprite1(int x, int y, int frame) {
	screen_add_sprite(x, y + TILEMAP_OFFSET_Y, frame);
	g_vars.sprites[g_vars.sprites_count - 1]->xflip = 0;
}

void screen_add_game_sprite2(int x, int y, int frame) {
	screen_add_sprite(x, y + TILEMAP_OFFSET_Y, frame);
	g_vars.sprites[g_vars.sprites_count - 1]->xflip = 1;
}

void screen_add_game_sprite3(int x, int y, int frame, int blinking_counter) {
	print_warning("screen_add_game_sprite3");
}

void screen_add_game_sprite4(int x, int y, int frame, int blinking_counter) {
	print_warning("screen_add_game_sprite4");
}


/* game screens */

#include "game.h"
#include "resource.h"
#include "sys.h"

struct vars_t g_vars;

void update_input() {
	g_sys.process_events();

	g_vars.input_key_left  = (g_sys.input.direction & INPUT_DIRECTION_LEFT) != 0  ? 0xFF : 0;
	g_vars.input_key_right = (g_sys.input.direction & INPUT_DIRECTION_RIGHT) != 0 ? 0xFF : 0;
	g_vars.input_key_up    = (g_sys.input.direction & INPUT_DIRECTION_UP) != 0    ? 0xFF : 0;
	g_vars.input_key_down  = (g_sys.input.direction & INPUT_DIRECTION_DOWN) != 0  ? 0xFF : 0;
	g_vars.input_key_space = g_sys.input.space ? 0xFF : 0;

	g_vars.input_keystate[2] = g_sys.input.digit1;
	g_vars.input_keystate[3] = g_sys.input.digit2;
	g_vars.input_keystate[4] = g_sys.input.digit3;
}

static void wait_input(int timeout) {
	const uint32_t end = g_sys.get_timestamp() + timeout * 10;
	while (g_sys.get_timestamp() < end) {
		g_sys.process_events();
		if (g_sys.input.quit || g_sys.input.space) {
			break;
		}
		g_sys.sleep(20);
	}
}

static void do_splash_screen() {
	load_file("titus.eat");
	video_copy_vga(0x7D00);
	fade_in_palette();
	fade_out_palette();
	load_file("tiny.eat");
	video_copy_vga(0x7D00);
	fade_in_palette();
	fade_out_palette();
}

static void scroll_screen_palette() {
	++g_vars.level_time;
	if (g_vars.level_time >= 30) {
		g_vars.level_time = 0;
	}
	const int count = 30 - g_vars.level_time;
	for (int i = 0; i < count; ++i) {
		g_sys.set_palette_color(225 + i, g_res.tmp + (225 + g_vars.level_time + i) * 3);
	}
	g_sys.update_screen(g_res.vga, 1);
}

static void do_select_screen_scroll_palette(int start, int end, int step, int count) {
	uint8_t *palette_buffer = g_res.tmp;
	do {
		for (int i = start * 3; i < end * 3; ++i) {
			int color = g_vars.palette_buffer[i];
			if ((step > 0 && color != palette_buffer[i]) || (step < 0 && color != 0)) {
				color += step;
			}
			g_vars.palette_buffer[i] = color;
		}
		g_sys.set_screen_palette(g_vars.palette_buffer + start * 3, start, end - start + 1, 6);
		g_sys.update_screen(g_res.vga, 1);
		g_sys.sleep(20);
	} while (--count != 0);
}

static void do_select_screen_scroll_palette_pattern1() {
	do_select_screen_scroll_palette(0x10, 0x4F, -1, 0x19);
}

static void do_select_screen_scroll_palette_pattern2() {
	do_select_screen_scroll_palette(0x60, 0x9F, -1, 0x19);
}

static void do_select_screen_scroll_palette_pattern3() {
	do_select_screen_scroll_palette(0x10, 0x4F,  1, 0x19);
}

static void do_select_screen_scroll_palette_pattern4() {
	do_select_screen_scroll_palette(0x60, 0x9F,  1, 0x19);
}

static void do_select_screen() {
	load_file("select.eat");
	video_copy_vga(0x7D00);
	fade_in_palette();
	memcpy(g_vars.palette_buffer, g_res.tmp, 256 * 3);
	do_select_screen_scroll_palette_pattern2();
	int bl = 2;
	while (!g_sys.input.quit) {
		int bh = bl;
		if (g_sys.input.direction & INPUT_DIRECTION_RIGHT) {
			g_sys.input.direction &= ~INPUT_DIRECTION_RIGHT;
			++bl;
			if (bl > 2) {
				bl = 2;
			}
		}
		if (g_sys.input.direction & INPUT_DIRECTION_LEFT) {
			g_sys.input.direction &= ~INPUT_DIRECTION_LEFT;
			--bl;
			if (bl < 1) {
				bl = 1;
			}
		}
		bh ^= bl;
		if (bh & 1) {
			if (bl & 1) {
				do_select_screen_scroll_palette_pattern4();
			} else {
				do_select_screen_scroll_palette_pattern2();
			}
		}
		if (bh & 2) {
			if (bl & 2) {
				do_select_screen_scroll_palette_pattern3();
			} else {
				do_select_screen_scroll_palette_pattern1();
			}
		}
		if (g_sys.input.space) {
			assert(bl == 1 || bl == 2);
			g_sys.input.space = 0;
			g_vars.player = 1 - ((bl & 3) - 1);
			fade_out_palette();
			break;
		}
		update_input();
		g_sys.sleep(30);
	}
}

void do_difficulty_screen() {
	char name[16];
	snprintf(name, sizeof(name), "dif%02d.eat", (g_vars.level >> 3) + 1);
	load_file(name);
	video_copy_vga(0x7D00);
	fade_in_palette();
	wait_input(560);
	fade_out_palette();
}

void do_level_number_screen() {
	load_file("fond.eat");
	video_draw_string("LEVEL NUMBER", 0x5E0C, 11);
	char buf[8];
	snprintf(buf, sizeof(buf), "%02d", g_vars.level);
	video_draw_string(buf, 0x5E9B, 11);
	video_copy_vga(0x7D00);
	fade_in_palette();
	fade_out_palette();
}

static uint16_t rol16(uint16_t x, int c) {
	return (x << c) | (x >> (16 - c));
}

static uint16_t get_password(uint16_t x) {
	x ^= 0xaa31;
	x *= 0xb297;
	return rol16(x, 3);
}

void do_level_password_screen() {
	load_file("password.eat");
	uint16_t ax = get_password(g_vars.player * 50 + g_vars.level - 1);
	char str[5];
	for (int i = 0; i < 4; ++i, ax <<= 4) {
		const uint8_t dx = (ax >> 12) + '0';
		str[i] = (dx <= '9') ? dx : (dx + 7);
	}
	str[4] = 0;
	video_draw_string("STAGE NUMBER", 0x7E96, 11);
	video_draw_string(str, 0xABB4, 20);
	video_copy_vga(0x7D00);
	fade_in_palette();
	scroll_screen_palette();
	wait_input(64000);
	fade_out_palette();
}

static void do_password_screen() {
	load_file("password.eat");
	video_draw_string("ENTER PASSWORD", 0x7E96, 11);
	fade_in_palette();
	char str[5] = "0000";
	video_draw_string(str, 0xABB4, 20);
}

static int do_menu_screen() {
	load_file("menu.eat");
	video_copy_vga(0x7D00);
	fade_in_palette();
	memset(g_vars.input_keystate, 0, sizeof(g_vars.input_keystate));
	g_vars.level_time = 0;
	while (!g_sys.input.quit) {
		scroll_screen_palette();
		if (g_vars.input_keystate[2] || g_vars.input_keystate[0x4F] || g_sys.input.space) {
			g_sys.input.space = 0;
			fade_out_palette();
			return 1;
		}
		if (g_vars.input_keystate[3] || g_vars.input_keystate[0x50]) {
			fade_out_palette();
			return 2;
		}
		if (g_vars.input_keystate[4] || g_vars.input_keystate[0x51]) {
			return 3;
		}
		update_input();
		g_sys.sleep(30);
	}
	return 0;
}

static int do_options_screen() {
	fade_out_palette();
	load_file("fond.eat");
	video_draw_string("GAME SPEED", 0x3EE9, 11);
	video_draw_string("1 FAST", 0x647E, 11);
	video_draw_string("2 NORMAL", 0x89FE, 11);
	video_copy_vga(0x7D00);
	fade_in_palette();
	memset(g_vars.input_keystate, 0, sizeof(g_vars.input_keystate));
	while (!g_sys.input.quit) {
		scroll_screen_palette();
		if (g_vars.input_keystate[2] || g_vars.input_keystate[0x4F]) {
			fade_out_palette();
			return 1;
		}
		if (g_vars.input_keystate[3] || g_vars.input_keystate[0x50]) {
			fade_out_palette();
			return 2;
		}
		update_input();
		g_sys.sleep(30);
	}
	return 0;
}

void do_game_over_screen() {
	load_file("fond.eat");
	video_draw_string("GAME OVER", 0x5E2E, 11);
	video_copy_vga(0x7D00);
	fade_in_palette();
	wait_input(64000);
	fade_out_palette();
}

void do_game_win_screen() {
	load_file("win.eat");
	video_copy_vga(0x7D00);
	fade_in_palette();
	fade_out_palette();
	load_file("end.eat");
	video_copy_vga(0xB500);
	static const int count = 5;
	static const struct {
		uint16_t offset;
		const char *str;
	} text[] = {
		{ 0x0F68, "CONGRATULATION" },
		{ 0x3B34, "YOU HAVE BEATEN" },
		{ 0x5CE8, "THE EVIL JUKEBOXE" },
		{ 0x7EB1, "NOW YOU ARE FREE" },
		{ 0xA072, "AND YOUR CONCERT" },
		{ 0xC22D, "WILL BE A SUCCESS" },
		{ 0xFFFF, 0 },
		{ 0x33BE, "PC CONVERSION" },
		{ 0x5590, "ERIC ZMIRO" },
		{ 0x864D, "PC GRAPHICS" },
		{ 0xA7FC, "DIDIER CARRERE" },
		{ 0xFFFF, 0 },
		{ 0x33AF, "ORIGINAL VERSION" },
		{ 0x5594, "ERIC CAEN" },
		{ 0x862B, "ORIGINAL GRAPHICS" },
		{ 0xA835, "BOB" },
		{ 0xFFFF, 0 },
		{ 0x33B1, "ORIGINAL MUSICS" },
		{ 0x559A, "DIMITRI" },
		{ 0x8653, "PC MUSICS" },
		{ 0xA7F2, "MICHAEL KNAEPEN" },
		{ 0xFFFF, 0 },
		{ 0x5A8F, "THANK YOU" },
		{ 0x8008, "FOR PLAYING" },
		{ 0xFFFF, 0 }
	};
	int i = 0;
	for (int j = 0; j < count; ++j) {
		for (; text[i].str; ++i) {
			video_draw_string(text[i].str, text[i].offset, 11);
		}
		++i;
		video_copy_vga(0x7D00);
		fade_in_palette();
		wait_input(64000);
		fade_out_palette();
		memcpy(g_res.tmp + 768, g_res.background, 64000);
	}
}

void game_main() {
	play_music(0);
	do_splash_screen();
	g_sys.set_screen_palette(common_palette_data, 0, 128, 6);
	video_load_sprites();
	g_sys.render_set_sprites_clipping_rect(0, 0, TILEMAP_SCREEN_W, TILEMAP_SCREEN_H);
	while (!g_sys.input.quit) {
		update_input();
		g_vars.level = g_options.start_level;
		if (g_vars.level == 0) {
			g_vars.level = 1;
		}
		const int ret = do_menu_screen();
		g_vars.players_table[0].lifes_count = 3;
		g_vars.players_table[1].lifes_count = 3;
		if (ret == 0) {
			break;
		} else if (ret == 1) {
			do_select_screen();
		} else if (ret == 2) {
			g_vars.level = -1;
			do_password_screen();
			if (g_vars.level < 0) {
				continue;
			}
			++g_vars.level;
		} else {
			do_options_screen();
			continue;
		}
		do_level();
	}
}

static void game_run(const char *data_path) {
	res_init(data_path, GAME_SCREEN_W * GAME_SCREEN_H);
	sound_init();
	game_main();
	sound_fini();
	res_fini();
}

struct game_t game = {
	"Blues Brothers : Jukebox Adventure",
	game_run
};

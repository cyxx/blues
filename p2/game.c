
#include <time.h>
#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

struct vars_t g_vars;

void update_input() {
	g_sys.process_events();

	g_vars.input.key_left  = (g_sys.input.direction & INPUT_DIRECTION_LEFT) != 0  ? 0xFF : 0;
	g_vars.input.key_right = (g_sys.input.direction & INPUT_DIRECTION_RIGHT) != 0 ? 0xFF : 0;
	g_vars.input.key_up    = (g_sys.input.direction & INPUT_DIRECTION_UP) != 0    ? 0xFF : 0;
	g_vars.input.key_down  = (g_sys.input.direction & INPUT_DIRECTION_DOWN) != 0  ? 0xFF : 0;
	g_vars.input.key_space = g_sys.input.space ? 0xFF : 0;

	g_vars.input.keystate[2] = g_sys.input.digit1;
	g_vars.input.keystate[3] = g_sys.input.digit2;
	g_vars.input.keystate[4] = g_sys.input.digit3;
}

static void wait_input(int timeout) {
	const uint32_t end = g_sys.get_timestamp() + timeout;
	while (g_sys.get_timestamp() < end) {
		g_sys.process_events();
		if (g_sys.input.quit || g_sys.input.space) {
			break;
		}
		g_sys.sleep(2);
	}
}

static void do_programmed_in_1992_screen() {
	time_t now;
	time(&now);
	struct tm *t = localtime(&now);
	if (t->tm_year + 1900 < 1996) { /* || t->tm_year + 1900 >= 2067 */
		return;
	}
	video_clear();
	g_sys.set_screen_palette(credits_palette_data, 0, 16, 6);
	int offset = 0x960;
	video_draw_string(offset, 5, "YEAAA > > >");
	char str[64];
	snprintf(str, sizeof(str), "MY GAME IS STILL WORKING IN %04d <<", 1900 + t->tm_year);
	offset += 0x1E0;
	video_draw_string(offset, 0, str);
	offset = 0x1680;
	video_draw_string(offset, 1, "PROGRAMMED IN 1992 ON AT >286 12MHZ>");
	offset += 0x1E0;
	video_draw_string(offset, 3, "> > > ENJOY OLDIES<<");
	g_sys.update_screen(g_res.vga, 1);
	wait_input(1000);
	video_clear();
}

static void do_credits() {
	g_sys.set_screen_palette(credits_palette_data, 0, 16, 6);
	int offset = 0x140;
	video_draw_string(offset, 1, "CODER> DESIGNER AND ARTIST DIRECTOR>");
	offset += 0x230;
	video_draw_string(offset, 14, "ERIC ZMIRO");
	offset += 0x460;
	video_draw_string(offset, 4, ">MAIN GRAPHICS AND BACKGROUND>");
	offset += 0x230;
	video_draw_string(offset, 11, "FRANCIS FOURNIER");
	offset += 0x460;
	video_draw_string(offset, 9, ">MONSTERS AND HEROS>");
	offset += 0x230;
	video_draw_string(offset, 11, "LYES  BELAIDOUNI");
	offset = 0x1770;
	video_draw_string(offset, 15, "THANKS TO");
	offset = 0x1A40;
	video_draw_string(offset, 2, "CRISTELLE> GIL ESPECHE AND CORINNE>");
	offset += 0x1E0;
	video_draw_string(offset, 0, "SEBASTIEN BECHET AND OLIVIER AKA DELTA>");
	g_sys.update_screen(g_res.vga, 1);
}

static void update_screen_img(const uint8_t *src, int present) {
	const int size = GAME_SCREEN_W * GAME_SCREEN_H;
	if (size < 64000) {
		return;
	} else if (size == 64000) {
		g_sys.update_screen(src, present);
	} else {
		memset(g_res.vga, 0, size);
		const int y_offs = (GAME_SCREEN_H - 200) / 2;
		const int x_offs = (GAME_SCREEN_W - 320) / 2;
		for (int y = 0; y < 200; ++y) {
			memcpy(g_res.vga + (y_offs + y) * GAME_SCREEN_W + x_offs, src + y * 320, 320);
		}
		g_sys.update_screen(g_res.vga, present);
	}
}

static void do_titus_screen() {
	uint8_t *data = load_file("TITUS.SQZ");
	if (data) {
		g_sys.set_screen_palette(data, 0, 256, 6);
		update_screen_img(data + 768, 0);
		g_sys.fade_in_palette();
		wait_input(700);
		g_sys.fade_out_palette();
		free(data);
	}
}

static bool fade_palettes(const uint8_t *target, uint8_t *current) {
	bool flag = false;
	for (int i = 0; i < 768; ++i) {
		int al = current[i];
		const int bh = target[i] - al;
		if (bh != 0) {
			if (abs(bh) < 2) {
				flag = true;
				al = target[i];
			} else {
				if (target[i] < al) {
					al -= 2;
				} else {
					al += 2;
				}
			}
			current[i] = al;
		}
	}
	return flag;
}

static void do_present_screen() {
	uint8_t *data = load_file("PRESENT.SQZ");
	if (data) {
		if (g_uncompressed_size == 65536 + 768) { /* demo version */
			g_sys.set_screen_palette(data, 0, 256, 6);
			update_screen_img(data + 768, 0);
			g_sys.fade_in_palette();
		} else {
			memmove(data + 768, data + 0x1030 * 16, 93 * 320);
			g_sys.set_screen_palette(data, 0, 256, 6);
			update_screen_img(data + 768, 0);
			g_sys.fade_in_palette();
			uint8_t palette[256 * 3];
			memcpy(palette, data, 256 * 3);
			while (fade_palettes(present_palette_data, palette) && !g_sys.input.quit) {
				g_sys.set_screen_palette(palette, 0, 256, 6);
				update_screen_img(data + 768, 1);
				wait_input(100);
			}
		}
		g_sys.fade_out_palette();
		free(data);
	}
}

static const uint8_t joystick_palette_data[] = {
	0x08,0x14,0x22,0x10,0x00,0x00,0x18,0x00,0x00,0x20,0x00,0x08,0x06,0x0F,0x1A,0x28,
	0x10,0x08,0x28,0x18,0x08,0x30,0x20,0x08,0x30,0x28,0x08,0x38,0x30,0x10,0x38,0x30,
	0x18,0x38,0x30,0x20,0x38,0x30,0x28,0x38,0x30,0x30,0x38,0x38,0x38,0x00,0x00,0x00
};

static void do_demo_screen() {
	uint8_t *data = load_file("JOYSTICK.SQZ");
	if (data) {
		video_copy_img(data);
		g_sys.set_screen_palette(joystick_palette_data, 0, 16, 6);
		update_screen_img(g_res.background, 0);
		g_sys.fade_in_palette();
		free(data);
		wait_input(10000);
	}
}

static void do_castle_screen() {
	uint8_t *data = load_file("CASTLE.SQZ");
	if (data) {
		g_sys.set_screen_palette(data, 0, 256, 6);
		update_screen_img(data + 768, 0);
		g_sys.fade_in_palette();
		free(data);
		wait_input(10000);
	}
}

static void do_map(){
	static const uint16_t spr_pos[2][10] = {
		{ 52, 62,  94, 115, 106,  81,  74, 146,  76,  48 },
		{  0, 80, 120, 152, 232, 328, 424, 528, 600, 600 }
	};
	const uint8_t pal = 13;
	const uint16_t spr = 457;
	uint8_t *data = load_file("MAP.SQZ");
	if (data) {
		g_res.map = (uint8_t *)malloc(MAP_W*MAP_H);
		video_copy_map(data);
		video_clear();
		if (g_res.map) {
			const int y_offs = (GAME_SCREEN_H - 200) / 2;
			const int x_offs = GAME_SCREEN_W;
			const uint16_t pos_y = spr_pos[0][g_vars.level_num];
			const uint16_t pos_x = spr_pos[1][g_vars.level_num];
			g_sys.set_screen_palette(palettes_tbl[pal], 0, 16, 6);
			video_load_sprites();
			uint16_t pad_w = GAME_SCREEN_W < MAP_W ? 0 : (GAME_SCREEN_W - MAP_W) / 2;
			for (uint16_t x = 0; x < MAP_W + pad_w; ++x) { /* 640*200*4bpp pic */
				uint16_t pitch = x < GAME_SCREEN_W ? x : MAP_W;
				uint16_t window_w = x < GAME_SCREEN_W ? 0 : x % GAME_SCREEN_W;
				uint16_t shown_pixels = x < GAME_SCREEN_W ? x : 0;
				video_draw_sprite(spr, GAME_SCREEN_W - x + pos_x, pos_y + y_offs, 0);
				for (uint8_t y = 1; y < MAP_H; ++y) {
						memcpy(g_res.vga + (y_offs + y) * GAME_SCREEN_W + x_offs - shown_pixels,
							g_res.map + y * MAP_W + window_w,
							pitch);
						if (x > MAP_W) {
							memset(g_res.vga + (y_offs + y) * GAME_SCREEN_W + x_offs + MAP_W - x,
								0,
								x % MAP_W);
						}
				}
				g_sys.update_screen(g_res.vga, 1);
				g_sys.render_clear_sprites();
				wait_input(1);
				if (g_sys.input.quit || g_sys.input.space) {
					break;
				}
			}
			wait_input(1000);
			g_sys.update_screen(g_res.vga, 0);
			g_sys.fade_out_palette();
			free(g_res.map);
		}
		free(data);
	}
}

static void do_menu() {
	uint8_t *data = load_file("MENU.SQZ");
	if (data) {
		g_sys.set_screen_palette(data, 0, 256, 6);
		update_screen_img(data + 768, 0);
		g_sys.fade_in_palette();
		free(data);
		memset(g_vars.input.keystate, 0, sizeof(g_vars.input.keystate));
		while (!g_sys.input.quit) {
			update_input();
			if (g_vars.input.keystate[2] || g_vars.input.keystate[0x4F] || g_sys.input.space) {
				g_sys.input.space = 0;
				g_sys.fade_out_palette();
				break;
			}
			if (g_vars.input.keystate[3] || g_vars.input.keystate[0x50]) {
				g_sys.fade_out_palette();
				break;
			}
			g_sys.sleep(30);
		}
	}
}

static void do_photos_screen() {
}

void input_check_ctrl_alt_e() {
	if (g_vars.input.keystate[0x1D] && g_vars.input.keystate[0x38] && g_vars.input.keystate[0x12]) {
		do_photos_screen();
	}
}

void input_check_ctrl_alt_w() {
	if (g_vars.input.keystate[0x1D] && g_vars.input.keystate[0x38] && g_vars.input.keystate[0x11]) {
		do_credits();
		wait_input(600);
	}
}

void do_theend_screen() {
	uint8_t *data = load_file("THEEND.SQZ");
	if (data) {
		g_sys.set_screen_palette(data, 0, 256, 6);
		update_screen_img(data + 768, 0);
		g_sys.fade_in_palette();
		free(data);
		wait_input(10000);
	}
	time_t now;
	time(&now);
	struct tm *t = localtime(&now);
	if (t->tm_year + 1900 < 1994) {
		return;
	}
	do_photos_screen();
}

uint32_t timer_get_counter() {
	const uint32_t current = g_sys.get_timestamp();
	return ((current - g_vars.starttime) * 1193182 / 0x4000) / 1000;
}

void random_reset() {
	g_vars.random.a = 5;
	g_vars.random.b = 34;
	g_vars.random.c = 134;
	g_vars.random.d = 58765;
}

uint8_t random_get_number() {
	g_vars.random.d += g_vars.random.a;
	g_vars.random.a += 3 + (g_vars.random.d >> 8);

	g_vars.random.b += g_vars.random.c;
	g_vars.random.b *= 2;
	g_vars.random.b += g_vars.random.a;

	g_vars.random.c ^= g_vars.random.a;
	g_vars.random.c ^= g_vars.random.b;

	return g_vars.random.b;
}

static uint16_t ror16(uint16_t x, int c) {
	return (x >> c) | (x << (16 - c));
}

uint16_t random_get_number2() {
	const uint16_t x = g_vars.random.e + 0x9248;
	g_vars.random.e = ror16(x, 3);
	return g_vars.random.e;
}

static uint16_t rol16(uint16_t x, int c) {
	return (x << c) | (x >> (16 - c));
}

/* original code returns a machine specific value, based on BIOS and CPU */
uint16_t random_get_number3(uint16_t x) {
	x ^= 0x55a3;
	x *= 0xb297; /* to match dosbox */
	return rol16(x, 3);
}

static void game_run(const char *data_path) {
	res_init(data_path, GAME_SCREEN_W * GAME_SCREEN_H);
	sound_init();
	video_convert_tiles(g_res.uniondat, g_res.unionlen);
	g_vars.level_num = g_options.start_level;
	do_programmed_in_1992_screen();
	if (!g_sys.input.space && !g_sys.input.quit) {
		do_titus_screen();
		play_music(3);
		do_present_screen();
	}
	g_sys.render_set_sprites_clipping_rect(0, 0, TILEMAP_SCREEN_W, TILEMAP_SCREEN_H);
	g_vars.random.e = 0x1234;
	g_vars.starttime = g_sys.get_timestamp();
	while (!g_sys.input.quit) {
		if (1) {
			g_vars.player_lifes = 2;
			g_vars.player_bonus_letters_mask = 0;
			g_vars.player_club_power = 20;
			g_vars.player_club_type = 0;
			if (g_res.dos_demo) {
				do_demo_screen();
			}
			do_menu();
			if (g_sys.input.quit) {
				break;
			}
		}
		uint8_t level_num;
		do {
			level_num = g_vars.level_num;
			if (g_vars.level_num >= 8 && g_vars.level_num < 10 && 0 /* !g_vars.level_expert_flag */ ) {
				do_castle_screen();
				break;
			}
			if (g_vars.level_num < 10 && g_options.show_map)
				do_map();
			do_level();
			print_debug(DBG_GAME, "previous level %d current %d", level_num, g_vars.level_num);
		} while (!g_res.dos_demo && g_vars.level_num != level_num);
	}
	sound_fini();
	res_fini();
}

struct game_t game = {
	"Prehistorik 2",
	game_run
};


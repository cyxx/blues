
#include <time.h>
#include "game.h"
#include "resource.h"
#include "sys.h"

static const bool _logos = false;

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
	const uint32_t end = g_sys.get_timestamp() + timeout * 10;
	while (g_sys.get_timestamp() < end) {
		g_sys.process_events();
		if (g_sys.input.quit || g_sys.input.space) {
			break;
		}
		g_sys.sleep(20);
	}
}

static void do_programmed_in_1992_screen() {
	time_t now;
	time(&now);
	struct tm *t = localtime(&now);
	if (t->tm_year + 1900 < 1996) {
		return;
	}
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
	wait_input(100);
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

static void do_titus_screen() {
	uint8_t *data = load_file("TITUS.SQZ");
	if (data) {
		g_sys.set_screen_palette(data, 0, 256, 6);
		g_sys.update_screen(data + 768, 0);
		fade_in_palette();
		wait_input(70);
		fade_out_palette();
		free(data);
	}
}

static void do_present_screen() {
	uint8_t *data = load_file("PRESENT.SQZ");
	if (data) {
		g_sys.set_screen_palette(data, 0, 256, 6);
		g_sys.update_screen(data + 768, 0);
		fade_in_palette();
		free(data);
	}
}

static void do_demo_screen() {
	uint8_t *data = load_file("JOYSTICK.SQZ");
	if (data) {
		video_copy_img(data);
		free(data);
	}
}

static void do_menu() {
	uint8_t *data = load_file("MENU.SQZ");
	if (data) {
		g_sys.set_screen_palette(data, 0, 256, 6);
		g_sys.update_screen(data + 768, 0);
		fade_in_palette();
		free(data);
		memset(g_vars.input.keystate, 0, sizeof(g_vars.input.keystate));
		while (!g_sys.input.quit) {
			update_input();
			if (g_vars.input.keystate[2] || g_vars.input.keystate[0x4F] || g_sys.input.space) {
				g_sys.input.space = 0;
				fade_out_palette();
				break;
			}
			if (g_vars.input.keystate[3] || g_vars.input.keystate[0x50]) {
				fade_out_palette();
				break;
			}
			if (g_vars.input.keystate[4] || g_vars.input.keystate[0x51]) {
				break;
			}
			g_sys.sleep(30);
		}
	}
}

void input_check_ctrl_alt_w() {
	if (g_vars.input.keystate[0x1D] && g_vars.input.keystate[0x38] && g_vars.input.keystate[0x11]) {
		do_credits();
		wait_input(60);
	}
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
	if (_logos) {
		do_programmed_in_1992_screen();
		do_titus_screen();
		play_music(3);
		do_present_screen();
	}
	g_sys.render_set_sprites_clipping_rect(0, 0, TILEMAP_SCREEN_W, TILEMAP_SCREEN_H);
	g_vars.random.e = 0x1234;
	g_vars.starttime = g_sys.get_timestamp();
	while (!g_sys.input.quit) {
		if (1) {
			video_set_palette();
			g_vars.player_lifes = 2;
			g_vars.player_bonus_letters_mask = 0;
			g_vars.player_club_power = 20;
			g_vars.player_club_type = 0;
			if (g_res.dos_demo) {
				do_demo_screen();
			}
			do_menu();
		}
		video_set_palette();
		video_set_palette();
		do_level();
	}
	sound_fini();
	res_fini();
}

struct game_t game = {
	"Prehistorik 2",
	game_run
};


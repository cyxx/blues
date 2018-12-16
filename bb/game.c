
#include "game.h"
#include "resource.h"
#include "sys.h"

// palette colors 0..15
static const uint16_t _colors_180_data[] = {
	0x180,0x000,0x182,0xfff,0x184,0xf86,0x186,0x000,0x188,0xf70,0x18a,0xc50,0x18c,0xa20,0x18e,0x620,
	0x190,0xeb8,0x192,0xb64,0x194,0x0ef,0x196,0x0bc,0x198,0x078,0x19a,0x056,0x19c,0x045,0x19e,0x034
};

// palette colors 16..31
static const uint16_t _colors_1a0_data[] = {
	0x1a0,0x000,0x1a2,0xfdd,0x1a4,0x678,0x1a6,0x046,0x1a8,0x000,0x1aa,0xa20,0x1ac,0xf87,0x1ae,0x955,
	0x1b0,0xbcf,0x1b2,0xfca,0x1b4,0x30a,0x1b6,0xa9a,0x1b8,0x900,0x1ba,0x666,0x1bc,0x747,0x1be,0x020
};

static const uint8_t _colors_cga[] = {
	0x00, 0x00, 0x00, 0x55, 0xFF, 0xFF, 0xFF, 0x55, 0xFF, 0xFF, 0xFF, 0xFF
};

struct vars_t g_vars;

void update_input() {
	g_sys.process_events();
	g_vars.inp_key_left = ((g_sys.input.direction & INPUT_DIRECTION_LEFT) != 0) || g_vars.inp_keyboard[0x4B] || g_vars.inp_keyboard[0x7A];
	g_vars.inp_key_right = ((g_sys.input.direction & INPUT_DIRECTION_RIGHT) != 0) || g_vars.inp_keyboard[0x4D] || g_vars.inp_keyboard[0x79];
	g_vars.inp_key_up = ((g_sys.input.direction & INPUT_DIRECTION_UP) != 0) || g_vars.inp_keyboard[0x48] || g_vars.inp_keyboard[0x7C];
	g_vars.inp_key_down = ((g_sys.input.direction & INPUT_DIRECTION_DOWN) != 0) || g_vars.inp_keyboard[0x50] || g_vars.inp_keyboard[0x7B];
	g_vars.inp_key_space = g_sys.input.space || g_vars.inp_keyboard[0x39] || g_vars.inp_keyboard[0x77];
	// g_vars.inp_key_tab = g_vars.inp_keyboard[0xF] || g_vars.inp_keyboard[0x78];
}

static void do_title_screen() {
	const uint32_t timestamp = g_sys.get_timestamp() + 20 * 1000;
	load_img(g_res.amiga_data ? "blues.lbm" : "pres.sqz", GAME_SCREEN_W, g_options.cga_colors ? 0 : -1);
	fade_in_palette();
	do {
		update_input();
		if (g_sys.input.space || g_sys.input.quit) {
			break;
		}
		g_sys.sleep(10);
	} while (g_sys.get_timestamp() < timestamp);
	play_sound(SOUND_0);
	fade_out_palette();
	g_sys.input.space = 0;
}

static void do_demo_screens() {
	static const char *filenames[] = { "text1.sqz", "maq1.sqz", "maq2.sqz", "maq3.sqz", "maq4.sqz", "maq5.sqz", 0 };
	for (int i = 0; filenames[i]; ++i) {
		load_img(filenames[i], GAME_SCREEN_W, -1);
		fade_in_palette();
		while (1) {
			update_input();
			if (g_sys.input.space) {
				break;
			}
			if (g_sys.input.quit) {
				return;
			}
			g_sys.sleep(10);
		}
		fade_out_palette();
	}
}

static void do_select_player() {
	int quit = 0;
	int fade = 0;
	int state = 0;
	int frame1 = 1;
	int frame2 = 1;
	const int color_rgb = 2;
	const int colors_count = 25;
	load_img(g_res.amiga_data ? "choix.lbm" : "choix.sqz", GAME_SCREEN_W, g_options.cga_colors ? 1 : -1);
	screen_load_graphics(g_options.cga_colors ? g_res.cga_lut_sqv : 0, 0);
	screen_clear_sprites();
	do {
		update_input();
		const uint32_t timestamp = g_sys.get_timestamp();
		switch (state) {
		case 0:
			screen_add_sprite(95, 155, animframe_0135[frame1]);
			if (frame1 < animframe_0135[0]) {
				++frame1;
			} else {
				frame1 = 1;
			}
			screen_add_sprite(190, 155, animframe_01d5[frame2]);
			if (frame2 < animframe_01d5[0]) {
				++frame2;
			} else {
				frame2 = 1;
			}
			g_vars.two_players_flag = 0;
			level_data[g_vars.level * MAX_OBJECTS + OBJECT_NUM_PLAYER1] = PLAYER_JAKE;
			level_data[g_vars.level * MAX_OBJECTS + OBJECT_NUM_PLAYER2] = 100;
			g_vars.player = PLAYER_JAKE;
			if (g_sys.input.direction & INPUT_DIRECTION_RIGHT) {
				g_sys.input.direction &= ~INPUT_DIRECTION_RIGHT;
				for (int i = 0; i < colors_count; ++i) {
					screen_adjust_palette_color( 2, color_rgb,  1);
					screen_adjust_palette_color( 9, color_rgb,  1);
					screen_adjust_palette_color( 3, color_rgb, -1);
					screen_adjust_palette_color(10, color_rgb, -1);
				}
				screen_vsync();
				state = 1;
				frame1 = frame2 = 1;
			} else if (g_sys.input.direction & INPUT_DIRECTION_LEFT) {
				g_sys.input.direction &= ~INPUT_DIRECTION_LEFT;
				for (int i = 0; i < colors_count; ++i) {
					screen_adjust_palette_color(2, color_rgb, 1);
					screen_adjust_palette_color(9, color_rgb, 1);
				}
				screen_vsync();
				state = 2;
				frame1 = frame2 = 1;
			}
			break;
		case 1:
			screen_add_sprite(95, 155, animframe_00dd[frame1]);
			if (frame1 < animframe_00dd[0]) {
				++frame1;
			} else {
				frame1 = 4;
			}
			screen_add_sprite(190, 155, animframe_022d[frame2]);
			if (frame2 < animframe_022d[0]) {
				++frame2;
			} else {
				frame2 = 1;
			}
			g_vars.two_players_flag = 0;
			level_data[g_vars.level * MAX_OBJECTS + OBJECT_NUM_PLAYER1] = PLAYER_ELWOOD;
			level_data[g_vars.level * MAX_OBJECTS + OBJECT_NUM_PLAYER2] = 100;
			g_vars.player = PLAYER_ELWOOD;
			if (g_sys.input.direction & INPUT_DIRECTION_RIGHT) {
				g_sys.input.direction &= ~INPUT_DIRECTION_RIGHT;
				for (int i = 0; i < colors_count; ++i) {
					screen_adjust_palette_color( 3, color_rgb, 1);
					screen_adjust_palette_color(10, color_rgb, 1);
				}
				screen_vsync();
				state = 2;
				frame1 = frame2 = 1;
			} else if (g_sys.input.direction & INPUT_DIRECTION_LEFT) {
				g_sys.input.direction &= ~INPUT_DIRECTION_LEFT;
				for (int i = 0; i < colors_count; ++i) {
					screen_adjust_palette_color( 3, color_rgb,  1);
					screen_adjust_palette_color(10, color_rgb,  1);
					screen_adjust_palette_color( 2, color_rgb, -1);
					screen_adjust_palette_color( 9, color_rgb, -1);
				}
				screen_vsync();
				state = 0;
				frame1 = frame2 = 1;
			}
			break;
		case 2:
			screen_add_sprite(95, 155, animframe_0135[frame1]);
			if (frame1 < animframe_0135[0]) {
				++frame1;
			} else {
				frame1 = 1;
			}
			screen_add_sprite(190, 155, animframe_022d[frame2]);
			if (frame2 < animframe_022d[0]) {
				++frame2;
			} else {
				frame2 = 1;
			}
			g_vars.two_players_flag = 1;
			if (g_sys.input.direction & INPUT_DIRECTION_RIGHT) {
				g_sys.input.direction &= ~INPUT_DIRECTION_RIGHT;
				for (int i = 0; i < colors_count; ++i) {
					screen_adjust_palette_color(2, color_rgb, -1);
					screen_adjust_palette_color(9, color_rgb, -1);
				}
				screen_vsync();
				state = 0;
				frame1 = frame2 = 1;
			} else if (g_sys.input.direction & INPUT_DIRECTION_LEFT) {
				g_sys.input.direction &= ~INPUT_DIRECTION_LEFT;
				for (int i = 0; i < colors_count; ++i) {
					screen_adjust_palette_color( 3, color_rgb, -1);
					screen_adjust_palette_color(10, color_rgb, -1);
				}
				screen_vsync();
				state = 1;
				frame1 = frame2 = 1;
			}
			break;
		}
		if (!fade) {
			fade_in_palette();
			fade = 1;
			for (int i = 0; i < colors_count; ++i) {
				screen_adjust_palette_color( 3, color_rgb, 1);
				screen_adjust_palette_color(10, color_rgb, 1);
			}
			continue;
		}
		screen_redraw_sprites();
		screen_flip();
		screen_vsync();
		const int diff = (timestamp + (1000 / 30)) - g_sys.get_timestamp();
		g_sys.sleep(diff < 10 ? 10 : diff);
		screen_clear_sprites();
		if (g_sys.input.space || g_vars.play_demo_flag) {
			quit = 1;
		}
	} while (!quit && !g_sys.input.quit);
}

static void do_inter_screen_helper(int xpos, int ypos, int c) {
	for (int i = 0; i < 40; ++i) {
		screen_add_sprite(xpos + 20 - 1 - i, ypos - 20 + 1 + i, 125);
		screen_redraw_sprites();
		if (c != 0) {
			screen_vsync();
		}
		screen_clear_sprites();
	}
	for (int i = 0; i < 40; ++i) {
		screen_add_sprite(xpos - 20 + 1 + i, ypos - 20 + 1 + i, 125);
		screen_redraw_sprites();
		if (c != 0) {
			screen_vsync();
		}
		screen_clear_sprites();
	}
}

static void do_inter_screen() {
	static const uint8_t xpos[] = { 0xFA, 0x50, 0xF0, 0xC8, 0x50, 0x50 };
	static const uint8_t ypos[] = { 0xAA, 0x37, 0x28, 0x5F, 0xA5, 0xAA };
	load_img(g_res.amiga_data ? "inter.lbm" : "inter.sqz", GAME_SCREEN_W, g_options.cga_colors ? 9 : -1);
	screen_clear_sprites();
	if (g_vars.level > 1) {
		for (int i = 0; i < g_vars.level - 1; ++i) {
			do_inter_screen_helper(xpos[i], ypos[i], 0);
		}
	}
	if (g_vars.level == MAX_LEVELS - 1) {
		do_inter_screen_helper(xpos[g_vars.level], ypos[g_vars.level], 0);
	}
	fade_in_palette();
	if (g_vars.level > 0 && g_vars.level < MAX_LEVELS - 1) {
		do_inter_screen_helper(xpos[g_vars.level - 1], ypos[g_vars.level - 1], 1);
	}
	// screen_do_transition2();
	screen_flip();
	if (g_vars.level < MAX_LEVELS - 1) {
		play_sound(SOUND_2);
		screen_add_sprite(xpos[g_vars.level], ypos[g_vars.level], 126);
		screen_redraw_sprites();
	}
	screen_flip();
	const uint32_t timestamp = g_sys.get_timestamp() + 4 * 1000;
	do {
		update_input();
		if (g_sys.input.space || g_sys.input.quit) {
			break;
		}
	} while (g_sys.get_timestamp() < timestamp);
	fade_out_palette();
}

void game_main() {
	play_music(0);
	screen_init();
	screen_flip();
	g_vars.start_level = 0;
	if (g_res.amiga_data) {
		load_spr("sprite", g_res.spr_sqv, 0);
		load_spr("objet", g_res.spr_sqv + SPRITE_SIZE, 101);
	} else {
		load_sqv("sprite.sqv", g_res.spr_sqv, 0, g_options.cga_colors ? 2 : -1);
	}
	if (g_options.amiga_status_bar || g_res.amiga_data) {
		uint16_t palette[16];
		for (int i = 0; i < 16; ++i) {
			assert(_colors_180_data[i * 2] == 0x180 + i * 2);
			palette[i] = _colors_180_data[i * 2 + 1];
		}
		g_sys.set_palette_amiga(palette, 32);
		g_res.spr_count = 120;
		extern const uint8_t icon6e92[]; // top or bottom status bar
		g_res.spr_frames[123] = icon6e92;
		g_res.spr_frames[124] = icon6e92;
		extern const uint8_t icon72de[]; // jake
		g_res.spr_frames[115] = icon72de;
		extern const uint8_t icon73a6[]; // elwood
		g_res.spr_frames[117] = icon73a6;
		extern const uint8_t icon6ef6[]; // heart
		g_res.spr_frames[120] = icon6ef6;
		extern const uint8_t icon740a[]; // instrument level 1
		g_res.spr_frames[135] = icon740a;
		extern const uint8_t icon746e[]; // instrument level 2
		g_res.spr_frames[136] = icon746e;
		extern const uint8_t icon74d2[]; // instrument level 3
		g_res.spr_frames[137] = icon74d2;
		extern const uint8_t icon7536[]; // instrument level 4
		g_res.spr_frames[138] = icon7536;
		extern const uint8_t icon759a[]; // instrument level 5
		g_res.spr_frames[139] = icon759a;
		extern const uint8_t icon75fe[]; // instrument level 1 (found)
		g_res.spr_frames[140] = icon75fe;
		extern const uint8_t icon7662[]; // instrument level 2 (found)
		g_res.spr_frames[141] = icon7662;
		extern const uint8_t icon76c6[]; // instrument level 3 (found)
		g_res.spr_frames[142] = icon76c6;
		extern const uint8_t icon772a[]; // instrument level 4 (found)
		g_res.spr_frames[143] = icon772a;
		extern const uint8_t icon778e[]; // instrument level 5 (found)
		g_res.spr_frames[144] = icon778e;
	}
	if (g_options.cga_colors) {
		g_sys.set_screen_palette(_colors_cga, 0, 4, 8);
	} else if (g_options.amiga_colors) {
		uint16_t palette[16];
		for (int i = 0; i < 16; ++i) {
			assert(_colors_1a0_data[i * 2] == 0x1a0 + i * 2);
			palette[i] = _colors_1a0_data[i * 2 + 1];
		}
		g_sys.set_palette_amiga(palette, 16);
	}
	do_title_screen();
	while (!g_sys.input.quit) {
		if (!g_vars.level_completed_flag) {
			g_vars.game_over_flag = 0;
			g_vars.play_level_flag = 1;
			if (!g_vars.play_demo_flag) {
				g_vars.level = g_options.start_level;
				do_select_player();
			} else {
				g_vars.level = g_vars.start_level;
			}
			screen_unk5();
		}
		while (!g_sys.input.quit && g_vars.play_level_flag) {
			if (!g_vars.game_over_flag) {
				do_inter_screen();
			}
			load_level_data(g_vars.level);
			do_level();
			if (g_res.dos_demo && g_vars.level > 0) {
				do_demo_screens();
				return;
			}
		}
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
	"Blues Brothers",
	game_run
};


#include "game.h"
#include "resource.h"
#include "sys.h"

struct vars_t g_vars;

void update_input() {
	g_sys.process_events();
	g_vars.inp_key_left = ((g_sys.input.direction & INPUT_DIRECTION_LEFT) != 0) || g_vars.inp_keyboard[0x4B] || g_vars.inp_keyboard[0x7A];
	g_vars.inp_key_right = ((g_sys.input.direction & INPUT_DIRECTION_RIGHT) != 0) || g_vars.inp_keyboard[0x4D] || g_vars.inp_keyboard[0x79];
	g_vars.inp_key_up = ((g_sys.input.direction & INPUT_DIRECTION_UP) != 0) || g_vars.inp_keyboard[0x48] || g_vars.inp_keyboard[0x7C];
	g_vars.inp_key_down = ((g_sys.input.direction & INPUT_DIRECTION_DOWN) != 0) || g_vars.inp_keyboard[0x50] || g_vars.inp_keyboard[0x7B];
	g_vars.inp_key_space = (g_sys.input.space != 0) || g_vars.inp_keyboard[0x39] || g_vars.inp_keyboard[0x77];
	g_vars.inp_key_tab = g_vars.inp_keyboard[0xF] || g_vars.inp_keyboard[0x78];
}

void do_title_screen() {
	const uint32_t timestamp = g_sys.get_timestamp() + 20 * 1000;
	load_img("pres.sqz");
	fade_in_palette();
	do {
		update_input();
		if (g_sys.input.space || g_sys.input.quit) {
			break;
		}
	} while (g_sys.get_timestamp() < timestamp);
	play_sound(SOUND_0);
	fade_out_palette();
	g_sys.input.space = 0;
	read_file("avtmag.sqv", g_res.avt_sqv);
}

static void check_cheat_code() {
	static const uint8_t codes[] = { 0x14, 0x17, 0x27, 0x18, 0x14, 0x23, 0x12, 0x12 };
	if ((g_vars.inp_code & 0x80) == 0 && g_vars.inp_code != 0) {
		if (g_vars.inp_code == codes[g_vars.cheat_code_len]) {
			++g_vars.cheat_code_len;
			if (g_vars.cheat_code_len == ARRAYSIZE(codes)) {
				g_vars.cheat_flag = 1;
			}
		} else {
			g_vars.cheat_code_len = 0;
		}
	}
}

void do_select_player() {
	int quit = 0;
	int fade = 0;
	int state = 0;
	int frame1 = 1;
	int frame2 = 1;
	const int color_rgb = 2;
	const int colors_count = 25;
	load_img("choix.sqz");
	screen_load_graphics();
	screen_clear_sprites();
	do {
		screen_unk4();
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
// state_default_:
		if (!fade) {
			fade_in_palette();
			fade = 1;
			for (int i = 0; i < colors_count; ++i) {
				screen_adjust_palette_color( 3, color_rgb, 1);
				screen_adjust_palette_color(10, color_rgb, 1);
			}
			continue;
		}
		screen_clear_last_sprite();
		screen_redraw_sprites();
		if (g_vars.cheat_flag) {
			for (int i = 5; i >= 0; --i) {
				if (g_vars.inp_keyboard[2 + i]) {
					g_vars.level = i;
					break;
				}
			}
//			screen_draw_number_type1(_level + 1, 256, 148, 2);
		} else {
			check_cheat_code();
		}
		screen_flip();
		screen_vsync();
		const int diff = (timestamp + (1000 / 30)) - g_sys.get_timestamp();
		g_sys.sleep(diff < 10 ? 10 : diff);
		g_vars.screen_draw_offset ^= 0x2000;
		screen_clear_sprites();
		if (g_sys.input.space || g_vars.play_demo_flag) {
			quit = 1;
		}
	} while (!quit && !g_sys.input.quit);
}

static void do_inter_screen_helper(int xpos, int ypos, int c) {
	if (c != 0) {
		g_vars.screen_draw_offset ^= 0x2000;
	}
	for (int i = 0; i < 40; ++i) {
		screen_add_sprite(xpos + 20 - 1 - i, ypos - 20 + 1 + i, 125);
		screen_clear_last_sprite();
		screen_redraw_sprites();
		if (c != 0) {
			screen_vsync();
		}
		screen_clear_sprites();
	}
	for (int i = 0; i < 40; ++i) {
		screen_add_sprite(xpos - 20 + 1 + i, ypos - 20 + 1 + i, 125);
		screen_clear_last_sprite();
		screen_redraw_sprites();
		if (c != 0) {
			screen_vsync();
		}
		screen_clear_sprites();
	}
	if (c != 0) {
		g_vars.screen_draw_offset ^= 0x2000;
	}
}

static void do_inter_screen() {
	static const uint8_t xpos[] = { 0xFA, 0x50, 0xF0, 0xC8, 0x50, 0x50 };
	static const uint8_t ypos[] = { 0xAA, 0x37, 0x28, 0x5F, 0xA5, 0xAA };
	load_img("inter.sqz");
	g_vars.screen_h = 199;
	screen_clear_sprites();
	if (g_vars.level > 1) {
		for (int i = 0; i < g_vars.level - 1; ++i) {
			do_inter_screen_helper(xpos[i], ypos[i], 0);
		}
	}
	if (g_vars.level == 5) {
		do_inter_screen_helper(xpos[g_vars.level], ypos[g_vars.level], 0);
	}
	fade_in_palette();
	if (g_vars.level > 0 && g_vars.level < 5) {
		do_inter_screen_helper(xpos[g_vars.level - 1], ypos[g_vars.level - 1], 1);
	}
	g_vars.screen_draw_offset = 0x2000;
	screen_do_transition2();
	screen_flip();
	if (g_vars.level < MAX_LEVELS - 1) {
		play_sound(SOUND_2);
		screen_add_sprite(xpos[g_vars.level], ypos[g_vars.level], 126);
		screen_clear_last_sprite();
		screen_redraw_sprites();
	}
	g_vars.screen_draw_offset = 0x2000;
	screen_flip();
	g_vars.screen_h = TILEMAP_SCREEN_H;
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
	g_vars.screen_w = GAME_SCREEN_W;
	g_vars.screen_h = GAME_SCREEN_H;
	g_vars.screen_draw_offset = 0;
	screen_flip();
	screen_init();
	g_sys.set_screen_size(GAME_SCREEN_W, GAME_SCREEN_H, "Blues Brothers");
	g_vars.start_level = 0;
	load_sqv("sprite.sqv", g_res.spr_sqv, 0);
	do_title_screen();
	while (g_sys.input.quit == 0) {
		if (!g_vars.level_completed_flag) {
//			_level_cheat_code = 0;
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
		while (g_sys.input.quit == 0 && g_vars.play_level_flag) {
			if (!g_vars.game_over_flag) {
				do_inter_screen();
			}
			load_level_data(g_vars.level);
			do_level();
		}
	}
}

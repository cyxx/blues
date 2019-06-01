
/* level main loop */

#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

static const char *_background[] = {
	"back0.eat", "back1.eat", "back2.eat", "back3.eat"
};

static const char *_decor[] = {
	"jardin.eat", "prison.eat", "entrepot.eat", "egout.eat"
};

static const uint16_t _vinyl_throw_delay = 5;
static const uint16_t _object_respawn_delay = 224;
static const uint16_t _undefined = 0x55AA;

static void level_player_draw_powerdown_animation(struct player_t *player);
static void level_player_draw_powerup_animation(struct player_t *player, struct object_t *obj);
static void level_update_tiles(struct object_t *obj, int ax, int dx, int bp);

static void do_end_of_level() {
	static const uint8_t color_index = 127;
	ja_decode_motif(g_vars.level_num % 9, color_index);
	static const uint8_t white[] = { 0x3F, 0x3F, 0x3F };
	g_sys.set_palette_color(color_index, white);
	g_sys.update_screen(g_res.vga, 1);
	g_sys.sleep(1000);
	g_sys.fade_out_palette();
}

static void level_player_die(struct player_t *player) {
	g_vars.player_xscroll = 0;
	player_flags2(&player->obj) |= 8;
	level_player_draw_powerdown_animation(player);
	if (player_flags(&player->obj) & 0x40) {
		// player_flags2(&player->other_player->obj) &= ~1;
	}
	--player->lifes_count;
	play_sound(SOUND_14);
}

static void level_reset_palette() {
	if (g_vars.reset_palette_flag) {
		return;
	}
	g_vars.reset_palette_flag = 1;
	memcpy(g_vars.palette_buffer, common_palette_data, 384);
	const uint8_t *pal = levels_palette_data + g_vars.level_num * 144 * 3;
	memcpy(g_vars.palette_buffer + 384, pal, 384); pal += 384;
	if ((g_vars.level & 1) != 0) {
		memcpy(g_vars.palette_buffer + 384 - 48, pal, 48);
	}
	for (int i = 0; i < 8; ++i) {
		uint8_t *dst = g_vars.palette_buffer + 384 + 48 * i;
		dst[0] = 0;
		dst[1] = 0x20;
		dst[2] = 0x3F;
	}
	g_sys.set_screen_palette(g_vars.palette_buffer, 0, 256, 6);
}

static void level_update_palette() {
	if (g_vars.level_time == 12 || g_vars.level_time2 == g_vars.level_time) {
		return;
	}
	if (g_vars.level_time2 < g_vars.level_time) {
		g_vars.level_time2 = g_vars.level_time;
		g_vars.reset_palette_flag = 0;
		level_reset_palette();
		return;
	}
	g_vars.level_time2 = g_vars.level_time;
	if (g_vars.level_time < 5) {
		play_sound(SOUND_15);
		uint8_t *palette_buffer = g_vars.palette_buffer + 128 * 3;
		for (int i = 0; i < 128 * 3; ++i) {
			const int color = palette_buffer[i];
			palette_buffer[i] = MAX(0, color - 8);
		}
		g_sys.set_screen_palette(palette_buffer, 128, 128, 6);
		if (g_vars.level_time == 0) {
			level_player_die(&g_vars.players_table[0]);
			level_player_die(&g_vars.players_table[1]);
			play_sound(SOUND_14);
		}
	}
}

static uint8_t level_lookup_tile(uint8_t num) {
	const uint8_t type = g_vars.tilemap_lut_type[num];
	if (type == 2) {
		if ((g_vars.tilemap_flags & 2) != 0 && (g_vars.level_loop_counter & 0x20) != 0) {
			return 0;
		}
		if ((g_vars.tilemap_flags & 4) == 0) {
			const uint8_t flags = g_vars.tilemap_lut_init2[num] ^ g_vars.tilemap_flags;
			if (flags & 1) {
				return 0;
			}
		}
	}
	num = g_vars.tilemap_current_lut[num];
	if (num == 248 && (g_vars.tilemap_flags & 1) != 0) {
		++num;
	}
	return num;
}

static uint8_t level_get_tile(int offset) {
	if (offset < 0 || offset >= g_vars.tilemap_w * g_vars.tilemap_h) {
		print_warning("Invalid tilemap offset %d", offset);
		return 0;
	}
	int num = g_vars.tilemap_data[offset];
	num = level_lookup_tile(num);
	return g_vars.level_tiles_lut[num];
}

static void level_draw_tile(uint8_t tile_num, int x, int y, int dx, int dy) {
	tile_num = level_lookup_tile(tile_num);
	// video_set_palette_index(g_vars.tile_palette_table[tile_num]);
	ja_decode_tile(g_res.tmp + tile_num * 128, 0x80 | (g_vars.tile_palette_table[tile_num] * 0x10), g_res.vga, GAME_SCREEN_W, x * 16 - dx, y * 16 - dy);
}

static void level_draw_tilemap() {
	video_copy_backbuffer(PANEL_H);
	const uint8_t *current_lut = g_vars.tilemap_current_lut + 0x100;
	if (current_lut >= &g_vars.tilemap_lut_init[0x600]) {
		g_vars.tilemap_current_lut = g_vars.tilemap_lut_init;
	} else {
		g_vars.tilemap_current_lut = current_lut;
	}
	for (int y = 0; y < TILEMAP_SCREEN_H / 16 + 1; ++y) {
		for (int x = 0; x < TILEMAP_SCREEN_W / 16 + 1; ++x) {
			const int offset = (g_vars.tilemap_y + y) * g_vars.tilemap_w + (g_vars.tilemap_x + x);
			const uint8_t num = g_vars.tilemap_data[offset];
			level_draw_tile(num, x, y, g_vars.tilemap_scroll_dx << 2, g_vars.tilemap_scroll_dy);
		}
	}
}

static void level_adjust_hscroll_right(int dx) {
	if (g_vars.tilemap_x >= g_vars.tilemap_end_x) {
		const int8_t _al = 3 - g_vars.tilemap_scroll_dx;
		if (_al < dx) {
			dx = _al;
		}
	}
	g_vars.tilemap_scroll_dx += dx;
	if (g_vars.tilemap_scroll_dx < 4) {
		return;
	}
	g_vars.tilemap_scroll_dx -= 4;
	++g_vars.tilemap_x;
}

static void level_adjust_hscroll_left(int dx) {
	g_vars.tilemap_scroll_dx -= dx;
	if (g_vars.tilemap_scroll_dx >= 0) {
		return;
	}
	g_vars.tilemap_scroll_dx += 4;
	--g_vars.tilemap_x;
	if (g_vars.tilemap_x < 0) {
		g_vars.tilemap_x = 0;
		g_vars.tilemap_scroll_dx = 0;
	}
}

static void level_adjust_vscroll_down(int dy) {
	if (g_vars.tilemap_y >= g_vars.tilemap_end_y) {
		const int8_t _al = 15 - g_vars.tilemap_scroll_dy;
		if (_al < dy) {
			dy = _al;
		}
	}
	g_vars.tilemap_scroll_dy += dy;
	if (g_vars.tilemap_scroll_dy < 16) {
		return;
	}
	g_vars.tilemap_scroll_dy -= 16;
	++g_vars.tilemap_y;
}

static void level_adjust_vscroll_up(int dy) {
	g_vars.tilemap_scroll_dy -= dy;
	if (g_vars.tilemap_scroll_dy >= 0) {
		return;
	}
	g_vars.tilemap_scroll_dy += 16;
	--g_vars.tilemap_y;
	if (g_vars.tilemap_y < 0) {
		g_vars.tilemap_scroll_dy = 0;
		g_vars.tilemap_y = 0;
	}
}

static void level_hit_player(struct player_t *p) {
	player_flags(&g_vars.players_table[0].obj) &= ~0x40;
	player_flags(&g_vars.players_table[1].obj) &= ~0x40;
	player_flags2(&g_vars.players_table[0].obj) &= ~1;
	player_flags2(&g_vars.players_table[1].obj) &= ~1;
}

static void level_player_hit_object(struct player_t *p, struct object_t *obj) {
	if (g_options.cheats & CHEATS_OBJECT_NO_HIT) {
		return;
	}
	if (object_blinking_counter(obj) != 0 || g_vars.level_loop_counter <= 20) {
		return;
	}
	play_sound(SOUND_4);
	if ((player_flags(&p->obj) & PLAYER_FLAGS_POWERUP) == 0 && (--p->energy < 0)) {
		p->energy = 0;
		level_player_die(p);
	}
	player_hit_counter(&p->obj) = 8;
	player_y_delta(&p->obj) = -40;
	if (player_flags(&p->obj) & PLAYER_FLAGS_POWERUP) {
		level_player_draw_powerdown_animation(p);
	}
	level_hit_player(p);
	object_blinking_counter(obj) = 33;
}

static void level_player_update_xvelocity(struct player_t *player, int16_t x) {
	if ((player_flags2(&player->obj) & 2) == 0) {
		x *= 8;
		if (x == 0) {
			if (player_x_delta(&player->obj) != 0) {
				player_x_delta(&player->obj) += (player_x_delta(&player->obj) >= 0) ? -8 : 8;
				const int dx = abs(player_x_delta(&player->obj));
				if (dx < 8) {
					player_x_delta(&player->obj) = 0;
				}
			}
		} else if (x < 0) {
			if (player_x_delta(&player->obj) + x < -32) {
				player_x_delta(&player->obj) = -32;
			} else {
				player_x_delta(&player->obj) += x;
			}
		} else {
			if (player_x_delta(&player->obj) + x > 32) {
				player_x_delta(&player->obj) = 32;
			} else {
				player_x_delta(&player->obj) += x;
			}
		}
	}
}

static struct object_t *find_object(int offset, int count, int stride) {
	for (int i = 0; i < count; ++i) {
		struct object_t *obj = &g_vars.objects_table[offset + i * stride];
		if (obj->spr_num == 0xFFFF) {
			return obj;
		}
	}
	print_warning("No free object %d count %d", offset, count / stride);
	return 0;
}

// small stars
static void level_init_object_spr_num_46(int x_pos, int y_pos) {
	struct object_t *obj = find_object(2, 20, 1);
	if (obj) {
		obj->x_pos = x_pos;
		obj->y_pos = y_pos;
		obj->spr_num = 46;
		object2_spr_count(obj) = 4;
		object2_spr_tick(obj) = 1;
		object_blinking_counter(obj) = 0;
	}
}

// large star
static void level_init_object_spr_num_92(int x_pos, int y_pos) {
	struct object_t *obj = find_object(2, 20, 1);
	if (obj) {
		obj->x_pos = x_pos;
		obj->y_pos = y_pos;
		obj->spr_num = 92;
		object2_spr_count(obj) = 4;
		object2_spr_tick(obj) = 1;
		object_blinking_counter(obj) = 0;
	}
}

// light
static void level_init_object_spr_num_98(int x_pos, int y_pos) {
	struct object_t *obj = find_object(2, 20, 1);
	if (obj) {
		obj->x_pos = x_pos;
		obj->y_pos = y_pos;
		obj->spr_num = 98;
		object2_spr_count(obj) = 4;
		object2_spr_tick(obj) = 1;
		object_blinking_counter(obj) = 0;
	}
}

// light
static void level_init_object_spr_num_98_align_pos(int x_pos, int y_pos) {
	struct object_t *obj = find_object(2, 20, 1);
	if (obj) {
		obj->x_pos = (x_pos << 4) + 8;
		obj->y_pos = (y_pos << 4) + 12;
		obj->spr_num = 98;
		object2_spr_count(obj) = 6;
		object2_spr_tick(obj) = 1;
		object_blinking_counter(obj) = 0;
	}
}

// vinyl fade animation
static void level_init_object_spr_num_104(int x_pos, int y_pos) {
	struct object_t *obj = find_object(2, 20, 1);
	if (obj) {
		obj->x_pos = (x_pos << 4) + 8;
		obj->y_pos = (y_pos << 4) + 8;
		obj->spr_num = 104;
		object2_spr_count(obj) = 6;
		object2_spr_tick(obj) = 3;
		object_blinking_counter(obj) = 0;
	}
}

static void level_update_bouncing_tiles(struct player_t *player) {
	const int offset = player_tilemap_offset(&player->obj);
	uint8_t *p = &g_vars.tilemap_data[offset];
	int num = *p;
	int adj = 2;
	if (num > 0x20 && (num <= 0x22 || num > 0x9C)) {
		adj = -adj;
	}
	if ((num & 1) == 0) {
		--p;
	}
	p[0] += adj;
	p[1] += adj;
	p[g_vars.tilemap_w] += adj;
	p[g_vars.tilemap_w + 1] += adj;
}

static int level_player_update_flags(struct player_t *player) {
	if (player_flags2(&player->obj) & 0x40) {
		return 1;
	}
	if (abs(player_x_delta(&player->obj)) < 24) {
		return 0;
	}
	player_flags(&player->obj) &= ~PLAYER_FLAGS_FACING_LEFT;
	if (player_x_delta(&player->obj) < 0) {
		player_flags(&player->obj) |= PLAYER_FLAGS_FACING_LEFT;
	}
	if (g_vars.level_loop_counter - player->ticks_counter < 4) {
		return 1;
	}
	player->ticks_counter = g_vars.level_loop_counter;
	if (player_flags2(&player->obj) & 2) {
		return 1;
	}
	level_init_object_spr_num_98(player->obj.x_pos, player->obj.y_pos);
	play_sound(SOUND_11);
	return 1;
}

static void level_player_update_hdirection(struct player_t *player) {
	if (g_vars.input_key_right) {
		level_player_update_xvelocity(player, 2);
	} else if (g_vars.input_key_left) {
		level_player_update_xvelocity(player, -2);
	} else {
		level_player_update_xvelocity(player, 0);
	}
}

static void level_update_player_anim_1(struct player_t *player) {
	player->obj.spr_num = 1;
	level_player_update_xvelocity(player, 0);
	if (player_throw_counter(&player->obj) != 0) {
		player->obj.spr_num = 0;
	}
}

static void level_update_player_anim_5(struct player_t *player) {
	level_player_update_hdirection(player);
	player->obj.spr_num = 5;
	if (player_jump_counter(&player->obj) != 0 && (player_flags2(&player->obj) & 1) == 0 && g_vars.input_key_up) {
		--player_jump_counter(&player->obj);
		const int dy = (int8_t)player_anim_dy[player_jump_counter(&player->obj)] + player_y_delta(&player->obj);
		if (dy >= -70) {
			player_y_delta(&player->obj) = dy;
		}
	}
}

static void level_update_player_anim_2(struct player_t *player) {
	if (player_jump_counter(&player->obj) != 7) {
		level_update_player_anim_5(player);
	} else {
		player->obj.spr_num = 2;
		level_player_update_hdirection(player);
	}
}

static void level_update_player_anim_3(struct player_t *player) {
	level_player_update_hdirection(player);
	player->obj.spr_num = 3;
}

static void level_update_player_anim_4(struct player_t *player) {
	player->obj.spr_num = 4;
	level_player_update_xvelocity(player, 0);
}

static void level_update_player_anim_34(struct player_t *player) {
	player->obj.spr_num = 0;
	if (level_player_update_flags(player)) {
		player->obj.spr_num = 34;
	}
	level_player_update_xvelocity(player, 0);
}

static void level_update_player_anim_0(struct player_t *player) {
	if (player_flags2(&player->obj) & 1) {
		level_update_player_anim_34(player);
	} else if (player_jump_counter(&player->obj) != 7) {
		level_update_player_anim_5(player);
	} else if (player->obj.spr_num == 1) {
		level_update_player_anim_1(player);
	} else if (player_idle_counter(&player->obj) != 90) {
		level_update_player_anim_34(player);
	} else {
		player->obj.spr_num = 1;
		level_player_update_xvelocity(player, 0);
	}
}

// throw vinyl
static void level_update_player_anim_6(struct player_t *player) {
	if (player->vinyls_count == 0) {
		play_sound(SOUND_8);
		level_update_player_anim_0(player);
	} else {
		level_player_update_xvelocity(player, 0);
		player->obj.spr_num = 6;
		if (player_throw_counter(&player->obj) == 0) {
			return;
		}
		if (g_vars.level_loop_counter - g_vars.throw_vinyl_counter <= _vinyl_throw_delay) {
			return;
		}
		g_vars.throw_vinyl_counter = g_vars.level_loop_counter;
		struct object_t *obj = find_object(22, 10, 1);
		if (obj) {
			obj->spr_num = 42; // vinyl
			object_blinking_counter(obj) = 0;
			object22_player_num(obj) = player - &g_vars.players_table[0];
			obj->x_pos = player->obj.x_pos;
			obj->y_pos = player->obj.y_pos - 12;
			object22_xvelocity(obj) = (player_flags(&player->obj) & PLAYER_FLAGS_FACING_LEFT) ? -24 : 24;
			int ax = ((player_flags(&player->obj) & PLAYER_FLAGS_POWERUP) != 0) ? 0x200 : 0x100;
			if (player_power(&player->obj) >= 33) {
				ax = 0x4000;
				if ((g_options.cheats & CHEATS_UNLIMITED_VINYLS) == 0) {
					player->vinyls_count -= 4;
				}
			}
			if ((g_options.cheats & CHEATS_UNLIMITED_VINYLS) == 0) {
				--player->vinyls_count;
			}
			object22_damage(obj) = ax;
			player_flags(&player->obj) |= PLAYER_FLAGS_THROW_VINYL;
			player_power(&player->obj) = 0;
			play_sound(SOUND_9);
		}
	}
}

static void level_update_player_anim_7(struct player_t *player) {
	if (player->vinyls_count == 0) {
		play_sound(SOUND_8);
		level_update_player_anim_0(player);
	} else {
		player->obj.y_pos += 12;
		level_update_player_anim_6(player);
		player->obj.y_pos -= 12;
	}
	player->obj.spr_num = 7;
}

static void level_update_player_anim_8(struct player_t *player) {
	player->obj.spr_num = 8;
	player->obj.y_pos -= 2;
	player_jump_counter(&player->obj) = 7;
	player_x_delta(&player->obj) = 0;
	player_y_delta(&player->obj) = 0;
}

static void level_update_player_anim_9(struct player_t *player) {
	player->obj.spr_num = 9;
	player_jump_counter(&player->obj) = 7;
	player_x_delta(&player->obj) = 0;
	player_y_delta(&player->obj) = 0;
}

static void level_update_player_anim_10(struct player_t *player, int spr_num) {
	player->obj.spr_num = spr_num;
	player->obj.y_pos += 2;
	const int x = (player->obj.x_pos & 15) - 8;
	if (x < 0) {
		++player->obj.x_pos;
	} else if (x > 0) {
		--player->obj.x_pos;
	}
	player_jump_counter(&player->obj) = 7;
	player_x_delta(&player->obj) = 0;
	player_y_delta(&player->obj) = 0;
}

static void level_update_player_anim_11(struct player_t *player) {
	player->obj.spr_num = 9;
	--player->obj.x_pos;
	player_jump_counter(&player->obj) = 7;
	player_x_delta(&player->obj) = 0;
	player_y_delta(&player->obj) = 0;
}

static void level_update_player_anim_12(struct player_t *player) {
	player->obj.spr_num = 9;
	++player->obj.x_pos;
	player_jump_counter(&player->obj) = 7;
	player_x_delta(&player->obj) = 0;
	player_y_delta(&player->obj) = 0;
}

static void level_update_player_anim_18(struct player_t *player) {
	level_player_update_hdirection(player);
	player->obj.spr_num = 18;
	if (player_jump_counter(&player->obj) != 0 && g_vars.input_key_up) {
		--player_jump_counter(&player->obj);
		player_y_delta(&player->obj) += (int8_t)player_anim_dy[player_jump_counter(&player->obj)];
	}
}

static void level_update_player_anim_15(struct player_t *player) {
	if (player_jump_counter(&player->obj) != 7) {
		level_update_player_anim_18(player);
	} else {
		player->obj.spr_num = 15;
		if (g_vars.input_key_right) {
			level_player_update_xvelocity(player, 2);
		} else if (g_vars.input_key_left) {
			level_player_update_xvelocity(player, -2);
		} else {
			level_player_update_xvelocity(player, 0);
		}
	}
}

static void level_update_player_anim_13(struct player_t *player) {
	if (player_jump_counter(&player->obj) != 7) {
		level_update_player_anim_18(player);
	} else {
		player->obj.spr_num = 13;
		level_player_update_flags(player);
		level_player_update_xvelocity(player, 0);
	}
}

static void level_update_player_anim_19(struct player_t *player) {
	if (player->vinyls_count == 0) {
		play_sound(SOUND_8);
		level_update_player_anim_13(player);
	} else {
		level_update_player_anim_6(player);
		player->obj.spr_num = 19;
	}
}

static void level_update_player_anim_26(struct player_t *player) {
	level_player_update_hdirection(player);
	player->obj.spr_num = 26;
	if (player_jump_counter(&player->obj) == 0 || !g_vars.input_key_up) {
		return;
	}
	--player_jump_counter(&player->obj);
	player_y_delta(&player->obj) += (int8_t)player_anim_dy[player_jump_counter(&player->obj)];
}

static void level_update_player_anim_27(struct player_t *player) {
	player->obj.spr_num = 27;
	level_player_update_xvelocity(player, 0);
	if (player_throw_counter(&player->obj) == 0) {
		return;
	}
	if (player_flags(&player->obj) & PLAYER_FLAGS_POWERUP) {
		player->obj.spr_num = 35;
	}
	player_flags(&player->obj) |= PLAYER_FLAGS_THROW_VINYL;
	player_flags(&player->obj) &= ~0x40;
	// struct player_t *other_player = player->other_player;
}

static void level_update_player_anim_28(struct player_t *player) {
	player->obj.y_pos += 12;
	level_update_player_anim_27(player);
	player->obj.y_pos -= 12;
	if (player_throw_counter(&player->obj) != 0 && (player_flags(&player->obj) & PLAYER_FLAGS_POWERUP) != 0) {
		player_obj_num(&player->obj) = 35;
	} else {
		player_obj_num(&player->obj) = 28;
	}
}

static void level_update_player_anim_29(struct player_t *player) {
	player->obj.spr_num = 29;
	player->obj.y_pos -= 2;
	const int x = (player->obj.x_pos & 15) - 8;
	if (x < 0) {
		++player->obj.x_pos;
	} else if (x > 0) {
		--player->obj.x_pos;
	}
	player_jump_counter(&player->obj) = 7;
	player_x_delta(&player->obj) = 0;
	player_y_delta(&player->obj) = 0;
}

static void level_update_player_anim_30(struct player_t *player) {
	player->obj.spr_num = 30;
	player_jump_counter(&player->obj) = 7;
	player_x_delta(&player->obj) = 0;
	player_y_delta(&player->obj) = 0;
}

static void level_update_player_anim_32(struct player_t *player) {
	player->obj.spr_num = 30;
	--player->obj.x_pos;
	player_jump_counter(&player->obj) = 7;
	player_x_delta(&player->obj) = 0;
	player_y_delta(&player->obj) = 0;
}

static void level_update_player_anim_33(struct player_t *player) {
	player->obj.spr_num = 30;
	++player->obj.x_pos;
	player_jump_counter(&player->obj) = 7;
	player_x_delta(&player->obj) = 0;
	player_y_delta(&player->obj) = 0;
}

static void level_update_player_from_input(struct player_t *player) {
	if (player_flags2(&player->obj) & 8) { // dead
		if ((player_flags2(&player->obj) & 1) == 0) {
			player_y_delta(&player->obj) = -120;
			player_x_delta(&player->obj) = (player->obj.x_pos < (g_vars.tilemap_x * 16 + TILEMAP_SCREEN_W / 2)) ? 24 : -24;
			player_flags2(&player->obj) |= 1;
		}
		player->obj.x_pos += player_x_delta(&player->obj) >> 3;
		player->obj.y_pos += player_y_delta(&player->obj) >> 3;
		player_y_delta(&player->obj) = MIN(player_y_delta(&player->obj) + 7, 120);
		if ((player->obj.y_pos >> 4) - g_vars.tilemap_y > TILEMAP_SCREEN_H / 16 + 3) {
			player_flags2(&player->obj) |= 0x10;
		}
		player_flags(&player->obj) &= ~PLAYER_FLAGS_THROW_VINYL;
		if (g_vars.input_key_space) {
			player_flags(&player->obj) |= PLAYER_FLAGS_THROW_VINYL;
		}
		return;
	}
	if (player->unk_counter != 0) {
		--player->unk_counter;
	}
	if (player->change_hdir_counter != 0) {
		--player->change_hdir_counter;
	}
	player_flags2(&player->obj) &= ~4;
	player_flags2(&player->obj) |= (g_vars.input_key_down & 4);
	if ((player_flags(&player->obj) & PLAYER_FLAGS_THROW_VINYL) != 0 && player->vinyls_count > 4) {
		if (player_power(&player->obj) < 255) {
			++player_power(&player->obj);
		}
	} else {
		player_power(&player->obj) = 0;
	}
	uint8_t flags = player_flags(&player->obj);
	if (g_vars.input_key_right) {
		player_flags(&player->obj) &= ~PLAYER_FLAGS_FACING_LEFT;
	}
	if (g_vars.input_key_left) {
		player_flags(&player->obj) |= PLAYER_FLAGS_FACING_LEFT;
	}
	if (((flags ^ player_flags(&player->obj)) & PLAYER_FLAGS_FACING_LEFT) != 0) { // horizontal direction changed
		player->change_hdir_counter += 4;
	}
	// flags = player_flags(&player->obj);
	uint8_t mask = 0;
	if (flags & PLAYER_FLAGS_STAIRS) {
		mask |= 4;
	}
	if (flags & 0x40) {
		mask |= 2;
	}
	if (flags & PLAYER_FLAGS_POWERUP) {
		mask |= 1;
	}
	flags = g_vars.input_key_space & 1;
	if (flags == 0) {
		if (player_power(&player->obj) >= 33 && (player_flags(&player->obj) & PLAYER_FLAGS_THROW_VINYL) != 0) {
			flags = 1;
		} else {
			player_flags(&player->obj) &= ~PLAYER_FLAGS_THROW_VINYL;
		}
	} else {
		if (player_flags(&player->obj) & PLAYER_FLAGS_THROW_VINYL) {
			flags = 0;
		}
	}
	mask = (mask << 1) | flags;
	if (player_flags2(&player->obj) & 1) {
		mask <<= 4;
		mask |= 2;
	} else {
		mask <<= 4;
		if ((player_flags(&player->obj) & 0x10) == 0) {
			mask |= (g_vars.input_key_up & 8);
		}
		mask |= (g_vars.input_key_right & 4);
		if (player_jump_counter(&player->obj) == 7) {
			mask |= ((player_flags(&player->obj) & 0x10) == 0) ? (g_vars.input_key_down & 2) : 2;
		}
		mask |= (g_vars.input_key_left & 1);
	}
	if (player_flags2(&player->obj) & 0x40) {
		player->dir_mask = mask;
		mask = 0;
	}
	switch (player_anim_lut[mask]) {
	case 0:
		level_update_player_anim_0(player);
		break;
	case 1:
		level_update_player_anim_1(player);
		break;
	case 2:
		level_update_player_anim_2(player);
		break;
	case 3:
		level_update_player_anim_3(player);
		break;
	case 4:
		level_update_player_anim_4(player);
		break;
	case 5:
		level_update_player_anim_5(player);
		break;
	case 6:
		level_update_player_anim_6(player);
		break;
	case 7:
		level_update_player_anim_7(player);
		break;
	case 8:
		level_update_player_anim_8(player);
		break;
	case 9:
		level_update_player_anim_9(player);
		break;
	case 10:
		level_update_player_anim_10(player, 10);
		break;
	case 11:
		level_update_player_anim_11(player);
		break;
	case 12:
		level_update_player_anim_12(player);
		break;
	case 13:
	case 35:
		level_update_player_anim_13(player);
		break;
	case 15:
		level_update_player_anim_15(player);
		break;
	case 17:
		if (player_y_delta(&player->obj) != 0) {
			level_update_player_anim_13(player);
		} else {
			player->obj.spr_num = 17;
			level_player_update_xvelocity(player, 0);
		}
		break;
	case 18:
		level_update_player_anim_18(player);
		break;
	case 19:
		level_update_player_anim_19(player);
		break;
	case 20:
		if (player->vinyls_count == 0) {
			play_sound(SOUND_8);
			level_update_player_anim_13(player);
		} else {
			player->obj.y_pos += 12;
			level_update_player_anim_6(player);
			player->obj.y_pos -= 12;
		}
		player->obj.spr_num = 20;
		break;
	case 21:
		if (player_jump_counter(&player->obj) != 7) {
			level_update_player_anim_26(player);
		} else {
			player->obj.spr_num = 21;
			level_player_update_flags(player);
			level_player_update_xvelocity(player, 0);
		}
		break;
	case 23:
		if (player_jump_counter(&player->obj) != 7) {
			level_update_player_anim_26(player);
		} else {
			player->obj.spr_num = 23;
			if (g_vars.input_key_right) {
				level_player_update_xvelocity(player, 2);
			} else if (g_vars.input_key_left) {
				level_player_update_xvelocity(player, -2);
			} else {
				level_player_update_xvelocity(player, 0);
			}
		}
		break;
	case 25:
		if (player_y_delta(&player->obj) != 0) {
			level_update_player_anim_0(player);
		} else {
			player->obj.spr_num = 25;
			level_player_update_xvelocity(player, 0);
		}
		break;
	case 26:
		level_update_player_anim_26(player);
		break;
	case 27:
		level_update_player_anim_27(player);
		break;
	case 28:
		level_update_player_anim_28(player);
		break;
	case 29:
		level_update_player_anim_29(player);
		break;
	case 30:
		level_update_player_anim_30(player);
		break;
	case 31:
		level_update_player_anim_10(player, 31);
		break;
	case 32:
		level_update_player_anim_32(player);
		break;
	case 33:
		level_update_player_anim_33(player);
		break;
	default:
		print_warning("Unhandled anim_lut %d mask %d", player_anim_lut[mask], mask);
		break;
	}
	if (g_vars.tilemap_h <= (player->obj.y_pos >> 4)) {
		level_player_die(player);
	}
	int ax = player_x_delta(&player->obj) >> 3;
	player->obj.x_pos += ax;
	int dx = g_vars.tilemap_w << 4;
	if (player->obj.x_pos >= dx || player->obj.x_pos < 0) {
		player->obj.x_pos -= ax;
		ax = 0;
	}
	if (g_vars.player_xscroll != 0) {
		g_vars.player_xscroll += ax;
	}
	player->obj.y_pos += player_y_delta(&player->obj) >> 3;
	int x = (g_vars.tilemap_x << 4) + 4;
	if (player_x_delta(&player->obj) <= 0 && player->obj.x_pos < x) {
		player->obj.x_pos = x;
	}
	if (player_x_delta(&player->obj) >= 0) {
		x += TILEMAP_SCREEN_W - 4;
		if (player->obj.x_pos > x) {
			player->obj.x_pos = x;
		}
	}
}

static void level_update_player_position() {
	assert(g_vars.player != 2);
	struct player_t *bp = &g_vars.players_table[0];
	if ((player_flags2(&bp->obj) & 8) == 0) {
		if ((g_vars.input_key_right | g_vars.input_key_left) == 0) {
		}
	}
	assert(g_vars.player != 2);
	struct object_t *obj = &g_vars.players_table[0].obj;
	if (player_flags2(obj) & 8) {
		return;
	}
	if (player_flags2(obj) & 1) {
		obj = &g_vars.players_table[1].obj;
	}
	if ((player_flags(obj) & 1) == 0 && (player_x_delta(obj) == 0)) {
		g_vars.player_xscroll = 0;
	}
	if (g_vars.player_xscroll != 0) {
		const int dx = MIN(abs(g_vars.player_xscroll) >> 4, 2);
		if (dx == 0) {
			g_vars.player_xscroll = obj->x_pos - (g_vars.tilemap_x << 4) - TILEMAP_SCREEN_W / 2;
		} else {
			if (g_vars.player_xscroll > 0) {
				g_vars.player_xscroll -= dx << 2;
				if (g_vars.player_xscroll < 0) {
					g_vars.player_xscroll = 0;
				}
				level_adjust_hscroll_right(dx);
			} else {
				g_vars.player_xscroll += dx << 2;
				if (g_vars.player_xscroll > 0) {
					g_vars.player_xscroll = 0;
				}
				level_adjust_hscroll_left(dx);
			}
		}
	} else {
		g_vars.player_xscroll += obj->x_pos - (g_vars.tilemap_x << 4) - TILEMAP_SCREEN_W / 2;
	}
	const int y = (g_vars.tilemap_y << 4) + g_vars.tilemap_scroll_dy + (TILEMAP_SCREEN_H / 2) - obj->y_pos;
	if (y >= 0) {
		const int dy = (int8_t)vscroll_offsets_table[y];
		level_adjust_vscroll_up(dy);
	} else {
		const int dy = (int8_t)vscroll_offsets_table[-y];
		level_adjust_vscroll_down(dy);
	}
}

static void level_update_input() {
	if (g_vars.input_keystate[0x19]) {
	}
	if (g_vars.input_keystate[0x3B]) {
		if (player_flags2(&g_vars.players_table[0].obj) & 8) {
			level_player_die(&g_vars.players_table[0]);
		}
		assert(g_vars.player != 2);
	}
	if (g_vars.input_keystate[0x3C]) {
		g_vars.players_table[0].lifes_count = 0;
		g_vars.players_table[1].lifes_count = 0;
		player_flags2(&g_vars.players_table[0].obj) = 8;
		player_flags2(&g_vars.players_table[1].obj) = 8;
	}
	level_update_player_from_input(&g_vars.players_table[0]);
	level_update_player_position();
}

static int level_collide_objects(const struct object_t *si, const struct object_t *di) {
	if (di->spr_num == 0xFFFF) {
		return 0;
	}
	const int dx = abs(si->x_pos - di->x_pos);
	if (dx >= 80) {
		return 0;
	}
	const int dy = abs(si->y_pos - di->y_pos);
	if (dy >= 70) {
		return 0;
	}
	// if (g_vars.input_keystate[0x2A]) {
	int d = di->y_pos + (sprite_offsets[di->spr_num & 0x1FFF] >> 8) - (sprite_sizes[di->spr_num & 0x1FFF] >> 8);
	int a = si->y_pos + (sprite_offsets[si->spr_num & 0x1FFF] >> 8) - (sprite_sizes[si->spr_num & 0x1FFF] >> 8);
	if (a < d) {
		SWAP(a, d);
		SWAP(si, di);
	}
	a -= sprite_offsets[si->spr_num & 0x1FFF] >> 8;
	if (a >= d) {
		return 0;
	}
	d = si->x_pos - (sprite_sizes[si->spr_num & 0x1FFF] & 255);
	a = di->x_pos - (sprite_sizes[di->spr_num & 0x1FFF] & 255);
	int b = sprite_offsets[di->spr_num & 0x1FFF] & 255;
	if (a >= d) {
		SWAP(a, d);
		b = sprite_offsets[si->spr_num & 0x1FFF] & 255;
	}
	return (a + b >= d);
}

static int level_is_object_visible(int x_pos, int y_pos) {
	const int x = abs((x_pos >> 4) - g_vars.tilemap_x - (TILEMAP_SCREEN_W / 32));
	if (x < (TILEMAP_SCREEN_W / 32 + 4)) {
		const int y = abs((y_pos >> 4) - g_vars.tilemap_y - (TILEMAP_SCREEN_H / 32));
		return (y < 10) ? 1 : 0;
	}
	return 0;
}

static int level_is_respawn_object_visible(const uint8_t *data) {
	const int x_pos = READ_LE_UINT16(data);
	const int y_pos = READ_LE_UINT16(data + 2);
	if (g_vars.level_loop_counter != 0) {
		const int x = abs((x_pos >> 4) - g_vars.tilemap_x - (TILEMAP_SCREEN_W / 32));
		if (x <= (TILEMAP_SCREEN_W / 32 + 2)) {
			const int y = (y_pos >> 4) - g_vars.tilemap_y;
			return (y >= 0 && y <= 12) ? 1 : 0;
		}
	}
	return !level_is_object_visible(x_pos, y_pos);
}

static void init_object82(int type, int x_pos, int y_pos, struct object_t *obj, uint16_t *ptr) {
	obj->data[2].w = ptr - g_vars.buffer;
	object82_state(obj) = 0;
	object82_type(obj) = type;
	obj->x_pos = x_pos;
	obj->y_pos = y_pos;
	*ptr = obj - g_vars.objects_table;
	object_blinking_counter(obj) = 0;
}

static uint8_t get_random_number() {
	static uint8_t random_a = 5;
	static uint8_t random_b = 34;
	static uint8_t random_c = 134;
	static uint16_t random_d = 58765;

	random_d += random_a;
	random_a += 3 + (random_d >> 8);

	random_b += random_c;
	random_b *= 2;
	random_b += random_a;

	random_c ^= random_a;
	random_c ^= random_b;

	return random_b;
}

static void level_init_object102_1(struct object_t *obj22, int x, int y, int num) {
	struct object_t *obj = find_object(102, 10, 1);
	if (obj) {
		obj->x_pos = x;
		obj->y_pos = y;
		obj->spr_num = num;
		object_blinking_counter(obj) = 0;
		const int n = (get_random_number() & 7) << 3;
		object102_x_delta(obj) = (object22_xvelocity(obj22) < 0) ? -n : n;
		object102_y_delta(obj) = -(((get_random_number() & 7) + 4) << 3);
	}
}

static void level_init_object102_2(struct object_t *obj22, int x, int y) {
	struct object_t *obj = find_object(102, 10, 1);
	if (obj) {
		obj->x_pos = x + 8;
		obj->y_pos = y + 12;
		obj->spr_num = (get_random_number() & 1) + 96;
		object_blinking_counter(obj) = 0;
		object102_x_delta(obj) = (object22_xvelocity(obj22) < 0) ? -16 : 16;
		object102_y_delta(obj) = -80;
	}
}

static void level_player_collide_object(struct player_t *player, struct object_t *obj) {
	if ((player_flags2(&player->obj) & 0x20) == 0) {
		return;
	}
	if ((obj->y_pos & 15) < 12) {
		return;
	}
	struct object_t *player_obj = &g_vars.objects_table[player_obj_num(&player->obj)];
	if (level_collide_objects(obj, player_obj)) {
		level_hit_player(player);
	}
}

static bool level_collide_vinyl_decor(int x, int y, struct object_t *obj) {
	const int offset = (y >> 4) * g_vars.tilemap_w + (x >> 4);
	uint8_t al = g_vars.tilemap_data[offset];
	al = g_vars.level_tiles_lut[al];
	if (al == 3) {
		object22_damage(obj) = 0;
		obj->spr_num = 0xFFFF;
		g_vars.tilemap_flags ^= 1;
		play_sound(SOUND_3);
	} else if (al == 9) {
		g_vars.tilemap_data[offset] = 0;
		level_init_object102_2(obj, x, y);
		play_sound(SOUND_1);
	} else if (al == 10) {
		obj->spr_num = 0xFFFF;
	} else {
		return false;
	}
	if (object22_damage(obj) <= 0x200) {
		obj->spr_num = 0xFFFF;
	}
	level_init_object_spr_num_98_align_pos(x, y);
	return true;
}

static struct object_t *level_collide_vinyl_objects82(struct object_t *obj) {
	for (int i = 0; i < 20; ++i) {
		struct object_t *obj2 = &g_vars.objects_table[82 + i];
		if (obj2->x_pos != -1 && object82_type(obj2) != 16 && object82_type(obj2) != 7) {
			if (level_collide_objects(obj, obj2)) {
				return obj2;
			}
		}
	}
	return 0;
}

static struct object_t *level_collide_vinyl_objects64(struct object_t *obj) {
	for (int i = 0; i < 8; ++i) {
		struct object_t *obj2 = &g_vars.objects_table[64 + i];
		if (obj2->x_pos != -1 && (obj2->spr_num & 0x1FFF) == 199) {
			if (level_collide_objects(obj, obj2)) {
				return obj2;
			}
		}
	}
	return 0;
}

static int level_update_object82_position(struct object_t *obj, int8_t dx, int8_t dy) {
	int ret = 0;
	object82_hflip(obj) = dx;
	obj->x_pos += dx >> 3;
	if (obj->x_pos < 0) {
		obj->x_pos = 0;
		++ret;
	}
	if (g_vars.tilemap_w <= (obj->x_pos >> 4)) {
		obj->x_pos -= 8;
		++ret;
	}
	obj->y_pos += dy >> 3;
	if (obj->y_pos < 0) {
		obj->y_pos = 0;
	}
	return ret;
}

static struct player_t *level_get_closest_player(struct object_t *obj, int x_dist) {
	struct player_t *player = &g_vars.players_table[0];
	int dx = abs(obj->x_pos - player->obj.x_pos);
	if (dx < x_dist) {
		return player;
	}
	if (g_vars.objects_table[1].spr_num != 0xFFFF) {
		++player;
		dx = abs(obj->x_pos - player->obj.x_pos);
		if (dx < x_dist) {
			return player;
		}
	}
	return 0;
}

static void level_update_object82_type0(struct object_t *obj) {
	if (object82_anim_data(obj) != &monster_spr_anim_data0[2] && !level_is_object_visible(obj->x_pos, obj->y_pos)) {
		obj->spr_num = 0xFFFF;
		g_vars.buffer[obj->data[2].w] = 0xFFFF;
		object82_counter(obj) = 0;
		g_vars.buffer[128 + obj->data[2].w] = g_vars.level_loop_counter;
	} else {
		const int state = object82_state(obj);
		if (state == 0) {
			const int angle = object82_counter(obj);
			const int16_t r1 = (60 * (int16_t)rotation_table[angle + 65]) >> 8;
			object82_hflip(obj) = r1 >> 8;
			const int16_t r2 = (60 * (int16_t)rotation_table[angle]) >> 8;
			object82_counter(obj) += 2;
			const uint8_t *p = object82_type0_init_data(obj);
			obj->x_pos = r1 + READ_LE_UINT16(p);
			obj->y_pos = r2 + READ_LE_UINT16(p + 2);
			struct player_t *player = level_get_closest_player(obj, 80);
			if (player) {
				object82_type0_player_num(obj) = player - &g_vars.players_table[0];
				object82_state(obj) = 1;
				obj->data[6].b[0] = 0;
				obj->data[7].b[0] = 0;
			}
		} else if (state == 1) {
			struct player_t *player = &g_vars.players_table[object82_type0_player_num(obj)];
			int16_t x_pos = player->obj.x_pos;
			if ((player_flags2(&player->obj) & 0x40) != 0) {
				x_pos += 60;
				if ((player->dir_mask & 4) == 0) {
					x_pos -= 120;
					if ((player->dir_mask & 1) == 0) {
						x_pos += 60;
						if ((player_flags(&player->obj) & PLAYER_FLAGS_FACING_LEFT) != 0) {
							x_pos -= 20;
						} else {
							x_pos = player->obj.x_pos;
						}
					}
				}
			}
			x_pos -= obj->x_pos;
			x_pos = (x_pos >> 8) | (1 + object82_type0_x_delta(obj));
			int dx = 32;
			if (x_pos < dx) {
				dx = -dx;
				if (x_pos > dx) {
					dx = x_pos;
				}
			}
			object82_type0_x_delta(obj) = dx;
			int16_t y_pos = player->obj.y_pos;
			if ((player_flags2(&player->obj) & 0x40) != 0) {
				y_pos -= 60;
				if ((player->dir_mask & 8) == 0) {
					y_pos += 120;
					if ((player->dir_mask & 2) == 0) {
						y_pos -= 60;
					}
				}
			}
			y_pos -= obj->y_pos;
			y_pos = (y_pos >> 8) | (1 + object82_type0_y_delta(obj));
			int dy = 32;
			if (y_pos < dy) {
				dy = -dy;
				if (y_pos > dy) {
					dy = y_pos;
				}
			}
			object82_type0_y_delta(obj) = dy;
			if (level_update_object82_position(obj, object82_type0_x_delta(obj), object82_type0_y_delta(obj)) != 0) {
				object82_type0_x_delta(obj) = -object82_type0_x_delta(obj);
			}
		}
	}
}

// snail
static void level_update_object82_type1(struct object_t *obj) {
	if (!level_is_object_visible(obj->x_pos, obj->y_pos)) {
		obj->spr_num = 0xFFFF;
		g_vars.buffer[obj->data[2].w] = 0xFFFF;
		object82_counter(obj) = 0;
	} else {
		int dx = object82_x_delta(obj) + (int8_t)object82_type1_hdir(obj);
		if (dx < 128 && dx > -128) {
			object82_x_delta(obj) += dx;
		}
		if ((g_vars.level_loop_counter & 3) == 0) {
			++object82_counter(obj);
			if (object82_counter(obj) >= 75) {
				object82_type1_hdir(obj) = -object82_type1_hdir(obj);
				object82_counter(obj) = 0;
			}
		}
		dx = obj->data[7].b[0] + object82_x_delta(obj);
		obj->data[7].b[0] = dx;
		obj->x_pos += (dx >> 8);
		object82_hflip(obj) = object82_type1_hdir(obj);
		level_update_tiles(obj, obj->x_pos, obj->y_pos, _undefined);
		obj->y_pos += object82_y_delta(obj) >> 3;
	}
}

static void level_update_object82_type3(struct object_t *obj) {
	if (!level_is_object_visible(obj->x_pos, obj->y_pos)) {
		obj->spr_num = 0xFFFF;
		g_vars.buffer[obj->data[2].w] = 0xFFFF;
		object82_counter(obj) = 0;
	} else {
		const int al = object82_state(obj);
		if (al == 0) {
			if (level_get_closest_player(obj, 160)) {
				object82_state(obj) = 1;
				object82_anim_data(obj) = monster_spr_anim_data3 + 4;
				object82_counter(obj) = 40;
			}
		} else if (al == 1) {
			level_update_tiles(obj, obj->x_pos, obj->y_pos, _undefined);
			level_update_object82_position(obj, 0, object82_y_delta(obj));
			if (--object82_counter(obj) == 0) {
				object82_counter(obj) = 40;
				object82_y_delta(obj) = -80;
			}
		}
	}
}

static int level_is_object_on_tile_5dc8(struct object_t *obj) {
	uint8_t tile_num;
	const int offset = g_vars.player_map_offset;
	tile_num = level_get_tile(offset - g_vars.tilemap_w);
	if (tile_funcs2[tile_num] == 0x5DC8) {
		return 0;
	}
	tile_num = level_get_tile(offset - g_vars.tilemap_w - 1);
	if (tile_funcs2[tile_num] == 0x5DC8) {
		return 1;
	}
	tile_num = level_get_tile(offset - g_vars.tilemap_w + 1);
	if (tile_funcs2[tile_num] == 0x5DC8) {
		return 1;
	}
	obj->data[0].b[1] = 0x10;
	return 1;
}

static void level_update_object82_type2(struct object_t *obj) {
	if (!level_is_object_visible(obj->x_pos, obj->y_pos)) {
		obj->spr_num = 0xFFFF;
		g_vars.buffer[obj->data[2].w] = 0xFFFF;
		object82_counter(obj) = 0;
	} else {
		int state = object82_state(obj);
		if (state == 0) {
			struct player_t *player = level_get_closest_player(obj, 160);
			if (player) {
				object82_state(obj) = 1;
				obj->data[5].w = -56;
				obj->data[4].b[0] = obj->data[4].b[1];
				obj->data[4].b[1] = -obj->data[4].b[1];
				object82_anim_data(obj) += 2;
				object82_counter(obj) = 4;
			}
		} else {
			if (state == 1) {
				if (obj->data[5].w == 0 && --object82_counter(obj) == 0) {
					object82_state(obj) = state = 2;
					object82_counter(obj) = 20;
				}
			}
			if (state == 2) {
				object82_anim_data(obj) = monster_spr_anim_data3;
				if (--object82_counter(obj) != 0) {
					goto update_tiles; // do not update position
				}
				object82_state(obj) = 0;
				obj->data[5].w = 0;
				obj->data[4].b[0] = 0;
			}
		}
		if (level_update_object82_position(obj, obj->data[4].b[0], 0) != 0) {
			obj->data[4].b[0] = -obj->data[4].b[0];
		}
update_tiles:
		level_update_tiles(obj, obj->x_pos, obj->y_pos, _undefined);
		if (level_is_object_on_tile_5dc8(obj)) {
			object82_x_delta(obj) = -object82_x_delta(obj);
			if (level_update_object82_position(obj, obj->data[4].b[0], 0) != 0) {
				obj->data[4].b[0] = -obj->data[4].b[0];
			}
		}
		obj->y_pos += object82_y_delta(obj) >> 3;
	}
}

// grass cutter
static void level_update_object82_type4_6(struct object_t *obj) {
	if (!level_is_object_visible(obj->x_pos, obj->y_pos)) {
		obj->spr_num = 0xFFFF;
		g_vars.buffer[obj->data[2].w] = 0xFFFF;
		object82_counter(obj) = 0;
	} else {
		const int state = object82_state(obj);
		if (state == 0) {
			struct player_t *player = level_get_closest_player(obj, 300);
			if (player) {
				object82_x_delta(obj) = (obj->x_pos < player->obj.x_pos) ? 8 : -8;
				object82_state(obj) = 1;
			}
		} else if (state == 1) { // head to player
			struct player_t *player = level_get_closest_player(obj, 300);
			if (player) {
				object82_x_delta(obj) <<= 2; // faster
				object82_anim_data(obj) = (object82_type(obj) == 4) ? (monster_spr_anim_data6 + 10) : (monster_spr_anim_data8 + 26);
				object82_state(obj) = 2;
			}
		}
		level_update_tiles(obj, obj->x_pos, obj->y_pos, _undefined);
		if (level_is_object_on_tile_5dc8(obj)) {
			object82_x_delta(obj) = -object82_x_delta(obj);
		}
		object82_hflip(obj) = object82_x_delta(obj) >> 8;
		if (level_update_object82_position(obj, object82_x_delta(obj), object82_y_delta(obj)) != 0) {
			object82_x_delta(obj) = -object82_x_delta(obj);
		}
	}
}

// dragon
static void level_update_object82_type7(struct object_t *obj) {
	level_update_object82_type0(obj);
	int num = g_vars.dragon_coords[0];
	const int x_pos = obj->x_pos;
	const int y_pos = obj->y_pos;
	g_vars.dragon_coords[(num + 2) / 2] = x_pos;
	g_vars.dragon_coords[(num + 4) / 2] = y_pos;
	if (player_flags2(&g_vars.players_table[0].obj) & 0x40) {
		g_vars.players_table[0].obj.x_pos = x_pos;
		g_vars.players_table[0].obj.y_pos = y_pos;
		int spr_num = 6;
		if (obj->spr_num & 0x8000) {
			spr_num |= 0x8000;
		}
		if (player_flags(&g_vars.players_table[0].obj) & PLAYER_FLAGS_JAKE) {
			spr_num += 50;
		}
		const int dx = (int8_t)obj->data[6].b[0];
		player_x_delta(&g_vars.players_table[0].obj) = (dx >> 2) + dx;
	}
	if (player_flags2(&g_vars.players_table[1].obj) & 0x40) {
		g_vars.players_table[1].obj.x_pos = x_pos;
		g_vars.players_table[1].obj.y_pos = y_pos;
		const int dx = (int8_t)obj->data[6].b[0];
		player_x_delta(&g_vars.players_table[1].obj) = (dx >> 2) + dx;
	}
	num = (num + 4) & 0x7F;
	g_vars.dragon_coords[0] = num;
	for (int i = 0; i < 8; ++i) {
		obj = &g_vars.objects_table[121 + i];
		num -= 12;
		if (num < 0) {
			num += 128;
		}
		obj->x_pos = g_vars.dragon_coords[(num + 2) / 2];
		obj->y_pos = g_vars.dragon_coords[(num + 4) / 2];
		object_blinking_counter(obj) = 0;
	}
}

// dead monster
static void level_update_object82_type16(struct object_t *obj) {
	if (!level_is_object_visible(obj->x_pos, obj->y_pos)) {
		obj->spr_num = 0xFFFF;
		g_vars.buffer[obj->data[2].w] = 0xFFFF;
		object82_counter(obj) = 0;
		g_vars.buffer[128 + obj->data[2].w] = g_vars.level_loop_counter;
	} else {
		obj->y_pos += object82_energy(obj) >> 3;
		const int y_vel = object82_energy(obj) + 4;
		if (y_vel < 128) {
			object82_energy(obj) = y_vel;
		}
	}
}

static void level_update_triggers() {
	const int count = g_vars.triggers_table[18];
	const uint8_t *start = g_vars.triggers_table + 19;
	const uint8_t *data = start;
	uint16_t *ptr = g_vars.buffer;
	for (int i = 0; i < count; ++i, data += 16, ++ptr) {
		if (*ptr != 0xFFFF) {
			continue;
		}
		const int x_pos = READ_LE_UINT16(data);
		const int y_pos = READ_LE_UINT16(data + 2);
		if (READ_LE_UINT16(data + 4) == 0 && READ_LE_UINT16(data + 6) == 9) {
			const uint16_t num = READ_LE_UINT16(data + 8);
			if (num == 12 || num == 25 || num == 8) {
				if (!level_is_object_visible(x_pos, y_pos)) {
					continue;
				}
				struct object_t *obj = find_object(32, 8, 4);
				if (obj) {
					int dx = 205;
					if (num != 8) {
						dx = 224; // ball with spikes
						if (num == 12) {
							WRITE_LE_UINT16(&g_vars.triggers_table[19] + i * 16 + 14, 0x260);
						}
					}
					obj[0].spr_num = dx;
					object_blinking_counter(&obj[0]) = 0;
					obj[0].data[1].w = READ_LE_UINT16(data + 14);
					obj[1].spr_num = 204;
					object_blinking_counter(&obj[1]) = 0;
					obj[2].spr_num = 204;
					object_blinking_counter(&obj[2]) = 0;
					obj[3].spr_num = 204;
					object_blinking_counter(&obj[3]) = 0;
					obj->data[2].b[1] = 128; // angle
					obj->data[2].b[0] = 0;
					obj->data[4].b[1] = 58; // radius
					obj->data[4].b[0] = 0;
					obj->data[5].w = 8; // angle step
					*ptr = obj - g_vars.objects_table;
					obj->data[0].w = data - start;
				}

			} else if (num == 2 || num == 24) {
				if (level_is_respawn_object_visible(data)) {
					continue;
				}
				struct object_t *obj = find_object(64, 8, 1);
				if (obj) {
					obj->spr_num = (num == 2) ? 199 : 205; // crate
					obj->x_pos = x_pos;
					obj->y_pos = y_pos;
					object64_counter(obj) = READ_LE_UINT16(data + 14);
					object_blinking_counter(obj) = 0;
					*ptr = obj - g_vars.objects_table;
					obj->data[0].w = data - start;
					object64_yvelocity(obj) = 0;
				}
			} else if (num == 15) {
				struct object_t *obj = find_object(82, 20, 1);
				if (obj) {
					obj->spr_num = 198; // jukebox
					init_object82(5, x_pos, y_pos, obj, ptr);
					object82_anim_data(obj) = monster_spr_anim_data9;
				}
			} else if (num == 19 || num == 3 || num == 16) {
				if (abs((int16_t)ptr[128] - g_vars.level_loop_counter) < _object_respawn_delay) {
					continue;
				}
				if (level_is_respawn_object_visible(data)) {
					continue;
				}
				struct object_t *obj = find_object(82, 20, 1);
				if (obj) {
					obj->spr_num = 214; // mosquito
					object82_anim_data(obj) = monster_spr_anim_data1;
					int type = 0;
					if (num == 3) {
						obj->spr_num = 230; // dragon
						object82_anim_data(obj) = monster_spr_anim_data0;
						for (int i = 0; i < 128; ++i) {
							g_vars.dragon_coords[1 + i] = -1;
						}
						g_vars.dragon_coords[0] = 0;
						g_vars.objects_table[121].spr_num = 233;
						g_vars.objects_table[122].spr_num = 233;
						g_vars.objects_table[123].spr_num = 233;
						g_vars.objects_table[124].spr_num = 233;
						g_vars.objects_table[125].spr_num = 233;
						g_vars.objects_table[126].spr_num = 233;
						g_vars.objects_table[127].spr_num = 234;
						g_vars.objects_table[128].spr_num = 235;
						type = 7; // fall-through
						obj->y_pos -= 60;
					}
					init_object82(type, x_pos, y_pos, obj, ptr);
					object82_energy(obj) = 128;
					object82_counter(obj) = 0;
					object82_type0_init_data(obj) = data;
				}
			} else if (num == 20) {
				if (level_is_respawn_object_visible(data)) {
					continue;
				}
				struct object_t *obj = find_object(82, 20, 1);
				if (obj) {
					obj->spr_num = 217; // snail
					init_object82(1, x_pos, y_pos, obj, ptr);
					object82_anim_data(obj) = monster_spr_anim_data2;
					object82_type1_hdir(obj) = -32;
					object82_x_delta(obj) = 0;
					obj->data[7].w = 0;
					object82_counter(obj) = 0;
					object82_y_delta(obj) = 0;
					object82_energy(obj) = 512;
				}
			} else if (num == 21 || num == 18) {
				if (abs((int16_t)ptr[128] - g_vars.level_loop_counter) < _object_respawn_delay) {
					continue;
				}
				if (level_is_respawn_object_visible(data)) {
					continue;
				}
				struct object_t *obj = find_object(82, 20, 1);
				if (obj) {
					uint8_t type;
					if (num == 21) {
						obj->spr_num = 228; // closed trap
						type = 3;
						object82_anim_data(obj) = monster_spr_anim_data3 + 24;
					} else {
						obj->spr_num = 225; // open trap
						object82_anim_data(obj) = monster_spr_anim_data3;
						type = 2;
					}
					init_object82(type, x_pos, y_pos, obj, ptr);
					object82_y_delta(obj) = 0;
					obj->data[4].b[0] = 0;
					obj->data[7].p = data;
					obj->data[4].b[1] = ((data[11] & 1) != 0) ? -40 : 40;
					object82_energy(obj) = 768;
				}
			} else if (num == 26 || num == 5 || num == 17) {
				if (abs((int16_t)ptr[128] - g_vars.level_loop_counter) < _object_respawn_delay) {
					continue;
				}
				if (level_is_respawn_object_visible(data)) {
					continue;
				}
				struct object_t *obj = find_object(82, 20, 1);
				if (obj) {
					obj->spr_num = 225; // grass cutter
					uint8_t type;
					if (num == 17) {
						type = 4;
						object82_anim_data(obj) = monster_spr_anim_data6;
					} else {
						type = 6;
						object82_anim_data(obj) = monster_spr_anim_data8;
					}
					init_object82(type, x_pos, y_pos, obj, ptr);
					obj->data[5].w = 0;
					obj->data[4].b[0] = 0;
					object82_energy(obj) = 512;
				}
			} else if (num == 14) {
				if (!level_is_object_visible(x_pos, y_pos)) {
					continue;
				}
				struct object_t *obj = find_object(112, 9, 1);
				if (obj) {
					obj->spr_num = 242; // vertical bar base
					obj->x_pos = x_pos;
					obj->y_pos = y_pos + 16;
					obj->data[3].w = obj->y_pos;
					obj->data[0].w = READ_LE_UINT16(data + 14);
					object_blinking_counter(obj) = 0;
					obj->data[2].w = 0;
					*ptr = obj - g_vars.objects_table;
					obj->data[1].w = data - start;
				}
			}
		} else if (READ_LE_UINT16(data + 4) == 2 && level_is_object_visible(x_pos, y_pos)) { // bonus
			struct object_t *obj = find_object(72, 10, 1);
			if (obj) {
				const uint16_t num = READ_LE_UINT16(data + 8);
				obj->spr_num = 190 + bonus_spr_table[num];
				object_blinking_counter(obj) = 0;
				obj->x_pos = x_pos;
				obj->y_pos = y_pos;
				*ptr = obj - g_vars.objects_table;
				obj->data[0].w = data - start;
			}
		}
	}
	for (int i = 0; i < 8; ++i) {
		struct object_t *obj = &g_vars.objects_table[32 + i * 4];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		obj->data[1].w += (obj->data[2].w & 0x8000) ? -3 : 3;
		const uint8_t *_di = start + obj->data[0].w;
		if (!level_is_object_visible(READ_LE_UINT16(_di), READ_LE_UINT16(_di + 2))) {
			obj[0].spr_num = 0xFFFF;
			obj[1].spr_num = 0xFFFF;
			obj[2].spr_num = 0xFFFF;
			obj[3].spr_num = 0xFFFF;
			g_vars.buffer[obj->data[0].w / 16] = 0xFFFF;
		} else {
			if (READ_LE_UINT16(_di + 8) == 25) {
				obj->data[4].w += obj->data[5].w;
				if (obj->data[4].w == 24000 || obj->data[4].w == 10240) {
					obj->data[5].w = -obj->data[5].w;
				}
				obj->data[1].w = (-(obj->data[4].w - 30000)) >> 4;
			}
			const int x_pos = obj->x_pos;
			obj[0].x_pos = obj[1].x_pos = obj[2].x_pos = obj[3].x_pos = READ_LE_UINT16(_di);
			obj[0].y_pos = obj[1].y_pos = obj[2].y_pos = obj[3].y_pos = READ_LE_UINT16(_di + 2);
			int dx, ax = obj[0].data[1].w * 2;
			obj[0].data[2].w += ax;
			const int angle = obj[0].data[2].b[1];
			const int radius = obj[0].data[4].b[1];

			ax = (radius * (int16_t)rotation_table[angle + 65]) >> 8;
			obj[0].x_pos += ax;
			ax >>= 1;
			dx = ax;
			obj[2].x_pos += ax;
			ax >>= 1;
			dx += ax;
			obj[3].x_pos += ax;
			obj[1].x_pos += dx;

			ax = (radius * (int16_t)rotation_table[angle]) >> 8;
			obj[0].y_pos += ax;
			ax >>= 1;
			dx = ax;
			obj[2].y_pos += ax;
			ax >>= 1;
			dx += ax;
			obj[3].y_pos += ax;
			obj[1].y_pos += dx;

			obj[0].data[3].w = x_pos - obj[0].x_pos;
		}
	}
	for (int i = 0; i < 8; ++i) {
		struct object_t *obj = &g_vars.objects_table[64 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		const int timer = object64_counter(obj);
		if (timer == 0) { // fall
			++object64_yvelocity(obj);
			if (obj->y_pos < 2048) {
				obj->y_pos += object64_yvelocity(obj);
			}
		} else if (timer <= 20) {
			obj->x_pos ^= (g_vars.level_loop_counter & 1);
		}
		const uint8_t *p = start + obj->data[0].w;
		if (!level_is_object_visible(READ_LE_UINT16(p), READ_LE_UINT16(p + 2))) {
			obj->spr_num = 0xFFFF;
			g_vars.buffer[obj->data[0].w / 16] = 0xFFFF;
		}
	}
	for (int i = 0; i < 10; ++i) {
		struct object_t *obj = &g_vars.objects_table[72 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		const uint8_t *p = start + obj->data[0].w;
		if (!level_is_object_visible(READ_LE_UINT16(p), READ_LE_UINT16(p + 2))) {
			obj->spr_num = 0xFFFF;
			g_vars.buffer[obj->data[0].w / 16] = 0xFFFF;
		}
	}
	for (int i = 0; i < 20; ++i) {
		struct object_t *obj = &g_vars.objects_table[2 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		if ((object2_spr_tick(obj) & g_vars.level_loop_counter) == 0) {
			--object2_spr_count(obj);
			if (object2_spr_count(obj) != 0) {
				++obj->spr_num;
			} else {
				obj->spr_num = 0xFFFF;
			}
		}
	}
	for (int i = 0; i < 9; ++i) {
		struct object_t *obj = &g_vars.objects_table[112 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		int offset;
		while (1) {
			obj->data[2].w += obj->data[0].w;
			obj->y_pos += obj->data[2].w >> 8;
			obj->data[2].w &= 0xFF;
			offset = (obj->y_pos >> 4) * g_vars.tilemap_w + (obj->x_pos >> 4);
			if (offset < 0) {
				print_warning("Invalid object #112 position %d,%d", obj->x_pos, obj->y_pos);
				break;
			}
			if (obj->data[0].w < 0) {
				offset -= g_vars.tilemap_w;
				if (obj->y_pos > obj->data[3].w) {
					break;
				}
			} else {
				level_player_collide_object(&g_vars.players_table[0], obj);
				level_player_collide_object(&g_vars.players_table[1], obj);
				uint8_t ah = g_vars.level_tiles_lut[g_vars.tilemap_data[offset]];
				uint8_t al = g_vars.level_tiles_lut[g_vars.tilemap_data[offset + 1]];
				if (ah != 1 && ah != 2 && al != 1 && al != 2) {
					break;
				}
			}
			obj->data[0].w = -obj->data[0].w;
		}
		if (obj->data[0].w < 0) {
			offset += g_vars.tilemap_w;
			if (g_vars.tilemap_data[offset + 1] != 0x95 && g_vars.tilemap_data[offset] != 0x95) {
				continue;
			}
			WRITE_LE_UINT16(g_vars.tilemap_data + offset - 2, 0);
			WRITE_LE_UINT16(g_vars.tilemap_data + offset, 0);
		} else {
			offset -= g_vars.tilemap_w;
			uint16_t tiles = READ_LE_UINT16(g_vars.tilemap_data + offset - 2);
			if (tiles != 0x9594 && tiles != 0) {
				level_init_object102_2(obj, offset % g_vars.tilemap_w - 2, offset / g_vars.tilemap_w);
			}
			WRITE_LE_UINT16(g_vars.tilemap_data + offset - 2, 0x9594);
			tiles = READ_LE_UINT16(g_vars.tilemap_data + offset);
			if (tiles != 0x9695 && tiles != 0) {
				level_init_object102_2(obj, offset % g_vars.tilemap_w, offset / g_vars.tilemap_w);
			}
			WRITE_LE_UINT16(g_vars.tilemap_data + offset, 0x9695);
		}
	}
	for (int i = 0; i < 10; ++i) {
		struct object_t *obj = &g_vars.objects_table[102 + i];
		if (obj->spr_num != 0xFFFF) {
			object102_y_delta(obj) += 8;
			obj->x_pos += object102_x_delta(obj) >> 3;
			obj->y_pos += object102_y_delta(obj) >> 3;
			const int y = (obj->y_pos >> 4) - g_vars.tilemap_y;
			if (y < 0 || y > TILEMAP_SCREEN_H / 16 + 3) {
				obj->spr_num = 0xFFFF;
			}
		}
	}
	for (int i = 0; i < 10; ++i) { // vinyls
		struct object_t *obj = &g_vars.objects_table[22 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		if (level_collide_vinyl_decor(obj->x_pos - 8, obj->y_pos - 8, obj)) {
			continue;
		}
		int16_t ax, dx;
		struct object_t *obj2 = level_collide_vinyl_objects82(obj);
		if (obj2 && object_blinking_counter(obj2) == 0) {
			level_init_object_spr_num_92(obj->x_pos, obj->y_pos);
			object82_y_delta(obj2) = -48;
			const int num = object82_type(obj2);
			if (num == 6 || num == 4 || num == 1) {
				ax = abs(object82_x_delta(obj2));
				object82_x_delta(obj2) = (obj->data[0].w < 0) ? -ax : ax;
			} else if (num == 2) {
				ax = abs((int8_t)obj2->data[4].b[0]);
				obj2->data[4].b[0] = (obj->data[0].w < 0) ? -ax : ax;
			}
			object_blinking_counter(obj2) = 3;
			if (obj2->spr_num == 198) {
				obj->spr_num = 0xFFFF;
				continue;
			}
			dx = object82_energy(obj2);
			ax = object82_energy(obj2) - object22_damage(obj);
			if (ax <= 0 || (g_options.cheats & CHEATS_ONE_HIT_VINYL) != 0) {
				play_sound(SOUND_1);
				ax = -80;
				object82_type(obj2) = 16;
				// seek to ko animation
				const uint8_t *p = object82_anim_data(obj2);
				while (READ_LE_UINT16(p) != 0x7D00) {
					p += 2;
				}
				object82_anim_data(obj2) = p + 2;
			}
			object82_energy(obj2) = ax;
			object22_damage(obj) -= dx;
			if (object22_damage(obj) <= 0) {
				obj->spr_num = 0xFFFF;
				object22_damage(obj) = 0;
				continue;
			}
		}
		obj2 = level_collide_vinyl_objects64(obj);
		if (obj2) {
			play_sound(SOUND_0);
			level_init_object102_1(obj, obj->x_pos, obj->y_pos, 200);
			level_init_object102_1(obj, obj->x_pos, obj->y_pos, 201);
			level_init_object102_1(obj, obj->x_pos, obj->y_pos, 202);
			level_init_object102_1(obj, obj->x_pos, obj->y_pos, 203);
			obj2->spr_num = 0xFFFF;
			g_vars.buffer[obj->data[0].w / 16] = 0xFFFF;
			if (object22_damage(obj) > 0x200) {
				level_init_object_spr_num_46(obj->x_pos, obj->y_pos);
			} else {
				obj->spr_num = 0xFFFF;
				continue;
			}
		} else {
			assert(g_vars.player != 2);
			if (object22_damage(obj) > 0x200) {
				level_init_object_spr_num_46(obj->x_pos, obj->y_pos);
			}
		}
		if ((g_vars.level_loop_counter & 1) == 0) {
			int num = (obj->spr_num & 0x1FFF) + 1;
			if (num > 45) { // rolling vinyl
				num = 42;
			}
			obj->spr_num = num;
			object_blinking_counter(obj) = 0;
		}
		ax = object22_xvelocity(obj);
		dx = 4;
		if (ax < 0) {
			ax = -ax;
			dx = -dx;
		}
		ax += 4;
		if (ax > 96) {
			dx = 0;
		}
		object22_xvelocity(obj) += dx;
		obj->x_pos += object22_xvelocity(obj) >> 3;
		ax = abs((obj->x_pos >> 4) - g_vars.tilemap_x - (TILEMAP_SCREEN_W / 32));
		if (ax >= (TILEMAP_SCREEN_W / 32 + 2)) {
			obj->spr_num = 0xFFFF;
		}
	}
	if (player_bounce_counter(&g_vars.players_table[0].obj) != 0) {
		--player_bounce_counter(&g_vars.players_table[0].obj);
	}
	if (player_bounce_counter(&g_vars.players_table[0].obj) != 0) {
		level_update_bouncing_tiles(&g_vars.players_table[0]);
	}
	assert(g_vars.player != 2);
	// update monsters
	for (int i = 0; i < 20; ++i) {
		struct object_t *obj = &g_vars.objects_table[82 + i];
		if (obj->spr_num != 0xFFFF) {
			const uint8_t *p = object82_anim_data(obj);
			int num;
			while (1) {
				num = (int16_t)READ_LE_UINT16(p);
				if (num >= 0) {
					break;
				}
				p += num << 1; // loop
			}
			obj->spr_num = num | ((object82_hflip(obj) & 0x80) << 8);
			object82_anim_data(obj) = p + 2;
			switch (object82_type(obj)) {
			case 0:
				level_update_object82_type0(obj);
				break;
			case 1:
				level_update_object82_type1(obj);
				break;
			case 2:
				level_update_object82_type2(obj);
				break;
			case 3:
				level_update_object82_type3(obj);
				break;
			case 4:
			case 6:
				level_update_object82_type4_6(obj);
				break;
			case 5:
				level_update_tiles(obj, obj->x_pos, obj->y_pos, _undefined);
				level_update_object82_position(obj, 0, obj->data[5].b[0]);
				break;
			case 7:
				level_update_object82_type7(obj);
				break;
			case 16:
				level_update_object82_type16(obj);
				break;
			default:
				print_warning("object %d unhandled type %d", 82 + i, object82_type(obj));
				break;
			}
		}
	}
	// collision with monsters
	for (int i = 0; i < 20; ++i) {
		struct object_t *obj = &g_vars.objects_table[82 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		struct player_t *player = &g_vars.players_table[0];
		if (object82_type(obj) == 5) { // jukebox for end of level
			if ((player_flags2(&player->obj) & 8) == 0) {
				if (level_collide_objects(obj, &g_vars.objects_table[0])) {
					do_end_of_level();
					g_vars.change_next_level_flag = 1;
					return;
				}
			}
		} else {
			if ((player_flags2(&player->obj) & 9) == 0) {
				if (object82_type(obj) == 16) { // monster ko
					continue;
				}
				if (player_hit_counter(&player->obj) == 0 && level_collide_objects(obj, &g_vars.objects_table[0])) {
					if (object82_type(obj) != 7) {
						level_player_hit_object(player, &g_vars.objects_table[0]);
					} else if (player_y_delta(&player->obj) > 0 && obj->y_pos > player->obj.y_pos) {
						player_flags2(&player->obj) |= 0x40;
					}
				}
			}
			assert(g_vars.player != 2);
		}
	}
}

static void level_update_player(struct player_t *player) {
	if (player_flags2(&player->obj) & 8) {
		return;
	}
	struct object_t *player_obj = &g_vars.objects_table[player_obj_num(&player->obj)];
	player_flags(&player->obj) &= ~1;
	player_obj->y_pos += 8;
	for (int i = 0; i < 8; ++i) { // platforms
		struct object_t *obj = &g_vars.objects_table[32 + i * 4];
		const uint16_t spr_num = player_obj->spr_num;
		player_obj->spr_num = 0;
		const bool ret = level_collide_objects(player_obj, obj);
		player_obj->spr_num = spr_num;
		if (!ret) {
			continue;
		}
		if ((obj->spr_num & 0x1FFF) == 224) { // ball with spikes
			player_x_delta(&player->obj) = (obj->x_pos <= player_obj->x_pos) ? 40 : -40;
			player_flags2(&player->obj) |= 2;
			level_player_hit_object(player, obj);
			continue;
		} else { // rotating platforms
			if (player_y_delta(&player->obj) < 0) {
				goto bonus_objects;
			}
			if (obj->y_pos < player_obj->y_pos) {
				continue;
			}
			player->obj.x_pos -= obj->data[3].w;
			player->obj.y_pos = obj->y_pos - 15;
			player_obj->y_pos = player->obj.y_pos;
			if (player_hit_counter(&player->obj) == 0) {
				player_flags2(&player->obj) &= ~2;
			}
			player_y_delta(&player->obj) = 0;
			player_flags(&player->obj) |= 1;
			player_jump_counter(&player->obj) = 7;
			goto bonus_objects;
		}
	}
	if (player_y_delta(&player->obj) >= 0) {
		for (int i = 0; i < 8; ++i) { // crates
			struct object_t *obj = &g_vars.objects_table[64 + i];
			if (obj->y_pos < player_obj->y_pos) {
				continue;
			}
			const int num = obj->spr_num;
			obj->spr_num = 0;
			const int ret = level_collide_objects(player_obj, obj);
			obj->spr_num = num;
			if (!ret) {
				continue;
			}
			const int dl = sprite_offsets[obj->spr_num & 0x1FFF] >> 8;
			if (dl > 16 && player->obj.y_pos > obj->y_pos - 16) {
				continue;
			}
			player->obj.y_pos = obj->y_pos - dl;
			player_obj->y_pos = player->obj.y_pos;
			if (player_hit_counter(&player->obj) == 0) {
				player_flags2(&player->obj) &= ~2;
			}
			player_y_delta(&player->obj) = 0;
			player_flags(&player->obj) |= 1;
			player_jump_counter(&player->obj) = 7;
			if (object64_counter(obj) != 0) {
				--object64_counter(obj);
			}
			goto bonus_objects;
		}
	}
	player_obj->y_pos -= 8;
bonus_objects:
	for (int i = 0; i < 10; ++i) { // bonuses
		struct object_t *obj = &g_vars.objects_table[72 + i];
		if (obj->spr_num == 0xFFFF || !level_collide_objects(player_obj, obj)) {
			continue;
		}
		const int spr_num = (obj->spr_num & 0x1FFF);
		if (spr_num == 194) { // cake, power up
			level_player_draw_powerup_animation(player, obj);
		} else {
			play_sound(SOUND_10);
			if (spr_num == 197) { // checkpoint
				g_vars.level_start_1p_x_pos = obj->x_pos;
				g_vars.level_start_1p_y_pos = obj->y_pos;
				g_vars.level_start_2p_player1_x_pos = obj->x_pos;
				g_vars.level_start_2p_player1_y_pos = obj->y_pos;
				g_vars.level_start_2p_player2_x_pos = obj->x_pos;
				g_vars.level_start_2p_player2_y_pos = obj->y_pos;
			} else if (spr_num == 195) { // shield
				struct object_t *player_obj = &g_vars.objects_table[player_obj_num(&player->obj)];
				object_blinking_counter(player_obj) = 240;
			} else if (spr_num == 190) {
				++player->lifes_count;
			} else if (spr_num == 193) {
				g_vars.level_time += 50;
			} else if (spr_num == 210) {
				player->vinyls_count += 20;
			} else if (spr_num == 192) {
				if (player->energy < 4) {
					++player->energy;
				}
			} else if (spr_num == 191) {
				player->energy = 4;
			} else if (spr_num == 196) {
				player->vinyls_count += 20;
			}
			obj->spr_num = 0xFFFF;
		}
	}
}

static void level_update_players() {
	if ((player_flags2(&g_vars.players_table[0].obj) & 1) == 0) {
		level_update_player(&g_vars.players_table[0]);
	}
	if (g_vars.player == 2 && (player_flags2(&g_vars.players_table[1].obj) & 1) == 0) {
		level_update_player(&g_vars.players_table[1]);
	}
}

static void init_panel(int x_offset) {
	for (int y = 0; y < PANEL_H; ++y) {
		memcpy(g_res.vga + (GAME_SCREEN_H - PANEL_H + y) * GAME_SCREEN_W + x_offset, g_res.board + y * 320, 320);
	}
	if (g_vars.player == 1) {
		video_draw_dot_pattern(x_offset);
	} else if (g_vars.player == 0) {
		video_draw_dot_pattern(x_offset + 44 * 4);
	}
}

static void draw_panel_vinyl(int offset, int spr, int x_offset) {
	const uint8_t *src = g_res.board + 0x1E00 + spr * 16;
	const int y_dst = (GAME_SCREEN_H - PANEL_H) + offset / 80;
	const int x_dst = (offset % 80) * 4 + 2 + x_offset;
	for (int y = 0; y < 16; ++y) {
		memcpy(g_res.vga + (y_dst + y) * GAME_SCREEN_W + x_dst, src, 16);
		src += 320;
	}
}

static void draw_panel_energy(int offset, int count, int x_offset) {
	const uint8_t *src = g_res.board + 0x2840 + count * 40;
	const int y_dst = (GAME_SCREEN_H - PANEL_H) + offset / 80;
	const int x_dst = (offset % 80) * 4 + x_offset;
	for (int y = 0; y < 6; ++y) {
		memcpy(g_res.vga + (y_dst + y) * GAME_SCREEN_W + x_dst, src, 40);
		src += 320;
	}
}

static void draw_panel_number(int bp, int di, int num, int x_offset) {
	int y_dst = (GAME_SCREEN_H - PANEL_H) + di / 80;
	int x_dst = (di % 80) * 4 + x_offset;
	int digits[3];
	int count = 0;
	do {
		digits[count++] = num % 10;
		num /= 10;
	} while (num != 0 && count < 3);
	if (bp == 0x1E40) { // center time
		x_dst += (3 - count) * 8 / 2;
	}
	for (; count > 0; --count) {
		const uint8_t *src = g_res.board + bp + digits[count - 1] * 8;
		for (int y = 0; y < 8; ++y) {
			memcpy(g_res.vga + (y_dst + y) * GAME_SCREEN_W + x_dst, src, 8);
			src += 320;
		}
		x_dst += 8;
	}
}

static void draw_panel() {
	const int x_offset = (GAME_SCREEN_W - 320) / 2;
	init_panel(x_offset);
	draw_panel_number(0x1E40, 0x435, MIN(g_vars.level_time, 999), x_offset);
	const int bp = 0x1E90;
	if (g_vars.player != 1) {
		draw_panel_number(bp, 0x29F, MIN(g_vars.players_table[0].lifes_count, 99), x_offset);
		draw_panel_number(bp, 0x288, g_vars.players_table[0].vinyls_count, x_offset);
		draw_panel_energy(0x41E, g_vars.players_table[0].energy, x_offset);
		const uint8_t al = player_power(&g_vars.players_table[0].obj);
		if (al > 8) {
			uint8_t cl = 0;
			if (al < 33) {
				++cl;
				if (al < 20) {
					++cl;
				}
			}
			draw_panel_vinyl(0x141, (g_vars.level_loop_counter >> cl) & 3, x_offset);
		}
	}
	if (g_vars.player != 0) {
		struct player_t *player = &g_vars.players_table[0];
		if (g_vars.player != 1) {
			player = &g_vars.players_table[1];
		}
		draw_panel_number(bp, 0x2CB, MIN(player->lifes_count, 99), x_offset);
		draw_panel_number(bp, 0x2B4, player->vinyls_count, x_offset);
		draw_panel_energy(0x44A, player->energy, x_offset);
		const uint8_t al = player_power(&player->obj);
		if (al > 8) {
			uint8_t cl = 0;
			if (al < 33) {
				++cl;
				if (al < 20) {
					++cl;
				}
			}
			draw_panel_vinyl(0x16D, (g_vars.level_loop_counter >> cl) & 3, x_offset);
		}
	}
}

static void level_draw_objects() {
	for (int i = OBJECTS_COUNT - 1; i >= 0; --i) {
		struct object_t *obj = &g_vars.objects_table[i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		const uint16_t num = obj->spr_num & 0x1FFF;
		if (object_blinking_counter(obj) != 0) {
			--object_blinking_counter(obj);
		}
		if (object_blinking_counter(obj) != 0 && (g_vars.level_loop_counter & 1) != 0) {
			// video_set_palette_index(1);
			continue;
		} else {
			// video_set_palette_index(sprite_palettes[num]);
		}
		const int spr_hflip = ((obj->spr_num & 0x8000) != 0) ? 1 : 0;
		const int spr_y_pos = obj->y_pos - ((g_vars.tilemap_y << 4) + g_vars.tilemap_scroll_dy) - (sprite_offsets[num] >> 8);
		int spr_x_pos = obj->x_pos - ((g_vars.tilemap_x << 4) + (g_vars.tilemap_scroll_dx << 2));
		int spr_w = (sprite_sizes[num] & 255);
		if (spr_hflip) {
			spr_w = (sprite_offsets[num] & 255) - spr_w;
		}
		spr_x_pos -= spr_w;
		video_draw_sprite(num, spr_x_pos, spr_y_pos, spr_hflip);
	}
}

static int16_t level_player_update_anim(struct player_t *player) {
	const uint8_t *p;
	if (player_prev_spr_num(&player->obj) != player->obj.spr_num) {
		player_idle_counter(&player->obj) = 0;
		player_prev_spr_num(&player->obj) = player->obj.spr_num;
		p = player_anim_table[player_prev_spr_num(&player->obj)];
	} else {
		++player_idle_counter(&player->obj);
		p = player_anim_data(&player->obj);
	}
	int num;
	while ((num = (int16_t)READ_LE_UINT16(p)) < 0) {
		p += num * 2;
	}
	if (num & 0x2000) {
		++player_throw_counter(&player->obj);
	}
	player_anim_data(&player->obj) = p + 2;
	return num;
}

static void level_player_update_spr(struct player_t *player, struct object_t *obj) {
	int num;
	player_throw_counter(&player->obj) = 0;
	if ((player_flags2(&player->obj) & 1) == 0) {
		if (player->obj.spr_num != 6 && player->obj.spr_num != 19 && player_y_delta(&player->obj) > 64) {
			num = abs(player_x_delta(&player->obj));
			num = (num == 32) ? 0 : 4;
			if (player_flags(&player->obj) & PLAYER_FLAGS_POWERUP) {
				num |= 2;
			}
			if (player_flags(&player->obj) & 0x40) {
				num |= 1;
			}
			static const uint8_t data[] = { 0x0B, 0x93, 0x24, 0x93, 0x0C, 0x94, 0x24, 0x94 };
			num = data[num];
		} else {
			if (player_hit_counter(&player->obj) != 0) {
				--player_hit_counter(&player->obj);
				num = 19;
			} else {
				num = level_player_update_anim(player);
			}
		}
	} else {
		num = level_player_update_anim(player);
	}
	const int _dx = num & 0x4000;
	num &= 0x1FFF;
	num |= (player_flags(&player->obj) & PLAYER_FLAGS_FACING_LEFT) << 13;
	if (_dx) {
		num ^= 0x8000;
	}
	obj->spr_num = num;
}

// crates
static void level_tile_func_5dc9(struct object_t *obj, int bp) {
	if (bp != _undefined && (player_flags2(obj) & 0x40) != 0) {
		level_init_object_spr_num_92(obj->x_pos, obj->y_pos);
		play_sound(SOUND_4);
		player_flags2(obj) &= ~0x40;
	}
	g_vars.player_xscroll = 0;
	obj->x_pos -= player_x_delta(obj) >> 3;
	player_x_delta(obj) = 0;
	if (bp == _undefined) {
		return;
	}
	const int offset = g_vars.player_map_offset;
	uint8_t al;
	al = g_vars.tilemap_data[offset - g_vars.tilemap_w];
	al = g_vars.level_tiles_lut[al];
	al = tiles_5dc9_lut[al];
	if (al == 0) {
		return;
	}
	al = g_vars.tilemap_data[offset - g_vars.tilemap_w - 1];
	al = g_vars.level_tiles_lut[al];
	al = tiles_5dc9_lut[al];
	if (al == 0) {
		obj->x_pos -= 2;
		return;
	}
	al = g_vars.tilemap_data[offset - g_vars.tilemap_w + 1];
	al = g_vars.level_tiles_lut[al];
	al = tiles_5dc9_lut[al];
	if (al != 0) {
		if ((player_flags(obj) & 0x10) == 0) {
			return;
		}
		level_player_die((struct player_t *)obj);
	}
	obj->x_pos += 2;
}

static void level_tile_func_5daa(struct object_t *obj, int bp) {
	if (bp != _undefined && (player_flags2(obj) & 0x40) != 0) {
		return;
	}
	player_y_delta(obj) = MIN(player_y_delta(obj) + 7, 120);
}

static void level_tile_func_5e58(struct object_t *obj, int bp) {
	if (bp != _undefined) {
		player_flags2(obj) &= ~0x40;
		level_init_object_spr_num_92(obj->x_pos, obj->y_pos - 24);
		play_sound(SOUND_4);
	}
	player_y_delta(obj) = 0;
	if (player_flags(obj) & PLAYER_FLAGS_STAIRS) {
		obj->y_pos += 2;
	}
}

static void level_tile_func_5e83(struct object_t *obj, int bp) {
	if (bp != _undefined) {
		player_flags2(obj) &= ~0x40;
	}
	if (player_y_delta(obj) < 0) {
		level_tile_func_5daa(obj, bp);
	} else {
		player_flags2(obj) &= ~2;
		player_y_delta(obj) = 0;
		obj->y_pos &= ~15;
		if (bp == _undefined) { // (obj < &g_vars.players_table[0])
			return;
		}
		if (player_jump_counter(obj) != 7) {
			player_jump_counter(obj) = 7;
			player_x_delta(obj) >>= 1;
		}
	}
}

// stairs
static void level_tile_func_5d3e(struct object_t *obj, int bp) {
	if (bp == _undefined) {
		level_tile_func_5daa(obj, bp);
		return;
	}
	player_flags2(obj) &= ~2;
	if (player_flags2(obj) & 0x40) {
		return;
	}
	if (player_hit_counter(obj) != 0) {
		level_tile_func_5daa(obj, bp);
		return;
	}
	if (player_x_delta(obj) != 0 && player_y_delta(obj) < 0) {
		level_tile_func_5daa(obj, bp);
		return;
	}
	const int offset = g_vars.player_map_offset;
	uint8_t ah = (player_flags(obj) & 0x40) | (player_flags2(obj) & 1);
	uint8_t al = g_vars.tilemap_data[offset - g_vars.tilemap_w];
	al = g_vars.level_tiles_lut[al];
	if (al == 8) {
		if (ah != 0) {
			level_tile_func_5daa(obj, bp);
			return;
		}
	} else {
		player_y_delta(obj) = 0;
		if (ah != 0 || (player_flags2(obj) & 4) == 0) {
			level_tile_func_5e83(obj, bp);
			return;
		}
	}
	player_y_delta(obj) = 0;
	player_flags(obj) |= PLAYER_FLAGS_STAIRS;
}

// bouncing mushrooms
static void level_tile_func_5f16(struct object_t *obj, int bp) {
	if (player_y_delta(obj) < 0) {
		return;
	} else if (player_y_delta(obj) == 0) {
		level_tile_func_5e83(obj, bp);
	} else {
		if (obj != &g_vars.players_table[0].obj) {
			print_warning("Unexpected object #%d spr %d on tile 5f16", obj - g_vars.objects_table, obj->spr_num);
			return;
		}
		player_flags2(obj) &= ~2;
		obj->y_pos &= ~15;
		player_y_delta(obj) = MAX(-player_y_delta(obj) - 16, -112);
		player_jump_counter(obj) = 0;
		player_bounce_counter(obj) = 7;
		player_tilemap_offset(obj) = g_vars.player_map_offset;
		const int x = (obj->x_pos >> 4) - g_vars.tilemap_x;
		if (x < 0 || x > TILEMAP_SCREEN_W / 16) {
			return;
		}
		const int y = (obj->y_pos >> 4) - g_vars.tilemap_y;
		if (y < 0 || y > (TILEMAP_SCREEN_H / 16) + 1) {
			return;
		}
		play_sound(SOUND_2);
	}
}

static void level_tile_func_5eec(struct object_t *obj, int bp) {
	assert(bp != _undefined);
	struct player_t *player = (struct player_t *)obj;
	const int offset = g_vars.player_map_offset;
	g_vars.tilemap_data[offset] = 0;
	++player->vinyls_count;
	level_init_object_spr_num_104(offset % g_vars.tilemap_w, offset / g_vars.tilemap_w);
	play_sound(SOUND_5);
	level_tile_func_5daa(obj, bp);
}

static void level_tile_func_5f0e(struct object_t *obj, int bp) {
	level_tile_func_5f16(obj, bp);
	obj->data[7].b[0] = 0;
}

// slopes
static void level_tile_func_5f7b(struct object_t *obj, int bp, int bx) {
	if (bp != _undefined) {
		player_flags2(obj) &= ~0x40;
	}
	if (player_y_delta(obj) < 0) {
		return;
	}
	player_flags2(obj) &= ~2;
	player_y_delta(obj) = 0;
	obj->y_pos &= ~15;
	if (bp == _undefined) { // (obj < &g_vars.players_table[0])
		return;
	}
	struct player_t *player = (struct player_t *)obj;
	if (player_jump_counter(&player->obj) != 7) {
		player_jump_counter(&player->obj) = 7;
		player_x_delta(&player->obj) >>= 1;
	}
	assert(bx >= 18);
	const uint8_t *p = tiles_yoffset_table + ((bx - 18) << 3);
	player->obj.y_pos += p[player->obj.x_pos & 15];
}

// electricity
static void level_tile_func_5f94(struct object_t *obj) {
	if (g_options.cheats & CHEATS_DECOR_NO_HIT) {
		return;
	}
	level_player_die((struct player_t *)obj);
}

// spikes
static void level_tile_func_5f9e(struct object_t *obj, int bp) {
	level_player_hit_object(&g_vars.players_table[0], &g_vars.objects_table[0]);
}

static void level_tile_func_5fa7(struct object_t *obj, int bp) {
	level_player_hit_object(&g_vars.players_table[0], &g_vars.objects_table[0]);
	level_tile_func_5e83(obj, bp);
}

static void level_tile_func(int ax, struct object_t *obj, int bp, int bx) {
	switch (ax) {
	case 0x5D3E:
		level_tile_func_5d3e(obj, bp);
		break;
	case 0x5DC8:
		break;
	case 0x5DC9:
		level_tile_func_5dc9(obj, bp);
		break;
	case 0x5DAA:
		level_tile_func_5daa(obj, bp);
		break;
	case 0x5E58:
		level_tile_func_5e58(obj, bp);
		break;
	case 0x5E83:
		level_tile_func_5e83(obj, bp);
		break;
	case 0x5EEC:
		level_tile_func_5eec(obj, bp);
		break;
	case 0x5F0E:
		level_tile_func_5f0e(obj, bp);
		break;
	case 0x5F16:
		level_tile_func_5f16(obj, bp);
		break;
	case 0x5F73:
		level_tile_func_5f7b(obj, bp, 18);
		break;
	case 0x5F78:
		level_tile_func_5f7b(obj, bp, 20);
		break;
	case 0x5F7B:
		level_tile_func_5f7b(obj, bp, bx);
		break;
	case 0x5F94:
		level_tile_func_5f94(obj);
		break;
	case 0x5F9E:
		level_tile_func_5f9e(obj, bp);
		break;
	case 0x5FA7:
		level_tile_func_5fa7(obj, bp);
		break;
	default:
		print_warning("Unimplemented tile func %x", ax);
		break;
	}
}

static void level_update_tiles(struct object_t *obj, int ax, int dx, int bp) {
	const int level_update_tiles_x = ax;
	const int level_update_tiles_y = dx;
	g_vars.player_map_offset = (dx >> 4) * g_vars.tilemap_w + (ax >> 4);
	uint8_t _al = 0;
	while (1) {
		const int offset = (dx >> 4) * g_vars.tilemap_w + (ax >> 4);
		_al = level_get_tile(offset);
		if (_al != 5) {
			break;
		}
		obj->y_pos -= 16;
		dx = obj->y_pos;
	}
	const int num = _al << 1;
	ax = 0x5DAA;
	if (obj->y_pos > 0) {
		if (bp == _undefined) {
			ax = tile_funcs0[_al];
		} else {
			ax = tile_funcs1[_al]; // player
		}
		if (ax == 0 && (obj->data[6].b[0] & 1) != 0) {
			return;
		}
	}
	level_tile_func(ax, obj, bp, num);
	if (bp == _undefined) {
		return; // not player
	}
	ax = level_update_tiles_x + ((player_x_delta(obj) < 0) ? -6 : 6);
	dx = level_update_tiles_y;
	const int offset = (dx >> 4) * g_vars.tilemap_w + (ax >> 4);
	_al = level_get_tile(offset - g_vars.tilemap_w);
	level_tile_func(tile_funcs2[_al], obj, bp, _al * 2);
	if (obj->spr_num != 3) {
		_al = level_get_tile(offset - g_vars.tilemap_w * 2);
		level_tile_func(tile_funcs2[_al], obj, bp, _al * 2);
	}
	player_flags(obj) &= ~0x10;
	player_flags2(obj) &= ~0x20;
	if (player_jump_counter(obj) == 7) {
		_al = level_get_tile(offset - g_vars.tilemap_w * 2);
		if (_al == 10) {
			player_flags2(obj) |= 0x20;
		}
		if (tile_funcs3[_al] == 0x5E58) {
			player_flags(obj) |= 0x10;
			player_flags2(obj) &= ~1;
		}
	}
	if ((player_flags(obj) & PLAYER_FLAGS_STAIRS) == 0 && player_y_delta(obj) >= 0) {
		return;
	}
	_al = level_get_tile(offset - g_vars.tilemap_w * 2);
	level_tile_func(tile_funcs3[_al], obj, bp, _al * 2);
}

static bool level_adjust_player_spr_num_helper(struct player_t *player, int ax, int dx) {
	if ((player_flags2(&player->obj) & 1) == 0) {
		return false;
	}
	if (player->change_hdir_counter < 30) {
		const int offset = (dx >> 4) * g_vars.tilemap_w + (ax >> 4);
		const uint8_t num = level_get_tile(offset - g_vars.tilemap_w);
		if (tile_funcs3[num] != 0x5E58) {
			return true;
		}
	}
	level_hit_player(player);
	return true;
}

static void level_adjust_player_spr_num(struct player_t *player) {
	struct object_t *obj = &g_vars.objects_table[player_obj_num(&player->obj)];
	obj->x_pos = player->obj.x_pos;
	obj->y_pos = player->obj.y_pos;
	if (player_flags2(&player->obj) & 8) {
		obj->spr_num = 19;
	} else {
		level_player_update_spr(player, obj);
		const int tile_x = (player->obj.x_pos >> 4);
		const int tile_y = (player->obj.y_pos >> 4);
		int offset = tile_y * g_vars.tilemap_w + tile_x;
		const int spr_h = sprite_offsets[player->obj.spr_num & 0x1FFF] >> 8;
		for (int i = 0; i < (spr_h >> 4); ++i) {
			offset -= g_vars.tilemap_w;
			if (offset < 0) {
				break;
			}
			uint8_t _al = g_vars.tilemap_data[offset];
			_al = g_vars.level_tiles_lut[_al];
			if (_al == 6) {
				g_vars.tilemap_data[offset] = 0;
				++player->vinyls_count;
				level_init_object_spr_num_104(tile_x, tile_y - i - 1);
				play_sound(SOUND_5);
			}
		}
		player_flags(&player->obj) &= ~PLAYER_FLAGS_STAIRS;
		if (!level_adjust_player_spr_num_helper(player, player->obj.x_pos, player->obj.y_pos)) {
			level_update_tiles(&player->obj, player->obj.x_pos, player->obj.y_pos, 0);
		}
	}
	obj->y_pos = player->obj.y_pos;
	const int spr_num = obj->spr_num & 0x1FFF;
	if ((player_flags(&player->obj) & 0x60) == 0x60) {
		obj->spr_num += (spr_num <= 148) ? 13 : 12;
	}
	if ((player_flags(&player->obj) & PLAYER_FLAGS_JAKE) != 0) {
		int d = (spr_num < 50) ? 50 : 25;
		if (spr_num == 138) {
			d = 1;
		}
		obj->spr_num += d;
	}
}

static void level_adjust_player_position() {
	level_adjust_player_spr_num(&g_vars.players_table[0]);
	assert(g_vars.player < 2);
}

static void level_sync() {
	++g_vars.level_time_counter;
	if (g_vars.level_time_counter >= 28) {
		g_vars.level_time_counter = 0;
		if ((g_options.cheats & CHEATS_UNLIMITED_TIME) == 0) {
			--g_vars.level_time;
		}
	}
	g_sys.update_screen(g_res.vga, 1);
	g_sys.render_clear_sprites();
	const int diff = (g_vars.timestamp + (1000 / 30)) - g_sys.get_timestamp();
	g_sys.sleep(MAX(diff, 10));
	g_vars.timestamp = g_sys.get_timestamp();
}

static void load_level_data(uint16_t level) {
	g_vars.level_num = (level >> 8);
	const uint8_t triggers_count = (level & 255);
	g_vars.level_tiles_lut = (level & 0xFF00) + level_data_tiles_lut;
	load_file(_background[g_vars.level_num]);
	video_copy_vga(0xB500);
	load_file(_decor[g_vars.level_num]);
	const uint8_t *buffer = g_res.tmp;
	memcpy(g_vars.tile_palette_table, buffer + 0x8000, 0x100);
	int bp = 0;
	const uint8_t *data = buffer + 0x8100;
	for (int i = 0; i < triggers_count; ++i) {
		const int num = data[18] * 16;
		data += num + 24;
		const int w = (int16_t)READ_LE_UINT16(data - 3);
		const int h = (int16_t)READ_LE_UINT16(data - 5);
		print_debug(DBG_GAME, "level len:%d w:%d h:%d", num, w, h);
		bp += w * h;
	}
	memcpy(g_vars.triggers_table, data, 19); data += 19;
	uint8_t *dst = g_vars.triggers_table + 19;
	int count = data[-1]; // data[18]
	print_debug(DBG_GAME, "level 0x%x count %d", level, count);
	assert(count <= TRIGGERS_COUNT);
	const int stride = count * sizeof(int16_t);
	for (int i = 0; i < count; ++i, data += 2) {
		for (int i = 0; i < 8; ++i) {
			memcpy(dst, data + stride * i, sizeof(int16_t));
			dst += 2;
		}
	}
	data += 7 * stride;
	if (g_debug_mask & DBG_GAME) {
		for (int i = 0; i < g_vars.triggers_table[18]; ++i) {
			const uint8_t *obj_data = g_vars.triggers_table + 19 + i * 16;
			const int x_pos = READ_LE_UINT16(obj_data);
			const int y_pos = READ_LE_UINT16(obj_data + 2);
			const int type1 = READ_LE_UINT16(obj_data + 4);
			const int type2 = READ_LE_UINT16(obj_data + 6);
			const int num   = READ_LE_UINT16(obj_data + 8);
			print_debug(DBG_GAME, "trigger #%d pos %d,%d type %d,%d num %d", i, x_pos, y_pos, type1, type2, num);
		}
	}
	g_vars.tilemap_h = READ_LE_UINT16(data); data += 2;
	g_vars.tilemap_w = READ_LE_UINT16(data); data += 2;
	g_vars.tilemap_type = *data++;
	print_debug(DBG_GAME, "tilemap w:%d h:%d type:%d", g_vars.tilemap_w, g_vars.tilemap_h, g_vars.tilemap_type);
	data = buffer + 0x8100;
	int offset = 0;
	count = level_data3[g_vars.level_num];
	print_debug(DBG_GAME, "level_data[3] %d bp %d", count, bp);
	for (int i = 0; i < count; ++i) {
		const int num = data[offset + 18] * 16;
		offset += num + 24;
	}
	offset += bp;
	assert(offset >= g_vars.tilemap_h);
	g_vars.tilemap_data = g_res.tmp + 0x8100 + offset;
	for (int i = 0; i < 0x600; ++i) {
		g_vars.tilemap_lut_init[i] = (i & 0xFF);
	}
	g_vars.tilemap_current_lut = g_vars.tilemap_lut_init;
	const uint8_t *tiles_lut = 0;
	switch (g_vars.tilemap_type & 0x3F) {
	case 0: tiles_lut = tile_lut_data0; break;
	case 1: tiles_lut = tile_lut_data1; break;
	case 2: tiles_lut = tile_lut_data2; break;
	case 3: tiles_lut = tile_lut_data3; break;
	default:
		print_error("load_level_data(): Invalid tilemap type %d", g_vars.tilemap_type);
		break;
	}
	for (int i = 0; i < tiles_lut[0]; ++i) {
		const uint8_t start = tiles_lut[i * 3 + 1];
		const uint8_t count = tiles_lut[i * 3 + 2];
		const uint8_t value = tiles_lut[i * 3 + 3];
		print_debug(DBG_GAME, "tiles start:%d count:%d value:0x%x", start, count, value);
		memset(g_vars.tilemap_lut_init2 + start, value, count);
	}
	const uint8_t *src = tiles_lut;
	int total_count = src[0]; // total_count
	while (1) {
		const uint8_t start = src[1];
		const uint8_t count = src[2];
		src += 3;
		--total_count;
		if (total_count < 0) {
			break;
		}
		uint8_t bh = 0;
		do {
			uint8_t ch = 0;
			do {
				assert(bh < 6);
				uint8_t dh = ch;
				for (int i = 0; i < count; ++i) {
					g_vars.tilemap_lut_init[(bh << 8) | (start + i)] = start + dh;
					++dh;
					if (dh == count) {
						dh = 0;
					}
				}
				++bh;
				++ch;
				if (bh == 6) {
					break;
				}
			} while (bh != count);
		} while (bh == 3);
	}
	const uint8_t *p = g_vars.tilemap_lut_init;
	for (int i = 0; i < 256; ++i) {
		const uint8_t num = p[i];
		if (p[0x100 + i] != num) {
			if (p[0x300 + i] == num) {
				g_vars.tilemap_lut_type[i] = 2;
			} else {
				g_vars.tilemap_lut_type[i] = 1;
			}
		} else {
			g_vars.tilemap_lut_type[i] = 0;
		}
	}
	g_vars.tilemap_lut_type[0xF8] = 1;
	g_vars.tilemap_end_x = g_vars.tilemap_w - (TILEMAP_SCREEN_W / 16 + 1);
	g_vars.tilemap_end_y = g_vars.tilemap_h - (TILEMAP_SCREEN_H / 16 + 1);
	g_vars.players_table[0].vinyls_count = READ_LE_UINT16(g_vars.triggers_table + 0xC);
	g_vars.players_table[1].vinyls_count = READ_LE_UINT16(g_vars.triggers_table + 0xE);
	g_vars.level_time = g_vars.level_time2 = READ_LE_UINT16(g_vars.triggers_table + 0x10);
	g_vars.level_time_counter = 0;
	const uint16_t num = (g_vars.level_num << 8) | triggers_count;
	if (num != g_vars.level_pos_num) {
		g_vars.level_start_2p_player1_x_pos = READ_LE_UINT16(g_vars.triggers_table);
		g_vars.level_start_2p_player1_y_pos = READ_LE_UINT16(g_vars.triggers_table + 0x4);
		g_vars.level_start_2p_player2_x_pos = READ_LE_UINT16(g_vars.triggers_table + 0x2);
		g_vars.level_start_2p_player2_y_pos = READ_LE_UINT16(g_vars.triggers_table + 0x6);
		g_vars.level_start_1p_x_pos = READ_LE_UINT16(g_vars.triggers_table + 0x8);
		g_vars.level_start_1p_y_pos = READ_LE_UINT16(g_vars.triggers_table + 0xA);
	}
	g_vars.level_pos_num = num;
	g_vars.level_loop_counter = 0;
}

static void reset_level_data() {
	memset(g_vars.objects_table, 0xFF, sizeof(g_vars.objects_table));
	g_vars.change_next_level_flag = 0;
	g_vars.player_xscroll = 0;
	g_vars.player_map_offset = 0;
	g_vars.throw_vinyl_counter = 0;
	g_vars.tilemap_flags = 0;
	memset(&g_vars.objects_table[32], 0xFF, 40 * sizeof(struct object_t));
	g_vars.tilemap_flags |= g_vars.tilemap_type >> 5;
	player_flags(&g_vars.players_table[0].obj) = 0;
	player_prev_spr_num(&g_vars.players_table[0].obj) = 0;
	player_flags(&g_vars.players_table[1].obj) = 0;
	player_prev_spr_num(&g_vars.players_table[1].obj) = 0;
}

static void level_init_players() {
	for (int i = 0; i < 128; ++i) {
		g_vars.buffer[i]       = 0xFFFF;
		g_vars.buffer[128 + i] = 0xFF20;
	}
	object_blinking_counter(&g_vars.objects_table[0]) = 0;
	object_blinking_counter(&g_vars.objects_table[1]) = 0;
	assert(g_vars.player != 2);
	struct player_t *player = &g_vars.players_table[0];
	player->obj.x_pos = g_vars.level_start_1p_x_pos;
	player->obj.y_pos = g_vars.level_start_1p_y_pos;
	player_prev_spr_num(&player->obj) = _undefined;
	player_obj_num(&player->obj) = 0;
	player_jump_counter(&player->obj) = 7;
	player_x_delta(&player->obj) = 0;
	player_y_delta(&player->obj) = 0;
	player->unk_counter = 0;
	player->change_hdir_counter = 0;
	player_bounce_counter(&player->obj) = 0;
	player_hit_counter(&player->obj) = 0;
	player_flags2(&player->obj) = 0;
	const uint8_t flag = ((g_vars.player & 1) != 0) ? PLAYER_FLAGS_JAKE : 0;
	player_flags(&player->obj) |= flag;
	player_flags2(&player->obj) = flag;
	player->energy = 4;
	g_vars.objects_table[1].spr_num = 0xFFFF;
}

static void level_init_tilemap() {
	g_vars.tilemap_x = 0;
	g_vars.tilemap_y = 0;
	g_vars.tilemap_scroll_dx = 0;
	g_vars.tilemap_scroll_dy = 0;
	while (1) {
		const int bottom_y = g_vars.tilemap_y + 11;
		const int player_y = g_vars.players_table[0].obj.y_pos >> 4;
		if (bottom_y > player_y) {
			break;
		}
		const int current_y = g_vars.tilemap_y;
		level_adjust_vscroll_down(16);
		if (g_vars.tilemap_y == current_y) {
			break;
		}
	}
	level_adjust_vscroll_down(16);
	level_adjust_vscroll_down(16);
	int prev_x = _undefined;
	while (1) {
		const int right_x = g_vars.tilemap_x + 10;
		const int player_x = g_vars.players_table[0].obj.x_pos >> 4;
		if (right_x >= player_x || prev_x == right_x) {
			break;
		}
		level_adjust_hscroll_right(4);
		prev_x = right_x;
	}
}

static void level_player_draw_powerdown_animation(struct player_t *player) {
	if (player_flags(&player->obj) & PLAYER_FLAGS_POWERUP) {
		player_flags(&player->obj) &= ~PLAYER_FLAGS_POWERUP;
		struct object_t *player_obj = &g_vars.objects_table[player_obj_num(&player->obj)];
		player_jump_counter(player_obj) = 0;
		static const uint8_t data[] = { // death animation sprites
			0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x79, 0x7B, 0x7B, 0x7B,
			0x7B, 0x7B, 0x7B, 0x7A, 0x7A, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x7B,
			0x7B, 0x7B, 0x7D, 0x7D, 0x7D, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x7D,
			0x7D, 0x7D, 0x7C, 0x8A, 0xFF
		};
		for (int i = 0; data[i] != 0xFF; ++i) {
			level_draw_tilemap();
			level_draw_objects();
			level_sync();
			player_obj->spr_num = data[i];
			if (player_flags(&player->obj) & PLAYER_FLAGS_JAKE) {
				player_obj->spr_num += 11;
			}
		}
	}
}

static void level_player_draw_powerup_animation(struct player_t *player, struct object_t *obj) {
	if ((player_flags(&player->obj) & PLAYER_FLAGS_POWERUP) == 0) {
		play_sound(SOUND_10);
		if (player_flags(&player->obj) & 0x40) {
			player_flags(&player->obj) &= ~0x40;
		}
		player_flags(&player->obj) |= PLAYER_FLAGS_POWERUP;
		obj->spr_num = 0xFFFF;
		static const uint8_t data[] = { // power animation sprites
			0x7D, 0x7D, 0x7D, 0x7D, 0x7D, 0x74, 0x75, 0x74, 0x75, 0x7C, 0x7C,
			0x7C, 0x7C, 0x75, 0x75, 0x76, 0x75, 0x76, 0x7B, 0x7B, 0x7B, 0x7B,
			0x76, 0x76, 0x77, 0x76, 0x77, 0x7A, 0x7A, 0x7A, 0x77, 0x77, 0x77,
			0x78, 0x77, 0x78, 0x7E, 0x78, 0x78, 0x78, 0x78, 0x79, 0x77, 0x79,
			0x79, 0xFF
		};
		struct object_t *player_obj = &g_vars.objects_table[player_obj_num(&player->obj)];
		for (int i = 0; data[i] != 0xFF; ++i) {
			level_draw_tilemap();
			level_draw_objects();
			level_sync();
			player_obj->spr_num = data[i];
			if (player_flags(&player->obj) & PLAYER_FLAGS_JAKE) {
				player_obj->spr_num += 11;
			}
		}
	}
}

static void init_level(uint16_t level) {
	load_level_data(level);
	reset_level_data();
	level_init_players();
	level_init_tilemap();
	level_draw_tilemap();
	g_vars.reset_palette_flag = 0;
}

static uint16_t get_level() {
	const uint8_t *data = (g_vars.player == 2) ? level_data2p : level_data1p;
	const uint16_t level = READ_BE_UINT16(data + (g_vars.level - 1) * 2);
	return level;
}

static bool change_level(bool first_level) {
	if (first_level || ((g_vars.level & 7) == 0 && (g_vars.level >> 3) < 4)) {
		do_difficulty_screen();
	}
	if ((g_vars.level & 3) == 0 && g_vars.level < LEVELS_COUNT) {
		// do_level_password_screen();
	}
	const uint16_t level = get_level();
	if (level == 0xFFFF) {
		do_game_win_screen();
		return false;
	} else {
		do_level_number_screen();
		const int num = ((g_vars.level >> 3) & 3) + 1;
		play_music(num);
		init_level(level);
		return true;
	}
}

void do_level() {
	change_level(true);
	while (!g_sys.input.quit) {
		g_vars.timestamp = g_sys.get_timestamp();
		update_input();
		level_update_palette();
		level_update_input();
		level_update_triggers();
		if (g_vars.change_next_level_flag) {
			++g_vars.level;
			if (change_level(false)) {
				continue;
			}
			break;
		}
		level_update_players();
		level_draw_tilemap();
		draw_panel();
		level_reset_palette();
		level_draw_objects();
		level_adjust_player_position();
		level_sync();
		++g_vars.level_loop_counter;
		assert(g_vars.player != 2);
		if ((player_flags2(&g_vars.players_table[0].obj) & 0x10) == 0) {
			continue;
		}
		// player fell or no energy left
		g_sys.fade_out_palette();
		assert(g_vars.player != 2);
		if (g_vars.players_table[0].lifes_count != 0) {
			init_level(get_level()); // restart
			continue;
		}
		do_game_over_screen();
		break;
	}
}

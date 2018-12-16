
/* level main loop */

#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

static const uint16_t _copper_data[18 * MAX_LEVELS] = {
	0x190,0x00a,0x00b,0x00c,0x00d,0x00e,0x00f,0x01f,0x02f,0x03f,0x04f,0x05f,0x06f,0x07f,0x08f,0x09f,0x0af,0x0bf,
	0x190,0x35c,0x15c,0x25c,0x35c,0x45c,0x55c,0x65b,0x75a,0x859,0x958,0xa57,0xb56,0xc55,0xd54,0xe53,0xf54,0xf64,
	0x198,0x900,0xa00,0xb00,0xc00,0xd00,0xe00,0xf00,0xf10,0xf20,0xf30,0xf40,0xf50,0xf60,0xf70,0xf80,0xf90,0xfa0,
	0x190,0x303,0x403,0x503,0x603,0x703,0x803,0x903,0xa03,0xb03,0xc03,0xd03,0xe03,0xf03,0xf04,0xf05,0xf06,0xf07,
	0x19a,0x003,0x004,0x005,0x006,0x007,0x008,0x009,0x00a,0x00b,0x00c,0x00d,0x00e,0x00f,0x10f,0x20f,0x30f,0x40f,
	0x198,0x400,0x500,0x600,0x700,0x800,0x900,0xa00,0xb00,0xc00,0xd00,0xe00,0xf00,0xf11,0xf22,0xf33,0xf44,0xf55
};

static const uint16_t _colors_data[16 * MAX_LEVELS] = {
	0x000,0xc77,0x989,0x669,0x147,0xfda,0xdb9,0xf87,0x4af,0x050,0x091,0x111,0xbcf,0xa20,0x630,0xfff,
	0x000,0xa67,0xb89,0x067,0x046,0xeb8,0xca5,0xf87,0x4af,0x660,0x980,0x111,0x59c,0xa25,0x635,0xfff,
	0x000,0x744,0x989,0x669,0x147,0xfda,0xdb9,0xf87,0x3ae,0x0a2,0x0d2,0x000,0xbcf,0xf30,0x950,0xfff,
	0x000,0x885,0x773,0x267,0x147,0xca8,0x75e,0xb76,0x000,0x996,0x664,0x111,0x546,0x553,0x289,0xbcf,
	0x000,0xa9a,0x753,0x868,0x407,0xfaa,0xbac,0xd67,0x38c,0xd96,0x964,0x000,0xbcd,0x000,0x705,0xfff,
	0x000,0x30b,0x747,0x446,0x006,0xfda,0x400,0xf87,0x37a,0x600,0xd63,0x000,0x000,0x800,0xdd0,0x98d
};

static const struct {
	const char *ck1;
	const char *ck2;
	const char *sql;
	const char *bin;
	const char *avt;
	const char *sqv;
	const int16_t *xpos;
	const int16_t *ypos;
	uint8_t music;
	uint8_t cga_dither_ck;
	uint8_t cga_dither_sqv;
	uint8_t cga_dither_avt;
} _levels[MAX_LEVELS] = {
	{ "mag.ck1", "mag.ck2", "mag.sql", "magasin.bin", "avtmag.sqv", "enemi1.sqv", level_xpos_magasin, level_ypos_magasin, 1, 3, 4, 3 },
	{ "ent.ck1", "ent.ck2", "ent.sql", "entrepot.bin", "avtent.sqv", "enemi2.sqv", level_xpos_ent, level_ypos_ent, 2, 3, 4, 3 },
	{ "prison.ck1", "prison.ck2", "prison.sql", "prison.bin", "avtpris.sqv", "enemi3.sqv", level_xpos_prison, level_ypos_prison, 3, 5, 6, 5 },
	{ "egou.ck1", "egou.ck2", "egou.sql", "egou.bin", "avtegou.sqv", "enemi4.sqv", level_xpos_egou, level_ypos_egou, 1, 3, 4, 3 },
	{ "ville.ck1", "ville.ck2", "ville.sql", "ville.bin", "avtville.sqv", "enemi5.sqv", level_xpos_ville, level_ypos_ville, 2, 7, 4, 7 },
	{ "concert.ck1", "concert.ck2", "concert.sql", "concert.bin", "", "enemi6.sqv", level_xpos_concert, level_ypos_concert, 3, 8, 4, 0 },
};

static const struct {
	const char *blk;
	const char *tbl;
	const char *m;
	const char *bin;
	const char *ennemi;
} _levels_amiga[MAX_LEVELS] = {
	{ "mag.blk", "mag.tbl", "mag.m", "magasin.bin", "ennemi1" },
	{ "ent.blk", "ent.tbl", "ent.m", "entrepo.bin", "ennemi2" },
	{ "prison.blk", "prison.tbl", "pris.m", "prison.bin", "ennemi3" },
	{ "egout.blk", "egout.tbl", "egou.m", "egout.bin", "ennemi4" },
	{ "ville.blk", "ville.tbl", "ville.m", "ville.bin", "ennemi5" },
	{ "concert.blk", "concert.tbl", 0, "concert.bin", "ennemi6" },
};

void load_level_data(int num) {
	print_debug(DBG_GAME, "load_level_data num %d", num);
	if (g_res.amiga_data) {
		load_blk(_levels_amiga[num].blk);
		read_file(_levels_amiga[num].tbl, g_res.sql, 0);
		load_bin(_levels_amiga[num].bin);
		load_m(_levels_amiga[num].m);
		// .avt
		load_spr(_levels_amiga[num].ennemi, g_res.tmp, SPRITES_COUNT);
	} else {
		const int dither_pattern = g_options.cga_colors ? _levels[num].cga_dither_ck : -1;
		if (num == 0 && g_res.dos_demo) {
			load_ck("demomag.ck1", 0x6000, dither_pattern);
			load_ck("demomag.ck2", 0x8000, dither_pattern);
			load_sql("demomag.sql");
		} else {
			load_ck(_levels[num].ck1, 0x6000, dither_pattern);
			load_ck(_levels[num].ck2, 0x8000, dither_pattern);
			load_sql(_levels[num].sql);
		}
		load_bin(_levels[num].bin);
		load_avt(_levels[num].avt, g_res.avt_sqv, 0, g_options.cga_colors ? _levels[num].cga_dither_avt : -1);
		load_sqv(_levels[num].sqv, g_res.tmp, SPRITES_COUNT, g_options.cga_colors ? _levels[num].cga_dither_ck : -1);
	}
	memcpy(g_vars.level_xpos, _levels[num].xpos, MAX_OBJECTS * sizeof(int16_t));
	memcpy(g_vars.level_ypos, _levels[num].ypos, MAX_OBJECTS * sizeof(int16_t));
	if (g_vars.music_num != _levels[num].music) {
		play_music(_levels[num].music);
		g_vars.music_num = _levels[num].music;
	}
	if (g_options.cga_colors) {
	} else if (g_options.amiga_colors) {
		g_sys.set_palette_amiga(_colors_data + g_vars.level * 16, 0);
	} else {
		g_sys.set_screen_palette(g_res.palette, 0, 16, 8);
	}
	screen_load_graphics(g_options.cga_colors ? g_res.cga_lut_sqv : 0, g_options.cga_colors ? g_res.cga_lut_avt : 0);
}

static void init_level() {
	static const uint16_t restart_xpos[] = { 720, 720, 912, 912, 336, 352, 288, 304, 960, 976,  64,  80 };
	static const uint16_t restart_ypos[] = { 496, 496, 752, 752, 816, 816, 304, 304, 592, 592, 352, 352 };
	if (g_vars.play_demo_flag) {
		g_vars.objects[OBJECT_NUM_PLAYER1].unk60 = 0;
	}
	int xpos = g_options.start_xpos16;
	if (xpos < 0) {
		xpos = g_vars.level_xpos[OBJECT_NUM_PLAYER1];
		if (!g_vars.two_players_flag && (g_vars.objects[OBJECT_NUM_PLAYER1].unk60 || g_vars.objects[OBJECT_NUM_PLAYER2].unk60)) {
			xpos = restart_xpos[g_vars.level * 2];
		}
		xpos = (xpos >> 4) - 10;
	} else {
		g_vars.level_xpos[OBJECT_NUM_PLAYER1] = (xpos << 4) + 10;
	}
	if (xpos < 0) {
		xpos = 0;
	}
	g_vars.screen_tilemap_xorigin = xpos << 4;

	int ypos = g_options.start_ypos16;
	if (ypos < 0) {
		ypos = g_vars.level_ypos[OBJECT_NUM_PLAYER1];
		if (!g_vars.two_players_flag && (g_vars.objects[OBJECT_NUM_PLAYER1].unk60 || g_vars.objects[OBJECT_NUM_PLAYER2].unk60)) {
			ypos = restart_ypos[g_vars.level * 2];
		}
		ypos = (ypos >> 4) - 6;
	} else {
		g_vars.level_ypos[OBJECT_NUM_PLAYER1] = (ypos << 4) + 6;
	}
	if (ypos < 0) {
		ypos = 0;
	}
	g_vars.screen_tilemap_yorigin = ypos << 4;

	if (g_vars.screen_tilemap_yorigin + (TILEMAP_SCREEN_H + 16) > g_vars.screen_tilemap_h) {
		g_vars.screen_tilemap_yorigin = g_vars.screen_tilemap_h - (TILEMAP_SCREEN_H + 16);
	} else {
		g_vars.screen_tilemap_yorigin += 16;
	}

	for (int i = 0; i < MAX_OBJECTS; ++i) {
		if (!g_vars.two_players_flag) {
			if (g_vars.player == PLAYER_JAKE) {
				level_data[g_vars.level * MAX_OBJECTS + OBJECT_NUM_PLAYER1] = PLAYER_JAKE;
				level_data[g_vars.level * MAX_OBJECTS + OBJECT_NUM_PLAYER2] = 100;
			} else {
				level_data[g_vars.level * MAX_OBJECTS + OBJECT_NUM_PLAYER1] = PLAYER_ELWOOD;
				level_data[g_vars.level * MAX_OBJECTS + OBJECT_NUM_PLAYER2] = 100;
			}
		} else {
			level_data[g_vars.level * MAX_OBJECTS + OBJECT_NUM_PLAYER1] = PLAYER_JAKE;
			level_data[g_vars.level * MAX_OBJECTS + OBJECT_NUM_PLAYER2] = PLAYER_ELWOOD;
		}
		struct object_t *obj = &g_vars.objects[i];
		obj->type = level_data[g_vars.level * MAX_OBJECTS + i];
		obj->anim_frame = 0;
		obj->anim_num = 1;
		if (obj->type != 100) {
			const int num = obj->type * 116 / 4;
			assert(num < 1189);
			obj->animframes_ptr = animframes_059d + num;
		} else {
			obj->animframes_ptr = 0;
		}
		obj->facing_left = 0;
		obj->yvelocity = 0;
		obj->xvelocity = 0;
		obj->xmaxvelocity = 48;
		obj->ymaxvelocity = 15;
		if (obj->type < 2) {
			if (obj->unk60 == 0) {
				obj->xpos = g_vars.level_xpos[i];
				obj->ypos = g_vars.level_ypos[i];
			} else {
				obj->xpos = restart_xpos[g_vars.level * 2 + obj->type];
				obj->ypos = restart_ypos[g_vars.level * 2 + obj->type];
			}
			print_debug(DBG_GAME, "init_level obj #%d pos %d,%d", i, obj->xpos, obj->ypos);
		} else {
			obj->xpos = g_vars.level_xpos[i];
			obj->ypos = g_vars.level_ypos[i];
		}
		obj->screen_xpos = obj->xpos - g_vars.screen_tilemap_xorigin;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->xacc = 0;
		obj->yacc = 2;
		obj->direction_lr = 0;
		obj->direction_ud = 0;
		obj->xpos16 = obj->xpos >> 4;
		obj->ypos16 = obj->ypos >> 4;
		obj->floor_ypos16 = 0;
		// obj->unk26 = 0;
		obj->yfriction = 0;
		obj->unk2B = 0;
		obj->sprite_type = 0;
		obj->unk2D = 0;
		obj->unk2E = 1;
		obj->unk2F = 0;
		if (obj->type != 100) {
			obj->op = level_objtypes[obj->type];
		} else {
			obj->op = 0;
		}
		obj->grab_state = 0;
		obj->grab_type = 0;
		obj->special_anim = 0;
		// obj->unk35 = 0;
		obj->player_ydist = 0;
		obj->player_xdist = 0;
		obj->sprite3_counter = 0;
		obj->visible_flag = 1;
		if (obj->type == 100) {
			obj->visible_flag = 0;
		}
		obj->moving_direction = 0;
		obj->unk3D = 0;
		obj->carry_crate_flag = 0;
		// obj->unk3F = 6;
		// obj->unk41 = 0;
		obj->unk42 = 0;
		obj->tile2_flags = 0;
		obj->tile1_flags = 0;
		obj->tile0_flags = 0;
		obj->tile012_xpos = 0;
		obj->elevator_direction = -1;
		obj->trigger3_num = 0;
		obj->trigger3 = 0;
		obj->unk50 = 0;
		// obj->unk4E = 0;
		if (obj->restart_level_flag == 0 && obj->level_complete_flag == 0 && (g_vars.level == 0 || g_vars.play_demo_flag)) {
			g_vars.player1_dead_flag = 0;
			g_vars.player2_scrolling_flag = 0;
			obj->lifes_count = 3;
			obj->data51 = 3;
			obj->vinyls_count = 0;
		} else if (obj->restart_level_flag != 0) {
			--obj->lifes_count;
			obj->data51 = 3;
			obj->vinyls_count = 0;
		}
		if (g_options.cheats & CHEATS_UNLIMITED_LIFES) {
			obj->lifes_count = 3;
			obj->data51 = 3;
		}
		obj->restart_level_flag = 0;
		obj->level_complete_flag = 0;
		obj->unk53 = 0;
		obj->unk54 = 0;
		obj->unk55 = 0;
		obj->unk56 = 0;
		obj->collide_flag = 0;
		// obj->unk5A = 0;
		// obj->unk5C = 1;
		obj->unk5D = 0;
		if (obj->ypos > g_vars.screen_tilemap_h) {
			obj->unk53 = 1;
		}
		obj->blinking_counter = 0;
	}
	for (int i = 0; i < MAX_DOORS; ++i) {
		const uint8_t *p = &level_door[g_vars.level * 240 + i * 8];
		g_vars.doors[i].src_x16 = READ_LE_UINT16(p);
		g_vars.doors[i].src_y16 = READ_LE_UINT16(p + 2);
		g_vars.doors[i].dst_x16 = READ_LE_UINT16(p + 4);
		g_vars.doors[i].dst_y16 = READ_LE_UINT16(p + 6);
	}
}

static void do_level_redraw_tilemap(int xpos, int ypos) {
	const int w = (TILEMAP_SCREEN_W / 16) + ((!g_options.dos_scrolling && (xpos & 15) != 0) ? 1 : 0);
	const int h = (TILEMAP_SCREEN_H / 16) + ((!g_options.dos_scrolling && (ypos & 15) != 0) ? 1 : 0);
	const int y = ypos >> 4;
	const int x = xpos >> 4;
	for (int j = 0; j < h; ++j) {
		const uint8_t *ptr = lookup_sql(x, y + j);
		for (int i = 0; i < w; ++i) {
			const uint8_t num = *ptr++;
			screen_draw_tile(g_vars.screen_tile_lut[num & 0x7F], num & 0x80, i, j);
		}
	}
	g_vars.screen_tilemap_size_w = w;
	g_vars.screen_tilemap_size_h = h;
}

static void do_level_update_tiles_anim() {
	for (int i = 0; level_tilesdata_1e8c[i]; ++i) {
		uint8_t *p = level_tilesdata_1e8c[i];
		if (p[0] == p[1]) {
			p[0] = 1;
		} else {
			++p[0];
		}
	}
	const int w = (TILEMAP_SCREEN_W / 16) + ((!g_options.dos_scrolling && (g_vars.screen_tilemap_xorigin & 15) != 0) ? 1 : 0);
	const int h = (TILEMAP_SCREEN_H / 16) + ((!g_options.dos_scrolling && (g_vars.screen_tilemap_yorigin & 15) != 0) ? 1 : 0);
	const int y = g_vars.screen_tilemap_yorigin >> 4;
	const int x = g_vars.screen_tilemap_xorigin >> 4;
	for (int j = 0; j < h; ++j) {
		uint8_t *ptr = lookup_sql(x, y + j);
		for (int i = 0; i < w; ++i, ++ptr) {
			uint8_t num = *ptr;
			struct trigger_t *t = &g_res.triggers[num];
			const uint8_t *data = t->op_table2;
			if (!data) {
				continue;
			}
			if (t->tile_index != 0) {
				num = data[t->tile_index];
				*ptr = num;
				t->unk16 = num;
			} else {
				const int current = data[0];
				num = data[current + 1];
				*ptr = num;
				t->unk16 = num;
			}
			screen_draw_tile(g_vars.screen_tile_lut[num & 0x7F], num & 0x80, i, j);
		}
	}
}

static void do_level_update_scrolling2() {
	if (g_vars.screen_scrolling_dirmask & 2) {
		if (g_vars.screen_tilemap_w * 16 - TILEMAP_SCREEN_W > g_vars.screen_tilemap_xorigin) {
			g_vars.screen_tilemap_xorigin += 16;
		}
	}
	if (g_vars.screen_scrolling_dirmask & 1) {
		if (g_vars.screen_tilemap_xorigin > 0) {
			g_vars.screen_tilemap_xorigin -= 16;
		}
	}
	if (g_vars.screen_scrolling_dirmask & 8) {
		if (g_vars.screen_tilemap_yorigin > 0) {
			g_vars.screen_tilemap_yorigin -= 16;
		}
	}
	if (g_vars.screen_scrolling_dirmask & 4) {
		if (g_vars.screen_tilemap_h - TILEMAP_SCREEN_H > g_vars.screen_tilemap_yorigin) {
			g_vars.screen_tilemap_yorigin += 16;
		}
	}
	if (!g_options.dos_scrolling) {
		const int x1 = TILEMAP_SCROLL_W * 2;
		const int x2 = TILEMAP_SCREEN_W - TILEMAP_SCROLL_W * 2;
		const struct object_t *obj = &g_vars.objects[OBJECT_NUM_PLAYER1];
		if (obj->screen_xpos > x2) {
			const int dx = obj->screen_xpos - x2;
			g_vars.screen_tilemap_xorigin += dx;
			if (g_vars.screen_tilemap_xorigin + TILEMAP_SCREEN_W > g_vars.screen_tilemap_w * 16) {
				g_vars.screen_tilemap_xorigin = g_vars.screen_tilemap_w * 16 - TILEMAP_SCREEN_W;
			}
		} else if (obj->screen_xpos < x1) {
			const int dx = obj->screen_xpos - x1;
			g_vars.screen_tilemap_xorigin += dx;
			if (g_vars.screen_tilemap_xorigin < 0) {
				g_vars.screen_tilemap_xorigin = 0;
			}
		}
		const int y1 = 64;
		const int y2 = 144;
		if (obj->screen_ypos > y2) {
			const int dy = obj->screen_ypos - y2;
			g_vars.screen_tilemap_yorigin += dy;
			if (g_vars.screen_tilemap_yorigin + TILEMAP_SCREEN_H > g_vars.screen_tilemap_h) {
				g_vars.screen_tilemap_yorigin = g_vars.screen_tilemap_h - TILEMAP_SCREEN_H;
			}
		} else if (obj->screen_ypos < y1) {
			const int dy = obj->screen_ypos - y1;
			g_vars.screen_tilemap_yorigin += dy;
			if (g_vars.screen_tilemap_yorigin < 0) {
				g_vars.screen_tilemap_yorigin = 0;
			}
		}
		g_vars.screen_tilemap_xoffset = -(g_vars.screen_tilemap_xorigin & 15);
		g_vars.screen_tilemap_yoffset = -(g_vars.screen_tilemap_yorigin & 15);
	}
	do_level_redraw_tilemap(g_vars.screen_tilemap_xorigin, g_vars.screen_tilemap_yorigin);
	if ((g_vars.level_loop_counter & 3) == 0) {
		do_level_update_tiles_anim();
	}
}

void do_level_drop_grabbed_object(struct object_t *obj) {
	struct object_t *obj37 = &g_vars.objects[37];
	if (obj->type != 0 && g_vars.two_players_flag) {
		obj37 = &g_vars.objects[36];
	}
	if (obj37->type != 100) {
		obj37->grab_state = 3;
		obj->grab_state = 0;
		obj->unk3D = 0;
	}
}

static void do_level_add_sprite1_case0(struct object_t *obj) {
	const uint8_t *anim_data = 0;

	assert(obj->animframes_ptr);

	if (obj->unk2F != 0 && obj->direction_ud == 0 && obj->direction_lr == 0) {
		anim_data = obj->animframes_ptr[24 / 4];
	} else if (triggers_get_tile_type(obj->xpos16, obj->ypos16) == 4) {
		anim_data = obj->animframes_ptr[28 / 4];
	} else {
		if (abs(obj->yvelocity) > 10 && obj->yvelocity > 0) {
			anim_data = obj->animframes_ptr[112 / 4];
		} else {
			anim_data = obj->animframes_ptr[8 / 4];
		}
	}
	if (obj->grab_state != 0) {
		if (obj->grab_type == 0) {
			anim_data = obj->animframes_ptr[52 / 4];
		} else if (obj->grab_type == 2 || obj->grab_type == 8) {
			anim_data = obj->animframes_ptr[32 / 4];
			obj->xmaxvelocity = 48;
		}
	}
	if (obj->anim_num < anim_data[0]) {
		++obj->anim_num;
	} else {
		obj->anim_num = 1;
	}
	obj->anim_frame = anim_data[obj->anim_num];
}

static void do_level_add_sprite1_case1(struct object_t *obj) {
	const uint8_t *anim_data = 0;
	assert(obj->animframes_ptr);
	obj->xmaxvelocity = 48;
	if ((obj->direction_lr & 1) != 0) {
		if (obj->unk2E == 0) {
			anim_data = obj->animframes_ptr[16 / 4];
			obj->xmaxvelocity = 24;
		} else if (obj->grab_state != 0) {
			if (obj->grab_type == 0 || obj->grab_type == 8) {
				anim_data = obj->animframes_ptr[48 / 4];
			} else if (obj->grab_type == 2) {
				anim_data = obj->animframes_ptr[32 / 4];
			} else {
				anim_data = obj->animframes_ptr[4 / 4];
			}
		} else {
			anim_data = obj->animframes_ptr[4 / 4];
		}
	} else if ((obj->direction_lr & 2) != 0) {
		if (obj->unk2E == 0) {
			anim_data = obj->animframes_ptr[16 / 4];
			obj->xmaxvelocity = 24;
		} else if (obj->grab_state != 0) {
			if (obj->grab_type == 0 || obj->grab_type == 8) {
				anim_data = obj->animframes_ptr[48 / 4];
			} else if (obj->grab_type == 2) {
				anim_data = obj->animframes_ptr[32 / 4];
			} else {
				anim_data = obj->animframes_ptr[4 / 4];
			}
		} else {
			anim_data = obj->animframes_ptr[4 / 4];
		}
	} else {
		if (obj->unk2E == 0) {
			anim_data = obj->animframes_ptr[12 / 4];
		} else if (obj->grab_state != 0) {
			if (obj->grab_type == 0 || obj->grab_type == 8) {
				anim_data = obj->animframes_ptr[44 / 4];
			} else if (obj->grab_type == 2) {
				anim_data = obj->animframes_ptr[32 / 4];
			} else {
				anim_data = obj->animframes_ptr[4 / 4];
			}
		} else {
			anim_data = obj->animframes_ptr[0];
		}
		if (obj->unk2D < 8) {
			if (abs(obj->xvelocity) > 8) {
				anim_data = obj->animframes_ptr[24 / 4];
			}
		}
	}
	if (obj->anim_num < anim_data[0]) {
		++obj->anim_num;
	} else {
		obj->anim_num = 1;
	}
	obj->anim_frame = anim_data[obj->anim_num];
}

// ladder
static void do_level_add_sprite1_case2(struct object_t *obj) {
	struct object_t *obj35 = &g_vars.objects[35];
	struct object_t *obj37 = &g_vars.objects[37];
	if (obj->type != 0 && g_vars.two_players_flag) {
		obj35 = &g_vars.objects[34];
		obj37 = &g_vars.objects[36];
	}
	if (obj37->type != 100 && obj37->grab_type != 0) {
		do_level_drop_grabbed_object(obj);
	}
	if (obj->grab_type == 8) {
		do_level_drop_grabbed_object(obj);
	}
	if (obj35->type != 100) {
		obj35->type = 100;
	}
	const uint8_t *anim_data = 0;
	obj->unk3D = 0;
	int tile_num = triggers_get_tile_type(obj->xpos16, obj->ypos16);
	if (obj->direction_ud != 0) {
		if (tile_num == 7) {
			anim_data = obj->animframes_ptr[20 / 4];
		} else {
			anim_data = obj->animframes_ptr[0];
		}
	} else if (obj->direction_lr != 0) {
		if (tile_num == 7) {
			anim_data = obj->animframes_ptr[20 / 4];
		} else {
			anim_data = obj->animframes_ptr[4 / 4];
		}
	} else {
		if (tile_num == 7) {
			anim_data = obj->animframes_ptr[36 / 4];
		} else if (obj->grab_state != 0) {
			if (obj->grab_type == 0 || obj->grab_type == 8) {
				anim_data = obj->animframes_ptr[24 / 4];
			} else if (obj->grab_type == 2) {
				anim_data = obj->animframes_ptr[32 / 4];
			} else {
				anim_data = obj->animframes_ptr[0];
			}
		} else {
			anim_data = obj->animframes_ptr[0];
		}
	}
	if (obj->anim_num < anim_data[0]) {
		obj->anim_num += (g_vars.level_loop_counter & 1);
	} else {
		obj->anim_num = 1;
	}
	obj->anim_frame = anim_data[obj->anim_num];
}

static void do_level_add_sprite1_case3(struct object_t *obj) {
	struct object_t *obj35 = &g_vars.objects[35];
	struct object_t *obj37 = &g_vars.objects[37];
	if (obj->type != 0 && g_vars.two_players_flag) {
		obj35 = &g_vars.objects[34];
		obj37 = &g_vars.objects[36];
	}
	if (obj37->type != 100 && obj37->grab_type != 0) {
		do_level_drop_grabbed_object(obj);
	}
	obj->unk3D = 0;
	if (obj35->type != 100) {
		obj35->type = 100;
		obj35->visible_flag = 0;
	}
	const uint8_t *anim_data = 0;
	obj->unk3D = 0;
	int tile_num = triggers_get_tile_type(obj->xpos16, obj->ypos16);
	obj->xmaxvelocity = 8;
	obj->xacc = 0;
	obj->unk2D = 10;
	if (obj->direction_lr != 0) {
		if (obj->direction_ud != 0) {
			if (tile_num == 6) {
				anim_data = obj->animframes_ptr[56 / 4];
			} else {
				anim_data = obj->animframes_ptr[0];
			}
		} else {
			if (tile_num == 6) {
				anim_data = obj->animframes_ptr[68 / 4];
			} else {
				anim_data = obj->animframes_ptr[4 / 4];
			}
		}
	} else {
		if (obj->direction_ud != 0) {
			if (tile_num == 6) {
				anim_data = obj->animframes_ptr[56 / 4];
			} else {
				anim_data = obj->animframes_ptr[0];
			}
		} else {
			if (tile_num == 6) {
				anim_data = obj->animframes_ptr[68 / 4];
			} else {
				anim_data = obj->animframes_ptr[4 / 4];
			}
		}
	}
	if (obj->anim_num < anim_data[0]) {
		obj->anim_num += (g_vars.level_loop_counter & 1);
	} else {
		obj->anim_num = 1;
	}
	obj->anim_frame = anim_data[obj->anim_num];
}

// swimming
static void do_level_add_sprite1_case4(struct object_t *obj) {
	struct object_t *obj35 = &g_vars.objects[35];
	struct object_t *obj37 = &g_vars.objects[37];
	if (obj->type != 0 && g_vars.two_players_flag) {
		obj35 = &g_vars.objects[34];
		obj37 = &g_vars.objects[36];
	}
	if (obj37->type != 100) {
		do_level_drop_grabbed_object(obj);
	}
	const uint8_t *anim_data = 0;
	obj->unk2F = 0;
	int tile0_num = triggers_get_tile_type(obj->xpos16, obj->ypos16 - 2);
	// int tile1_num = triggers_get_tile_type(obj->xpos16, obj->ypos16 - 1);
	// int tile2_num = triggers_get_tile_type(obj->xpos16, obj->ypos16);
	obj->xmaxvelocity = 16;
	if (obj->direction_lr != 0) {
		if (obj->direction_ud == 1) {
			if (tile0_num == 0) {
				obj->unk3D = 1;
			} else {
				obj->unk3D = 2;
			}
		} else if (obj->direction_ud == 2) {
			obj->unk3D = 0;
		} else {
			obj->unk3D = 1;
		}
		anim_data = obj->animframes_ptr[92 / 4];
	} else {
		if (obj->direction_ud == 1) {
			if (tile0_num == 0) {
				obj->unk3D = 1;
			} else {
				obj->unk3D = 2;
			}
			anim_data = obj->animframes_ptr[100 / 4];
		} else if (obj->direction_ud == 2) {
			obj->unk3D = 0;
			anim_data = obj->animframes_ptr[100 / 4];
		} else {
			obj->unk3D = 1;
			anim_data = obj->animframes_ptr[96 / 4];
			if (triggers_get_tile_type(obj->xpos16, obj->ypos16 + 1) == 13) {
				anim_data = obj->animframes_ptr[104 / 4];
			}
		}
	}
	if (obj->anim_num < anim_data[0]) {
		++obj->anim_num;
	} else {
		obj->anim_num = 1;
	}
	obj->anim_frame = anim_data[obj->anim_num];
	if (tile0_num >= 11 && obj35->type == 100) {
		play_sound(SOUND_15);
		obj35->type = 2;
		obj35->grab_type = 7;
		obj35->visible_flag = 1;
		if (obj->facing_left) {
			obj35->xpos = obj->xpos - 8;
			obj35->screen_xpos = obj->screen_xpos - 8;
		} else {
			obj35->xpos = obj->xpos + 8;
			obj35->screen_xpos = obj->screen_xpos + 8;
		}
		obj35->ypos = obj->ypos - 8;
		obj35->screen_ypos = obj->screen_ypos - 8;
		obj35->xpos16 = obj->xpos16;
		obj35->ypos16 = obj->ypos16;
		obj35->anim_num = 1;
		obj35->animframes_ptr = animframes_059d + (obj35->type * 116 / 4);
	} else {
		obj35->unk2E = 0;
		const int num = triggers_get_tile_type(obj35->xpos16, obj35->ypos16);
		if (num == 11 || num == 12) {
			obj35->yacc = -3;
			obj35->yvelocity = -3;
			obj35->ypos16 = obj35->ypos >> 4;
			obj35->xpos16 = obj35->xpos >> 4;
		} else {
			obj35->type = 100;
			obj35->visible_flag = 0;
			obj35->grab_type = 0;
		}
	}
}

static void do_level_add_sprite1(struct object_t *obj) {
	print_debug(DBG_GAME, "add_sprite1 obj->type %d", obj->type);
	if (obj->type == 100) {
		// print_warning("do_level_add_sprite1: obj.type 100");
		return;
	}
	obj->sprite3_counter = 0;
	switch (obj->sprite_type) {
	case 0:
		do_level_add_sprite1_case0(obj);
		break;
	case 1:
		do_level_add_sprite1_case1(obj);
		break;
	case 2:
		do_level_add_sprite1_case2(obj);
		break;
	case 3:
		do_level_add_sprite1_case3(obj);
		break;
	case 4:
		do_level_add_sprite1_case4(obj);
		break;
	default:
		print_warning("do_level_add_sprite1: unhandled sprite_type %d", obj->sprite_type);
		break;
	}
	if (obj->type != 100) {
		if (obj->blinking_counter != 0 && (g_vars.level_loop_counter & 1) != 0) {
			if (obj->facing_left == 0) {
				screen_add_game_sprite3(obj->screen_xpos, obj->screen_ypos + 1, obj->anim_frame, obj->blinking_counter);
			} else {
				screen_add_game_sprite4(obj->screen_xpos, obj->screen_ypos + 1, obj->anim_frame, obj->blinking_counter);
			}
		} else {
			if (obj->facing_left == 0) {
				screen_add_game_sprite1(obj->screen_xpos, obj->screen_ypos + 1, obj->anim_frame);
			} else {
				screen_add_game_sprite2(obj->screen_xpos, obj->screen_ypos + 1, obj->anim_frame);
			}
		}
	}
}

static void do_level_add_sprite2(struct object_t *obj) {
	print_debug(DBG_GAME, "add_sprite2 obj->type %d", obj->type);
	assert(obj->type == 2);
	assert(obj->animframes_ptr);
	const uint8_t *anim_data = obj->animframes_ptr[obj->grab_type];
	if (obj->anim_num < anim_data[0]) {
		++obj->anim_num;
	} else {
		obj->anim_num = 1;
	}
	obj->anim_frame = anim_data[obj->anim_num];
	if (obj->grab_type >= 16 && obj->grab_type != 23) {
		obj->yacc = -1;
		obj->yvelocity = -1;
		obj->facing_left = 0;
		if (obj->data5F < 30) {
			++obj->data5F;
		} else {
			obj->data5F = 0;
			obj->type = 100;
			obj->visible_flag = 0;
			obj->grab_type = 0;
			obj->yacc = 0;
			obj->yvelocity = 0;
		}
	}
	if (obj->type != 100) {
		if (obj->facing_left == 0) {
			screen_add_game_sprite1(obj->screen_xpos, obj->screen_ypos + 1, obj->anim_frame);
		} else {
			screen_add_game_sprite2(obj->screen_xpos, obj->screen_ypos + 1, obj->anim_frame);
		}
	}
}

static void do_level_add_sprite3(struct object_t *obj) {
	print_debug(DBG_GAME, "add_sprite3 obj->type %d", obj->type);
	if (!obj->animframes_ptr) return;
	const uint8_t *anim_data = obj->animframes_ptr[obj->special_anim];
	if (obj->sprite3_counter == 0) {
		obj->anim_num = 1;
		obj->anim_frame = anim_data[obj->anim_num];
		++obj->sprite3_counter;
	} else if (obj->sprite3_counter == 1) {
		if (obj->anim_num < anim_data[0]) {
			if (obj->collide_flag == 0) {
				obj->anim_num += (g_vars.level_loop_counter & 1);
			}
			obj->anim_frame = anim_data[obj->anim_num];
		} else {
			if (obj->special_anim != 16) {
				obj->anim_num = 1;
				obj->special_anim = 0;
				obj->sprite3_counter = 0;
			// } else if (obj->unk5A != 0) {
				// obj->unk5C = 4;
			}
		}
		if (obj->special_anim == 18 && obj->anim_num > 8 && (g_vars.inp_key_up != 0 /* || g_inp_key_T != 0*/)) {
			obj->anim_num = 1;
			obj->special_anim = 0;
			obj->sprite3_counter = 0;
			// if (obj->unk5A != 0) {
				// obj->unk5C = 4;
			// }
		}
	}
	if (obj->type != 100) {
		if (obj->blinking_counter != 0 && (g_vars.level_loop_counter & 1) != 0) {
			if (obj->facing_left == 0) {
				screen_add_game_sprite3(obj->screen_xpos, obj->screen_ypos + 1, obj->anim_frame, obj->blinking_counter);
			} else {
				screen_add_game_sprite4(obj->screen_xpos, obj->screen_ypos + 1, obj->anim_frame, obj->blinking_counter);
			}
		} else {
			if (obj->facing_left == 0) {
				screen_add_game_sprite1(obj->screen_xpos, obj->screen_ypos + 1, obj->anim_frame);
			} else {
				screen_add_game_sprite2(obj->screen_xpos, obj->screen_ypos + 1, obj->anim_frame);
			}
		}
	}
}

void do_level_update_tile(int x, int y, int num) {
	uint8_t *ptr = lookup_sql(x, y);
	*ptr = num;
	screen_draw_tile(g_vars.screen_tile_lut[num & 0x7F], num & 0x80, x, y);
}

static void do_level_reset_tiles(struct object_t *obj, int dy) {
	int i, y, var2, var4;

	y = obj->ypos16 + dy;
	var2 = triggers_get_next_tile_num(obj->xpos16, y);
	i = 1;
	do {
		var4 = triggers_get_next_tile_num(obj->xpos16 + i, y);
		++i;
	} while (var4 == var2);
	do_level_update_tile(obj->xpos16, y, var4);
}

static int do_level_throw_crate(struct object_t *obj) {
	if (obj->screen_xpos > TILEMAP_SCREEN_W + 16 || obj->screen_xpos <= -16) {
		obj->moving_direction = 1;
	}
	if (obj->xpos < 16 || obj->xpos > g_vars.screen_tilemap_w * 16 - 16) {
		obj->moving_direction = 1;
	}
	if (obj->moving_direction == 0) {
		if (obj->facing_left == 0) {
			obj->direction_lr |= 1;
			obj->xvelocity = 128;
		} else {
			obj->direction_lr |= 2;
			obj->xvelocity = -128;
		}
		obj->yacc = 0;
		obj->yvelocity = 0;
		obj->xmaxvelocity = 128;
		return 0;
	} else {
		obj->type = 100;
		obj->visible_flag = 0;
		obj->xvelocity = 0;
		obj->unk5D = 0;
		return 1;
	}
}

static void do_level_update_grabbed_object_type8(struct object_t *obj) {
	obj->direction_ud = 0;
	if (obj->screen_ypos < 32 || obj->ypos - 48 <= 0) {
		obj->type = 100;
		obj->visible_flag = 0;
		obj->grab_state = 0;
		obj->grab_type = 0;
		obj->yvelocity = 0;
		obj->yacc = 2;
		obj->unk5D = 0;
	} else {
		obj->yacc = -4;
		obj->yvelocity = -4;
	}
}

static void do_level_update_grabbed_object(struct object_t *obj) {
	struct object_t *obj37 = &g_vars.objects[37];

	if (obj->type != 0 && g_vars.two_players_flag != 0) {
		--obj37;
	}
	switch (obj->grab_state) {
	case 0:
		if (obj37->type == 100) {
			obj->carry_crate_flag = 0;
			obj->grab_state = 1;
			play_sound(SOUND_3);
			if (obj->unk2E == 0) {
				obj->unk2E = 1;
			}
		}
		break;
	case 1:
		obj37->type = 2;
		obj37->visible_flag = 1;
		obj37->xpos = obj->xpos;
		obj37->ypos = obj->ypos - 30;
		obj37->screen_xpos = obj->screen_xpos;
		obj37->screen_ypos = obj->screen_ypos - 30;
		obj37->animframes_ptr = animframes_059d + (obj37->type * 116 / 4);
		obj37->anim_num = 1;
		obj37->facing_left = obj->facing_left;
		obj37->sprite_type = obj->sprite_type;
		obj37->yvelocity = obj->yvelocity;
		obj37->xvelocity = obj->xvelocity;
		obj37->yacc = obj->yacc;
		obj37->xacc = obj->xacc;
		obj37->unk5D = obj->type;
		obj37->xpos16 = obj->xpos16;
		obj37->ypos16 = obj->ypos16;
		obj->grab_state = 1;
		if (obj->carry_crate_flag != 0) {
			play_sound(SOUND_5);
			obj->grab_state = 0;
			obj37->grab_state = 2;
			obj37->ypos += 10;
			obj->unk3D = 0;
			obj37->moving_direction = 0;
			obj->carry_crate_flag = 0;
		}
		if (triggers_get_tile_type(obj37->xpos16, obj37->ypos16 - 2) > 5 || obj37->ypos < 32 || obj37->xpos < 16) {
			if (obj37->grab_type == 2) {
				obj->grab_state = 0;
				obj->unk3D = 0;
				obj->carry_crate_flag = 0;
				obj37->grab_state = 3;
				obj37->moving_direction = 0;
				obj37->anim_num = 1;
			}
		}
		break;
	case 2:
		if (obj->grab_type == 0) {
			if (do_level_throw_crate(obj)) {
				obj->grab_state = 0;
			}
		} else if (obj->grab_type == 2 || obj->grab_type == 8) {
			do_level_update_grabbed_object_type8(obj);
		}
		break;
	case 3:
		if (obj->grab_type == 2 || obj->grab_type == 3) {
			obj->grab_type = 3;
			if (obj->anim_num >= 8) {
				obj->type = 100;
				obj->visible_flag = 0;
				obj->grab_state = 0;
				obj->grab_type = 0;
				obj->yvelocity = 0;
				obj->yacc = 2;
				obj37->unk5D = 0;
			}
		} else if (obj->grab_type == 8) {
			do_level_update_grabbed_object_type8(obj);
		} else {
			obj->type = 100;
			obj->visible_flag = 0;
			obj->grab_state = 0;
			obj->grab_type = 0;
			obj->yvelocity = 0;
			obj->yacc = 2;
			obj37->unk5D = 0;
		}
		break;
	default:
		print_warning("do_level_update_grabbed_object: unhandled grab_state %d", obj->grab_state);
		break;
	}
}

void do_level_update_panel_lifes(struct object_t *obj) {
	struct object_t *obj39 = &g_vars.objects[OBJECT_NUM_PLAYER1];
	struct object_t *obj40 = &g_vars.objects[OBJECT_NUM_PLAYER2];
	if (!g_vars.two_players_flag) {
		static const uint16_t data[] = { 216, 232, 248, 264, 280, 296 };
		for (int i = 0; i < obj->data51; ++i) {
			screen_draw_frame(g_res.spr_frames[120 + i], 12, 16, data[i], 161);
		}
		for (int i = obj->data51; i < 5; ++i) {
			screen_draw_frame(g_res.spr_frames[119 + i], 12, 16, data[i], 161);
		}
	} else {
		static const uint8_t data[] = { 18, 21, 20, 19, 19, 19 };
		if (obj->type == PLAYER_JAKE) {
			screen_draw_frame(g_res.spr_frames[101 + data[obj->data51]], 12, 16, 80, 161);
		} else {
			screen_draw_frame(g_res.spr_frames[101 + data[obj->data51]], 12, 16, 248, 161);
		}
	}
	if (obj->data51 == 0) { // health
		if (!g_vars.two_players_flag) {
			obj->restart_level_flag = 1;
			g_vars.game_over_flag = 1;
		} else {
			if (obj->lifes_count != 1) {
				--obj->lifes_count;
				obj->data51 = 3;
				do_level_update_panel_lifes(obj);
				if (obj->type == PLAYER_JAKE) {
					screen_draw_number(obj->lifes_count - 1, 48, 163, 2);
				} else {
					screen_draw_number(obj->lifes_count - 1, 216, 163, 2);
				}
			} else if (obj->type == PLAYER_JAKE) {
				g_vars.player2_scrolling_flag = 1;
				g_vars.switch_player_scrolling_flag = 1;
				obj39->special_anim = 16;
				g_vars.player2_dead_flag = 1;
				screen_draw_frame(g_res.spr_frames[116], 12, 16, 8, 161);
			} else { // PLAYER_ELWOOD
				g_vars.player2_scrolling_flag = 0;
				g_vars.switch_player_scrolling_flag = 1;
				obj40->special_anim = 16;
				g_vars.player1_dead_flag = 1;
				screen_draw_frame(g_res.spr_frames[118], 12, 16, 176, 161);
			}
			if (g_vars.player2_dead_flag && g_vars.player1_dead_flag) {
				g_vars.game_over_flag = 1;
			}
		}
	}
}

static void do_level_update_panel_vinyls(struct object_t *obj) {
	if (obj->vinyls_count > 99) {
		obj->vinyls_count -= 100;
		if (!g_vars.two_players_flag && obj->data51 < 5) {
			++obj->data51;
		}
		if (g_vars.two_players_flag && obj->data51 < 3) {
			++obj->data51;
		}
		do_level_update_panel_lifes(obj);
	}
	if (!g_vars.two_players_flag) {
		if (obj->vinyls_count != g_vars.vinyls_count) {
			screen_draw_number(obj->vinyls_count, 192, 163, 2);
		}
		g_vars.vinyls_count = obj->vinyls_count;
	} else {
		if (obj->vinyls_count != g_vars.vinyls_count) {
			screen_draw_number(obj->vinyls_count, 112, 163, 2);
		}
		g_vars.vinyls_count = obj->vinyls_count;
	}
}

static void do_level_update_panel_2nd_player() {
	struct object_t *obj = &g_vars.objects[OBJECT_NUM_PLAYER2];
	if (obj->vinyls_count > 99) {
		obj->vinyls_count = 0;
		if (!g_vars.two_players_flag && obj->data51 < 5) {
			++obj->data51;
		}
		if (g_vars.two_players_flag && obj->data51 < 3) {
			++obj->data51;
		}
		do_level_update_panel_lifes(obj);
	}
	if (obj->vinyls_count != g_vars.vinyls_count) {
		screen_draw_number(obj->vinyls_count, 280, 163, 2);
		g_vars.vinyls_count = obj->vinyls_count;
	}
}

static void draw_level_panel() {
	struct object_t *obj39 = &g_vars.objects[OBJECT_NUM_PLAYER1];
	struct object_t *obj40 = &g_vars.objects[OBJECT_NUM_PLAYER2];
	g_vars.vinyls_count = 0;
	if (!g_vars.two_players_flag) {
		screen_draw_frame(g_res.spr_frames[123], 12, 320, 0, -12);
		screen_draw_frame(g_res.spr_frames[124], 12, 320, 0, 161);
		if (g_vars.player == PLAYER_JAKE) {
			screen_draw_frame(g_res.spr_frames[115], 12, 16, 24, 161);
		} else {
			screen_draw_frame(g_res.spr_frames[117], 12, 16, 24, 161);
		}
		for (int i = 0; i < obj39->data51; ++i) {
			screen_draw_frame(g_res.spr_frames[120], 12, 16, 216 + i * 16, 161);
		}
		screen_draw_number(obj39->vinyls_count, 192, 163, 2);
		screen_draw_number(obj39->lifes_count, 64, 163, 2);
		for (int i = 0; i < 5; ++i) {
			screen_draw_frame(g_res.spr_frames[135 + i], 12, 16, 80 + i * 32, -12);
		}
		if (obj39->data5F == 0) {
			for (int i = 0; i < g_vars.level; ++i) {
				screen_draw_frame(g_res.spr_frames[140 + i], 12, 16, 80 + i * 32, -12);
			}
		} else {
			for (int i = 0; i <= g_vars.level; ++i) {
				screen_draw_frame(g_res.spr_frames[140 + i], 12, 16, 80 + i * 32, -12);
			}
		}
	} else {
		screen_draw_frame(g_res.spr_frames[123], 12, 320, 0, -12);
		screen_draw_frame(g_res.spr_frames[124], 12, 320, 0, 161);
		screen_draw_frame(g_res.spr_frames[115], 12, 16, 8, 161);
		screen_draw_frame(g_res.spr_frames[117], 12, 16, 176, 161);
		do_level_update_panel_lifes(obj39);
		do_level_update_panel_lifes(obj40);
		screen_draw_number(obj39->vinyls_count, 112, 163, 2);
		screen_draw_number(obj40->vinyls_count, 280, 163, 2);
		screen_draw_number(obj39->lifes_count, 48, 163, 2);
		screen_draw_number(obj40->lifes_count, 216, 163, 2);
		for (int i = 0; i < 5; ++i) {
			screen_draw_frame(g_res.spr_frames[135 + i], 12, 16, 80 + i * 32, -12);
		}
		if (obj39->data5F == 0) {
			for (int i = 0; i < g_vars.level; ++i) {
				screen_draw_frame(g_res.spr_frames[140 + i], 12, 16, 80 + i * 32, -12);
			}
		} else {
			for (int i = 0; i <= g_vars.level; ++i) {
				screen_draw_frame(g_res.spr_frames[140 + i], 12, 16, 80 + i * 32, -12);
			}
		}
	}
}

void do_level_enter_door(struct object_t *obj) {
	static const uint8_t pos[] = { 74, 74, 77, 66, 102 };
	for (int i = 0; i < MAX_DOORS; ++i) {
		if (obj->xpos16 == g_vars.doors[i].src_x16 && obj->ypos16 == g_vars.doors[i].src_y16) {
			screen_do_transition1(0);
			obj->xpos = g_vars.doors[i].dst_x16 << 4;
			obj->ypos = g_vars.doors[i].dst_y16 << 4;
			if (obj->ypos > g_vars.screen_tilemap_h) {
				obj->unk53 = 1;
				g_vars.screen_tilemap_yorigin = pos[g_vars.level] << 4;
				const int w = TILEMAP_SCREEN_W / 16;
				g_vars.screen_tilemap_xorigin = (((obj->xpos >> 4) / w) * w) << 4;
			} else {
				g_vars.screen_tilemap_xorigin = ((obj->xpos >> 4) - 10) << 4;
				g_vars.screen_tilemap_yorigin = ((obj->ypos >> 4) -  6) << 4;
				if (g_vars.screen_tilemap_yorigin + (TILEMAP_SCREEN_H + 16) > g_vars.screen_tilemap_h) {
					g_vars.screen_tilemap_yorigin = g_vars.screen_tilemap_h - (TILEMAP_SCREEN_H + 16);
				} else {
					g_vars.screen_tilemap_yorigin += 16;
				}
				obj->unk53 = 0;
			}
			obj->screen_xpos = obj->xpos - g_vars.screen_tilemap_xorigin;
			obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
			do_level_redraw_tilemap(g_vars.screen_tilemap_xorigin, g_vars.screen_tilemap_yorigin);
			do_level_update_scrolling2();
			draw_level_panel();
			do_level_update_panel_vinyls(obj);
			if (g_vars.two_players_flag) {
				do_level_update_panel_2nd_player();
			}
			screen_flip();
			screen_do_transition1(1);
			obj->unk3D = 0;
			obj->sprite_type = 0;
			obj->yacc = 2;
			obj->yvelocity = 0;
		}
	}
}

static void do_level_update_input(struct object_t *obj) {
	struct object_t *obj37 = &g_vars.objects[37];
	int _si, _di;

	obj->carry_crate_flag = 0;
	if (obj->unk56 != 0) {
		obj->special_anim = 27;
		obj->xmaxvelocity = 112;
		switch (obj->unk54) {
		case 1:
			obj->direction_ud = 2;
			break;
		case 2:
			obj->yacc = obj->yvelocity = -4;
			break;
		}
		switch (obj->unk55) {
		case 1:
			obj->direction_lr = 1;
			break;
		case 2:
			obj->direction_lr = 2;
			break;
		}
	}
	if (obj->special_anim != 0 && obj->special_anim != 22) {
		return;
	}
	obj->xmaxvelocity = 48;
	if (obj->trigger3_num == 0) {
		triggers_update_tiles1(obj);
	} else {
		triggers_update_tiles2(obj);
	}
	if (g_vars.inp_key_space && g_vars.inp_key_up && !g_vars.inp_key_up_prev && !g_vars.player2_scrolling_flag) {
		obj->carry_crate_flag = 1;
		do_level_enter_door(obj);
		g_vars.inp_key_space_prev = g_vars.inp_key_space;
	}
	if (g_vars.inp_key_space != g_vars.inp_key_space_prev && !g_vars.inp_key_up) {
		if (g_vars.inp_key_space) {
			obj->carry_crate_flag = 1;
		}
		g_vars.inp_key_space_prev = g_vars.inp_key_space;
	}
	if (g_vars.inp_key_right) {
		obj->direction_lr |= OBJECT_DIRECTION_RIGHT;
	}
	if (g_vars.inp_key_left) {
		obj->direction_lr |= OBJECT_DIRECTION_LEFT;
	}
	_si = triggers_get_tile_type(obj->xpos16, obj->ypos16);
	if (g_vars.inp_key_up && !g_vars.inp_key_space) {
		obj->direction_ud = OBJECT_DIRECTION_UP;
		if (!g_vars.inp_key_up_prev && obj->sprite_type != 0) {
			obj->yfriction = 0;
			_di = triggers_get_tile_type(obj->xpos16, obj->ypos16 - 2);
			if (_di == 10 && (_si == 0 || _si == 1)) {
				if (obj->unk2E == 1 && obj->sprite_type != 4 && obj->blinking_counter == 0) {
					if (obj37->type != 100) {
						do_level_drop_grabbed_object(obj);
					}
					obj->special_anim = 18;
					play_sound(SOUND_11);
				}
			} else {
				if (obj->grab_state != 0 && obj->grab_type == 0) {
					obj->yfriction = 9;
				} else {
					obj->yfriction = 14;
				}
			}
		}
	} else if (g_vars.inp_key_down) {
		obj->direction_ud = 2;
		if (!g_vars.inp_key_down_prev) {
			obj->unk2B = 1;
		}
	} else {
		obj->direction_ud = 0;
	}
	g_vars.inp_key_up_prev = g_vars.inp_key_up;
	g_vars.inp_key_down_prev = g_vars.inp_key_down;
	if (obj->grab_state == 0) {
		if (obj->carry_crate_flag != 0 && _si == 5) { // crate
			obj->grab_type = 0;
			obj->grab_state = 0;
			do_level_reset_tiles(obj, 0);
			g_vars.objects[37].grab_state = 0;
			g_vars.objects[37].grab_type = 0;
			do_level_update_grabbed_object(obj);
			obj->carry_crate_flag = 0;
		} else if (_si == 8) { // balloon
			obj->grab_state = 0;
			obj->grab_type = 2;
			do_level_reset_tiles(obj, 0);
			do_level_reset_tiles(obj, -1);
			g_vars.objects[37].grab_state = 0;
			g_vars.objects[37].grab_type = 2;
			do_level_update_grabbed_object(obj);
			obj->unk3D = 2;
		} else if (_si == 9) { // umbrella
			obj->grab_type = 8;
			obj->grab_state = 0;
			do_level_reset_tiles(obj, 0);
			do_level_reset_tiles(obj, -1);
			g_vars.objects[37].grab_state = 0;
			g_vars.objects[37].grab_type = 8;
			do_level_update_grabbed_object(obj);
			obj->unk3D = 1;
		}
	}
}

static void do_level_update_scrolling(struct object_t *obj) {
	if (obj->unk53 != 0) {
		g_vars.screen_scrolling_dirmask = 0;
		return;
	}
	if (g_vars.switch_player_scrolling_flag) {
		if (obj->screen_xpos > (TILEMAP_SCREEN_W - 144)) {
			g_vars.screen_scrolling_dirmask |= 2;
		}
		if (obj->screen_xpos < 144) {
			g_vars.screen_scrolling_dirmask |= 1;
		}
		if ((g_vars.screen_scrolling_dirmask & 2) != 0 && obj->screen_xpos < TILEMAP_SCREEN_W / 2) {
			g_vars.screen_scrolling_dirmask &= ~2;
			g_vars.switch_player_scrolling_flag = 0;
			g_vars.inp_keyboard[0xC1] = 0;
		}
		if ((g_vars.screen_scrolling_dirmask & 1) != 0 && obj->screen_xpos > TILEMAP_SCREEN_W / 2) {
			g_vars.screen_scrolling_dirmask &= ~1;
			g_vars.switch_player_scrolling_flag = 0;
			g_vars.inp_keyboard[0xC1] = 0;
		}
	} else {
		int _si = (obj->xvelocity >> 3) + obj->unk1C;
		if (obj->screen_xpos > (TILEMAP_SCREEN_W - TILEMAP_SCROLL_W) && (g_vars.screen_tilemap_w * 16 - TILEMAP_SCREEN_W) > g_vars.screen_tilemap_xorigin) {
			g_vars.screen_scrolling_dirmask |= 2;
		}
		if (obj->screen_xpos < TILEMAP_SCROLL_W && g_vars.screen_tilemap_xorigin > 0) {
			g_vars.screen_scrolling_dirmask |= 1;
		}
		if ((g_vars.screen_scrolling_dirmask & 2) != 0 && (obj->screen_xpos < (TILEMAP_SCROLL_W - 16) || _si <= 0)) {
			g_vars.screen_scrolling_dirmask &= ~2;
		}
		if ((g_vars.screen_scrolling_dirmask & 1) != 0 && (obj->screen_xpos > (TILEMAP_SCREEN_W - (TILEMAP_SCROLL_W - 16)) || _si >= 0)) {
			g_vars.screen_scrolling_dirmask &= ~1;
		}
	}
	if (obj->screen_ypos < 64) {
		g_vars.screen_scrolling_dirmask |= 8;
	} else if (obj->screen_ypos > 144) {
		g_vars.screen_scrolling_dirmask |= 4;
	}
	if ((g_vars.screen_scrolling_dirmask & 8) != 0 && obj->screen_ypos > 112) {
		g_vars.screen_scrolling_dirmask &= ~8;
	} else if ((g_vars.screen_scrolling_dirmask & 4) != 0 && obj->screen_ypos < 128) {
		g_vars.screen_scrolling_dirmask &= ~4;
	}
}

static void do_level_update_object_bounds(struct object_t *obj) {
	int _si = g_vars.objects[OBJECT_NUM_PLAYER1].screen_xpos - obj->screen_xpos;
	int _di = g_vars.objects[OBJECT_NUM_PLAYER1].screen_ypos - obj->screen_ypos;
	if (g_vars.objects[OBJECT_NUM_PLAYER1].special_anim == 16) {
		_si = GAME_SCREEN_W;
		_di = GAME_SCREEN_H;
	}
	if (g_vars.two_players_flag) {
		int tmp_dx = g_vars.objects[OBJECT_NUM_PLAYER2].screen_xpos - obj->screen_xpos;
		int tmp_dy = g_vars.objects[OBJECT_NUM_PLAYER2].screen_ypos - obj->screen_ypos;
		if (g_vars.objects[OBJECT_NUM_PLAYER2].special_anim == 16) {
			tmp_dx = GAME_SCREEN_W;
			tmp_dy = GAME_SCREEN_H;
		}
		if (abs(_si) >= abs(tmp_dx) && abs(_di) >= abs(tmp_dy)) {
			obj->player_xdist = tmp_dx;
			obj->player_ydist = tmp_dy;
			// obj->unk41 = 1;
			return;
		}
	}
	obj->player_xdist = _si;
	obj->player_ydist = _di;
	// obj->unk41 = 0;
}

void do_level_update_projectile(struct object_t *obj) {
	struct object_t *obj38 = &g_vars.objects[38];
	if (obj->unk42 == 0) {
		if (obj38->type == 100) {
			obj->special_anim = 2;
			obj->unk42 = 1;
		}
	} else if (obj->unk42 == 1) {
		if (obj38->grab_type == 10 || obj->anim_num >= 7) {
			obj38->type = 2;
			obj38->visible_flag = 1;
			static const uint8_t data[MAX_OBJECTS] = {
				0, 0, 0, 0, 4, 5, 0, 0, 0, 0, 0, 0, 0, 6, 23, 0, 9, 9, 0, 10, 0,
				11, 0, 12, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 0, 0, 0, 0
			};
			obj38->grab_type = data[obj->type];
			if (obj->anim_num == 7) {
				if (obj38->grab_type == 10) {
					play_sound(SOUND_16);
				} else {
					play_sound(SOUND_4);
				}
			}
			switch (obj38->grab_type) {
			case 10:
				obj38->xpos = obj->xpos;
				obj38->ypos = obj->ypos;
				obj38->xvelocity = 0;
				break;
			case 11:
			case 13:
				obj38->ypos = obj->ypos - 10;
				obj38->xpos = obj->xpos + 32 - (obj->facing_left << 6);
				obj38->xvelocity = obj38->yvelocity = 64 - (obj->facing_left << 7);
				break;
			case 12:
				obj38->ypos = obj->ypos - 3;
				obj38->xpos = obj->xpos + 16 - (obj->facing_left << 5);
				obj38->xvelocity = obj38->yvelocity = 32 - (obj->facing_left << 6);
				break;
			default:
				obj38->ypos = obj->ypos - 18;
				obj38->xpos = obj->xpos + 32 - (obj->facing_left << 6);
				obj38->xvelocity = obj38->yvelocity = 64 - (obj->facing_left << 7);
				break;
			}
			const int num = obj38->type * 116 / 4;
			assert(num < 1189);
			obj38->animframes_ptr = animframes_059d + num;
			obj38->anim_num = 1;
			obj38->facing_left = obj->facing_left;
			obj->unk42 = 0;
			obj38->unk42 = 2;
			obj38->unk5D = obj->type;
		}
	} else if (obj->unk42 == 2) {
		if (obj->grab_type == 10) {
			obj->yacc = 4;
			obj->yvelocity = 4;
			if (obj->screen_ypos > 150) {
				obj->type = 0;
				obj->visible_flag = 0;
				obj->grab_type = 0;
				obj->unk42 = 0;
				obj->unk5D = 0;
			}
		} else {
			obj->direction_lr = obj->facing_left + 1;
			obj->xacc = 64;
			obj->xmaxvelocity = 400;
			obj->yvelocity = 0;
			obj->yacc = 0;
			if (obj->screen_xpos < 0 || obj->screen_xpos > GAME_SCREEN_W - 10 || obj->xpos < 32) {
				obj->type = 100;
				obj->visible_flag = 0;
				obj->grab_type = 0;
				obj->unk42 = 0;
				obj->unk5D = 0;
			}
		}
	}
}

static void do_level_fixup_object_position(struct object_t *obj) {
	int num, ret;

	num = triggers_get_tile_type(obj->xpos16, obj->ypos16);
	if (obj->grab_state != 0) {
		num = 0;
	}
	switch (num) {
	case 2:
	case 14:
		ret = triggers_tile_get_yoffset(obj);
		obj->ypos = ret + (obj->ypos & ~15);
		obj->ypos16 = obj->ypos >> 4;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		break;
	case 3:
	case 15:
		--obj->ypos16;
		ret = triggers_tile_get_yoffset(obj);
		obj->ypos = ret + (obj->ypos & ~15) - 16;
		obj->ypos16 = obj->ypos >> 4;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		break;
	}
}

// gravity
static void do_level_fixup_object_unkA(struct object_t *obj) {
	int16_t _si = triggers_get_tile_type(obj->xpos16, obj->ypos16);
	if (obj->special_anim == 18) {
		_si = 0;
		obj->sprite_type = 0;
	}
	if (_si == 6) {
		obj->yfriction = 0;
		obj->sprite_type = 3;
		if (obj->direction_ud == 1) {
			if (triggers_get_tile_type(obj->xpos16, (obj->ypos - 2) >> 4) != 6) {
				obj->yvelocity = -1;
			} else {
				obj->yvelocity = -2;
			}
		} else if (obj->direction_ud == 2) {
			obj->yvelocity = 2;
		} else {
			obj->yvelocity = 0;
		}
	} else if (_si == 7) {
		if (obj->direction_ud != 0 && obj->unk2E != 0) {
			obj->xpos = (obj->xpos + 7) & ~15;
			obj->screen_xpos = obj->xpos - g_vars.screen_tilemap_xorigin;
			obj->xpos16 = obj->xpos >> 4;
		}
		obj->yfriction = 0;
		obj->sprite_type = 2;
		if (obj->direction_ud == 1) {
			if (triggers_get_tile_type(obj->xpos16, (obj->ypos - 2) >> 4) != 7) {
				obj->yvelocity = -1;
			} else {
				obj->yvelocity = -2;
			}
		} else if (obj->direction_ud == 2) {
			obj->yvelocity = 2;
		}
	} else {
		if (_si == 11 || _si == 12) {
			obj->yfriction = 0;
			obj->sprite_type = 4;
		}
		if (obj->yfriction != 0) {
			obj->yvelocity -= obj->yfriction;
			obj->yfriction = 0;
		} else {
			obj->yvelocity += obj->yacc;
			if (obj->type < 2) {
				if (obj->unk3D == 1 && obj->yvelocity > 0) {
					obj->yacc = 1;
					obj->yvelocity = obj->yacc;
				} else if (obj->unk3D == 2) {
					obj->yacc = -1;
					obj->yvelocity = obj->yacc;
				}
			}
			if (obj->yvelocity > obj->ymaxvelocity) {
				obj->yvelocity = obj->ymaxvelocity;
			}
		}
	}
}

static void do_level_fixup_object_ypos16(struct object_t *obj) {
	if (obj->collide_flag == 0) {
		if (obj->ypos < 32) {
			obj->ypos = 32;
			obj->screen_ypos = 32 - g_vars.screen_tilemap_yorigin;
			obj->yvelocity = 0;
		} else if (obj->ypos > g_vars.screen_tilemap_h && obj->unk53 == 0) {
			obj->ypos = g_vars.screen_tilemap_h - 16;
			obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
			obj->yvelocity = 0;
		} else {
			obj->ypos16 = obj->ypos >> 4;
		}
	}
}

// collision with bonus
static void do_level_handle_object_bonus_collision(struct object_t *obj, struct object_t *test_obj) {
	if (obj->yvelocity < 0) {
		return;
	}
	if (test_obj->xpos + level_obj_w[test_obj->type] <= obj->xpos) {
		return;
	}
	if (test_obj->xpos - level_obj_w[test_obj->type] >= obj->xpos) {
		return;
	}
	if (test_obj->ypos - level_obj_h[test_obj->type] >= obj->ypos) {
		return;
	}
	if (obj->ypos >= test_obj->ypos) {
		return;
	}
	obj->yvelocity = test_obj->yvelocity;
	obj->yacc = test_obj->yacc;
	obj->unk1E = test_obj->unk1E;
	obj->unk1C = test_obj->xvelocity >> 3;
	obj->sprite_type = 1;
	obj->ypos = test_obj->ypos - level_obj_h[test_obj->type] + 1;
	obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
	obj->ypos16 = obj->ypos >> 4;
	if ((obj->direction_ud & 2) != 0 && obj->grab_state == 0) {
		obj->unk2E = 0;
	}
}

void do_level_player_hit(struct object_t *obj) {
	if (obj->blinking_counter == 0) {
		if (obj->data51 > 0 && (g_options.cheats & CHEATS_UNLIMITED_ENERGY) == 0) {
			--obj->data51;
			do_level_update_panel_lifes(obj);
		}
		if (obj->special_anim != 16) {
			obj->blinking_counter = 75;
		}
	}
}

static void do_level_handle_object_collision(struct object_t *obj) {
	struct object_t *obj35 = &g_vars.objects[35];
	if (obj->special_anim == 16) {
		return;
	}
	if (obj->type != 0 && g_vars.two_players_flag) {
		obj35 = &g_vars.objects[34];
	}
	if (obj->type < 2) {
		level_obj_h[obj->type] = (obj->unk2E == 0) ? 14 : 26;
	}
	if (obj->unk2F != 0) {
		return;
	}
	for (int i = 0; i < MAX_OBJECTS; ++i) {
		struct object_t *current_obj = &g_vars.objects[i];
		if (!current_obj->visible_flag || current_obj->type == 100) {
			continue;
		}
		if (current_obj->type == obj->type) {
			continue;
		}
		const int current_obj_type = level_obj_type[current_obj->type];
		if (current_obj_type == 0) {
			continue;
		}
		if (current_obj_type == 1) {
			do_level_handle_object_bonus_collision(obj, current_obj);
			continue;
		}
		if (current_obj_type == 2) {
			if (current_obj->collide_flag != 0 || current_obj->special_anim == 16) {
				continue;
			}
			int x2 = obj->xpos + level_obj_w[obj->type];
			int x1 = current_obj->xpos - level_obj_w[current_obj->type];
			if (x2 < x1) {
				continue;
			}
			int y1 = current_obj->ypos - level_obj_h[current_obj->type];
			if (y1 > obj->ypos) {
				continue;
			}
			x1 = obj->xpos - level_obj_w[obj->type];
			x2 = current_obj->xpos + level_obj_w[current_obj->type];
			if (x1 > x2) {
				continue;
			}
			int y2 = obj->ypos - level_obj_h[obj->type];
			if (y2 > current_obj->ypos) {
				continue;
			}
			if (obj->type == 0 || obj->type == 1) {
				if (obj->blinking_counter == 0 && current_obj->type > 2 && obj->special_anim != 18 && !(g_options.cheats & CHEATS_NO_HIT)) {
					if (obj->grab_state != 0) {
						do_level_drop_grabbed_object(obj);
					}
					if (obj->sprite_type != 4) {
						obj->special_anim = 18;
					}
					do_level_player_hit(obj);
					play_sound(SOUND_11);
				}
			} else if (obj->type == 2) {
				if (current_obj->type < 2) {
					if (current_obj->blinking_counter == 0 && obj->unk5D > 1 && current_obj->special_anim != 18 && !(g_options.cheats & CHEATS_NO_HIT)) {
						if (current_obj->grab_state != 0) {
							do_level_drop_grabbed_object(current_obj);
						}
						if (current_obj->sprite_type != 4) {
							current_obj->special_anim = 18;
						}
						do_level_player_hit(current_obj);
						play_sound(SOUND_11);
					}
				} else if (current_obj->type > 2) {
					if ((g_vars.objects[OBJECT_NUM_PLAYER1].grab_state == 0 && obj->unk5D == 0) || (g_vars.objects[OBJECT_NUM_PLAYER2].grab_state == 0 && obj->unk5D == 1)) {
						if (obj35->type == 100) {
							current_obj->collide_flag = 1;
							current_obj->xmaxvelocity = 60;
							if (obj->facing_left) {
								current_obj->xvelocity = -60;
								current_obj->data51 = 1;
							} else {
								current_obj->xvelocity = 60;
								current_obj->data51 = 2;
							}
							current_obj->facing_left ^= 1;
							if (current_obj->yvelocity >= 0) {
								current_obj->yfriction = 12;
							}
							obj->type = 100;
							obj->visible_flag = 0;
							play_sound(SOUND_11);
						}
					}
				}
			}
		}
	}
}

static void do_level_handle_object_unk59(struct object_t *obj) {
	if (obj->collide_flag == 0) {
		if (obj->type != 2) {
			if (obj->sprite_type == 0) {
				if (obj->yvelocity < 0) {
					level_call_trigger_func(obj, -2);
				} else {
					level_call_trigger_func(obj, 0);
				}
			} else {
				if (obj->yvelocity < 0) {
					level_call_trigger_func(obj, -2);
				} else {
					if (triggers_get_tile_type(obj->xpos16, obj->ypos16) != 7) {
						if (triggers_get_tile_type(obj->xpos16, obj->ypos16) != 6) {
							level_call_trigger_func(obj, 0);
						}
					}
				}
			}
		}
		if (obj->type < 3) {
			do_level_handle_object_collision(obj);
		}
	}
}

static void do_level_fixup_object_unk8(struct object_t *obj) {
	if (obj->direction_lr & OBJECT_DIRECTION_RIGHT) {
		obj->facing_left = 0;
		if (obj->xvelocity < obj->xmaxvelocity) {
			obj->xacc = obj->unk2D;
		} else {
			obj->xacc = 0;
		}
	} else if (obj->direction_lr & OBJECT_DIRECTION_LEFT) {
		obj->facing_left = 1;
		if (obj->xvelocity > -obj->xmaxvelocity) {
			obj->xacc = -obj->unk2D;
		} else {
			obj->xacc = 0;
		}
	} else {
		if (obj->xvelocity < 0) {
			obj->xacc = obj->unk2D;
			if (obj->xvelocity > -obj->xacc) {
				obj->xacc = -obj->xvelocity;
			}
		} else if (obj->xvelocity > 0) {
			obj->xacc = -obj->unk2D;
			if (obj->xvelocity < -obj->xacc) {
				obj->xacc = -obj->xvelocity;
			}
		} else {
			obj->xacc = 0;
		}
	}
	obj->xvelocity += obj->xacc + obj->unk2F;
	if (obj->xvelocity > obj->xmaxvelocity) {
		obj->xvelocity = obj->xmaxvelocity;
	}
	if (obj->xvelocity < -obj->xmaxvelocity) {
		obj->xvelocity = -obj->xmaxvelocity;
	}
}

static void do_level_fixup_object_xpos16(struct object_t *obj) {
	int _ax, _si;

	_ax = obj->xvelocity + (obj->unk1C << 3);
	if (_ax >= 0) {
		_si = (obj->xvelocity >> 3) + obj->unk1C;
		obj->tile012_xpos = (obj->xpos + _si + 8) >> 4;
		obj->tile0_flags = triggers_get_next_tile_flags(obj->tile012_xpos, obj->ypos16);
		obj->tile1_flags = triggers_get_next_tile_flags(obj->tile012_xpos, obj->ypos16 - 1);
		obj->tile2_flags = triggers_get_next_tile_flags(obj->tile012_xpos, obj->ypos16 - 2);
		if (obj->type != 2) {
			if ((obj->tile0_flags & 1) != 0) {
				obj->moving_direction ^= 1;
				if (obj->sprite_type == 2) {
					obj->direction_ud = 1;
				}
				if (obj->xvelocity > 0) {
					obj->xvelocity = 0;
				}
				_si = 0;
			} else if (obj->unk2E == 1 && (obj->tile1_flags & 1) != 0) {
				obj->moving_direction ^= 1;
				if (obj->xvelocity > 0) {
					obj->xvelocity = 0;
				}
				_si = 0;
			} else if (obj->unk2E == 2 && (obj->tile2_flags & 1) != 0) {
				if (obj->xvelocity > 0) {
					obj->xvelocity = 0;
				}
				_si = 0;
			}
		}
		if (obj->xpos < g_vars.screen_tilemap_w * 16 - 16) {
			obj->xpos += _si;
			obj->screen_xpos += _si;
			obj->xpos16 = obj->xpos >> 4;
		}
	} else {
		if (obj->xvelocity < -8) {
			_si = (obj->xvelocity >> 3) + obj->unk1C;
		} else {
			_si = obj->unk1C;
		}
		obj->tile012_xpos = (obj->xpos + _si - 8) >> 4;
		obj->tile0_flags = triggers_get_next_tile_flags(obj->tile012_xpos, obj->ypos16);
		obj->tile1_flags = triggers_get_next_tile_flags(obj->tile012_xpos, obj->ypos16 - 1);
		obj->tile2_flags = triggers_get_next_tile_flags(obj->tile012_xpos, obj->ypos16 - 2);
		if (obj->type != 2) {
			if ((obj->tile0_flags & 2) != 0) {
				obj->moving_direction ^= 1;
				if (obj->sprite_type == 2) {
					obj->direction_ud = 1;
				}
				if (obj->xvelocity < 0) {
					obj->xvelocity = 0;
				}
				_si = 0;
			} else if (obj->unk2E == 1 && (obj->tile1_flags & 2) != 0) {
				obj->moving_direction ^= 1;
				if (obj->xvelocity < 0) {
					obj->xvelocity = 0;
				}
				_si = 0;
			} else if (obj->unk2E == 2 && (obj->tile2_flags & 2) != 0) {
				if (obj->xvelocity < 0) {
					obj->xvelocity = 0;
				}
				_si = 0;
			}
		}
		if (obj->xpos > 16) {
			obj->xpos += _si;
			obj->screen_xpos += _si;
			obj->xpos16 = obj->xpos >> 4;
		}
	}
}

static void do_level_update_object_unk2E(struct object_t *obj) {
	if (obj->direction_ud & 2) {
		if (obj->grab_state == 0) {
			if (triggers_get_tile_data(obj) != 0) {
				obj->unk2E = 0;
			}
		}
	} else {
		if (obj->tile1_flags == 0 && obj->unk2E == 0) {
			obj->unk2E = 1;
		}
	}
	const int num = triggers_get_tile_type(obj->xpos16, obj->ypos16);
	if (num == 11 || num == 12) {
		obj->unk2E = 0;
	}
}

static void do_level_update_objects() {
	const int bounding_box_x1 = g_vars.screen_tilemap_xorigin - 140;
	const int bounding_box_x2 = g_vars.screen_tilemap_xorigin + TILEMAP_SCREEN_W + 140;
	const int bounding_box_y1 = g_vars.screen_tilemap_yorigin - 100;
	const int bounding_box_y2 = g_vars.screen_tilemap_yorigin + TILEMAP_SCREEN_H + 190;
	for (int i = 0; i < MAX_OBJECTS; ++i) {
		struct object_t *obj = &g_vars.objects[i];
		print_debug(DBG_GAME, "update_objects #%d type %d pos %d,%d", i, obj->type, obj->xpos, obj->ypos);
		obj->screen_xpos = obj->xpos - g_vars.screen_tilemap_xorigin;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->unk2B = 0;
		obj->direction_lr = 0;
		obj->direction_ud = 0;
		if (obj->type < 2) {
			if (g_vars.inp_keyboard[0xC1] != 0) { // F7, change player
				if (!g_vars.switch_player_scrolling_flag && g_vars.two_players_flag) {
					if (!g_vars.player2_scrolling_flag && g_vars.objects[OBJECT_NUM_PLAYER1].unk53 == 0) {
						g_vars.player2_scrolling_flag = 1;
						g_vars.switch_player_scrolling_flag = 1;
					} else if (g_vars.player2_scrolling_flag && g_vars.objects[OBJECT_NUM_PLAYER2].unk53 == 0) {
						g_vars.player2_scrolling_flag = 1;
						g_vars.switch_player_scrolling_flag = 1;
					}
				}
				g_vars.inp_keyboard[0xC1] = 0;
			}
			if (obj->blinking_counter != 0) {
				--obj->blinking_counter;
			}
			obj->yacc = 2;
			if (i == OBJECT_NUM_PLAYER1) {
				if (!g_vars.player2_dead_flag) {
					do_level_update_input(obj);
					do_level_update_object_unk2E(obj);
					do_level_update_panel_vinyls(obj);
				}
				if (!g_vars.player2_scrolling_flag) {
					do_level_update_scrolling(obj);
				}
				if (obj->data5F != 0 && !g_vars.found_music_instrument_flag) {
					// found music instrument
					if (g_vars.level < (MAX_LEVELS - 1)) {
						static const uint8_t data[] = { 94, 81, 96, 26, 26, 3, 89, 52, 15, 96, 51, 20, 72, 27, 5 };
						const uint8_t *p = data + g_vars.level * 3;
						do_level_update_tile(p[0], p[1], p[2]);
					}
					g_vars.found_music_instrument_flag = 1;
				}
			} else if (g_vars.two_players_flag) {
				if (!g_vars.player1_dead_flag) {
					do_level_update_object_unk2E(obj);
					do_level_update_panel_2nd_player();
				}
				if (g_vars.player2_scrolling_flag) {
					do_level_update_scrolling(obj);
				}
			}
		} else if (obj->type > 2 && obj->collide_flag == 0) {
			const int x = g_vars.level_xpos[i];
			const int y = g_vars.level_ypos[i];
			if (obj->type == 32) {
				obj->unk5D = i;
			}
			if (x > bounding_box_x1 && x < bounding_box_x2 && y > bounding_box_y1 && y < bounding_box_y2) {
				obj->visible_flag = 1;
				level_call_object_func(obj);
			} else {
				obj->visible_flag = 0;
				if (obj->type == 22 || obj->type == 35) {
					obj->xpos = x;
					obj->ypos = y;
				}
			}
		}
		if (obj->visible_flag != 0) {
			do_level_update_object_bounds(obj);
			if (obj->unk42 != 0) {
				do_level_update_projectile(obj);
			}
			obj->unk2D = 10;
			obj->unk1C = 0;
			if (obj->sprite_type != 6) {
				obj->sprite_type = 0;
			}
			do_level_fixup_object_position(obj);
			do_level_fixup_object_unkA(obj);
			obj->ypos += obj->yvelocity;
			obj->screen_ypos += obj->yvelocity;
			do_level_fixup_object_ypos16(obj);
			do_level_handle_object_unk59(obj);
			do_level_fixup_object_unk8(obj);
			do_level_fixup_object_xpos16(obj);
			if (obj->grab_state != 0) {
				do_level_update_grabbed_object(obj);
			}
			if (obj->collide_flag != 0) {
				if (obj->screen_ypos > 200) {
					obj->visible_flag = 0;
				}
				obj->xmaxvelocity = 60;
				if (obj->data51 == 1) {
					obj->xvelocity = -60;
				} else {
					obj->xvelocity = 60;
				}
				obj->facing_left ^= 1;
				obj->special_anim = 1;
			}
			obj->floor_ypos16 = obj->ypos16;
			if (obj->type != 2) {
				if (obj->special_anim != 0) {
					do_level_add_sprite3(obj);
				} else {
					do_level_add_sprite1(obj);
				}
			} else {
				do_level_add_sprite2(obj);
			}
		}
	}
	if (g_vars.triggers_counter != 0) {
		if (g_vars.triggers_counter < 10) {
			screen_draw_frame(g_res.spr_frames[140 + g_vars.level], 12, 16, g_vars.level * 32 + 80, -12);
			++g_vars.triggers_counter;
		} else {
			screen_draw_frame(g_res.spr_frames[145], 12, 16, g_vars.level * 32 + 80, -12);
			++g_vars.triggers_counter;
			if (g_vars.triggers_counter > 20) {
				g_vars.triggers_counter = 1;
			}
		}
	}
	if (!g_vars.two_players_flag && g_vars.objects[OBJECT_NUM_PLAYER1].level_complete_flag != 0) {
		g_vars.level_completed_flag = 1;
	} else {
		if (g_vars.objects[OBJECT_NUM_PLAYER1].level_complete_flag != 0 && g_vars.objects[OBJECT_NUM_PLAYER2].level_complete_flag != 0) {
			g_vars.level_completed_flag = 1;
		}
		if (g_vars.objects[OBJECT_NUM_PLAYER1].level_complete_flag != 0 && g_vars.player1_dead_flag) {
			g_vars.level_completed_flag = 1;
		}
		if (g_vars.objects[OBJECT_NUM_PLAYER2].level_complete_flag != 0 && g_vars.player2_dead_flag) {
			g_vars.level_completed_flag = 1;
		}
	}
	if (g_vars.level_completed_flag) {
		++g_vars.update_objects_counter;
		if (g_vars.update_objects_counter > 50) {
			if (g_vars.level < (MAX_LEVELS - 1)) {
				++g_vars.level;
			} else {
				g_vars.level = 0;
				g_vars.play_level_flag = 0;
				g_vars.level_completed_flag = 0;
			}
			g_vars.objects[OBJECT_NUM_PLAYER1].unk60 = 0;
			g_vars.objects[OBJECT_NUM_PLAYER2].unk60 = 0;
			g_vars.objects[OBJECT_NUM_PLAYER1].data5F = 0;
			g_vars.objects[OBJECT_NUM_PLAYER2].data5F = 0;
			g_vars.objects[OBJECT_NUM_PLAYER1].level_complete_flag = 0;
			g_vars.objects[OBJECT_NUM_PLAYER2].level_complete_flag = 0;
			// g_vars.screen_draw_offset -= TILEMAP_OFFSET_Y * 40;
			screen_unk5();
			// g_vars.screen_draw_offset += TILEMAP_OFFSET_Y * 40;
			g_vars.quit_level_flag = 1;
		}
	} else if (g_vars.game_over_flag) {
		g_vars.screen_scrolling_dirmask = 0;
		if ((!g_vars.two_players_flag && g_vars.objects[OBJECT_NUM_PLAYER1].lifes_count == 1) || (g_vars.player2_dead_flag && g_vars.player1_dead_flag)) {
			screen_draw_frame(g_res.spr_frames[127], 34, 160, 80, 63);
			g_vars.objects[OBJECT_NUM_PLAYER1].unk60 = 0;
			g_vars.objects[OBJECT_NUM_PLAYER2].unk60 = 0;
			g_vars.objects[OBJECT_NUM_PLAYER1].data5F = 0;
			g_vars.objects[OBJECT_NUM_PLAYER2].data5F = 0;
			g_vars.objects[OBJECT_NUM_PLAYER1].restart_level_flag = 0;
			g_vars.objects[OBJECT_NUM_PLAYER2].restart_level_flag = 0;
			g_vars.objects[OBJECT_NUM_PLAYER1].level_complete_flag = 0;
			g_vars.objects[OBJECT_NUM_PLAYER2].level_complete_flag = 0;
			screen_vsync();
		}
		if (g_vars.objects[35].screen_ypos > 90) {
			g_vars.objects[35].screen_ypos = 90;
			g_vars.objects[35].ypos = g_vars.screen_tilemap_yorigin + 90;
		}
		++g_vars.update_objects_counter;
		if (g_vars.objects[OBJECT_NUM_PLAYER1].yvelocity == 0) {
			g_vars.objects[OBJECT_NUM_PLAYER1].special_anim = 21;
		} else {
			g_vars.objects[OBJECT_NUM_PLAYER1].special_anim = 2;
		}
		if (g_vars.update_objects_counter > 70) {
			if (!g_vars.two_players_flag && g_vars.objects[OBJECT_NUM_PLAYER1].lifes_count == 1) {
				g_vars.play_level_flag = 0;
			}
			// g_vars.screen_draw_offset -= TILEMAP_OFFSET_Y * 40;
			screen_unk5();
			// g_vars.screen_draw_offset += TILEMAP_OFFSET_Y * 40;
			g_vars.quit_level_flag = 1;
		}
	}
}

static void draw_foreground_tiles() {
	int x = g_vars.screen_tilemap_xorigin >> 4;
	int y = g_vars.screen_tilemap_yorigin >> 4;
	for (int j = 0; j < (TILEMAP_SCREEN_H / 16); ++j) {
		const uint8_t *ptr = lookup_sql(x, y);
		for (int i = 0; i < (TILEMAP_SCREEN_W / 16); ++i) {
			uint8_t num = *ptr++;
			const uint8_t *data = g_res.triggers[num].op_table2;
			if (data) {
				const int current = data[0];
				num = data[current + 1];
			}
			const int avt_num = g_res.triggers[num].foreground_tile_num;
			if (avt_num != 255) {
				const int tile_y = g_vars.screen_tilemap_yoffset + j * 16 + TILEMAP_OFFSET_Y;
				const int tile_x = g_vars.screen_tilemap_xoffset + i * 16;
				g_sys.render_add_sprite(RENDER_SPR_FG, avt_num, tile_x, tile_y, 0);
			}
		}
		++y;
	}
}

void do_level() {
	static const int W = 320 / 16;
	for (int tile_num = 0; tile_num < 128; ++tile_num) {
		g_vars.screen_tile_lut[tile_num] = (tile_num / W) * 640 + (tile_num % W);
	}
	g_vars.screen_tilemap_w = level_dim[g_vars.level * 2];
	g_vars.screen_tilemap_h = level_dim[g_vars.level * 2 + 1];
	for (int i = 0; i < MAX_TRIGGERS; ++i) {
		g_res.triggers[i].unk16 = i;
	}
	g_vars.switch_player_scrolling_flag = 0;
	init_level();
	screen_clear(0);
	do_level_redraw_tilemap(g_vars.screen_tilemap_xorigin, g_vars.screen_tilemap_yorigin);
	if (g_options.amiga_copper_bars) {
		g_sys.set_copper_bars(_copper_data + g_vars.level * 18);
	}
	g_vars.inp_keyboard[0xB9] = 0; // SPACE
	// g_vars.screen_draw_offset = TILEMAP_OFFSET_Y * 40;
	g_vars.update_objects_counter = 0;
	g_vars.game_over_flag = 0;
	g_vars.level_completed_flag = 0;
	g_vars.quit_level_flag = 0;
	g_vars.player2_scrolling_flag = 0;
	g_vars.found_music_instrument_flag = 0;
	g_sys.render_set_sprites_clipping_rect(0, TILEMAP_OFFSET_Y, TILEMAP_SCREEN_W, TILEMAP_SCREEN_H);
	bool screen_transition_flag = true;
	do {
		const uint32_t timestamp = g_sys.get_timestamp();
		update_input();
		if (g_vars.inp_keyboard[0xBF] != 0) { // F5, quit game
			play_sound(SOUND_0);
			g_vars.inp_keyboard[0xBF] = 0;
			g_vars.quit_level_flag = 1;
			g_vars.play_level_flag = 0;
			g_vars.objects[OBJECT_NUM_PLAYER1].unk60 = 0;
			g_vars.objects[OBJECT_NUM_PLAYER2].unk60 = 0;
			g_vars.objects[OBJECT_NUM_PLAYER1].data5F = 0;
			g_vars.objects[OBJECT_NUM_PLAYER2].data5F = 0;
			if (!g_vars.play_demo_flag) {
				g_vars.start_level = 0;
			}
		} else if (g_vars.inp_keyboard[0xC4] != 0) { // F10
			play_sound(SOUND_0);
			while (g_vars.inp_keyboard[0xC4] != 0 && g_vars.inp_keyboard[0xB] == 0);
			while (g_vars.inp_keyboard[0xC4] != 0 && g_vars.inp_keyboard[0xB] != 0);
		}
		// demo
		do_level_update_scrolling2();
		do_level_update_objects();
		screen_redraw_sprites();
		if (!g_res.amiga_data) {
			draw_foreground_tiles();
		}
		++g_vars.level_loop_counter;
		draw_level_panel();

		if (screen_transition_flag) {
			screen_transition_flag = false;
			g_sys.update_screen(g_res.vga, 0);
			screen_do_transition2();
		} else {
			g_sys.update_screen(g_res.vga, 1);
		}
		memset(g_res.vga, 0, GAME_SCREEN_W * GAME_SCREEN_H);

		const int diff = (timestamp + (1000 / 30)) - g_sys.get_timestamp();
		g_sys.sleep(diff < 10 ? 10 : diff);
		screen_clear_sprites();

	} while (!g_sys.input.quit && !g_vars.quit_level_flag);
	// g_vars.screen_draw_offset -= TILEMAP_OFFSET_Y * 40;
	screen_unk5();
	if (g_options.amiga_copper_bars) {
		g_sys.set_copper_bars(0);
	}
	g_sys.render_set_sprites_clipping_rect(0, 0, GAME_SCREEN_W, GAME_SCREEN_H);
	g_vars.inp_keyboard[0xBF] = 0;
}

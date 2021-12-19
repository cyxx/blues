
/* level main loop */

#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

static const bool _score_8_digits = true; /* maximum score 99.999.999 */

static const bool _demo_inputs = false;

static const bool _expert = true;

static const bool _redraw_tilemap = true;
static const bool _redraw_panel = true;

static const uint16_t _undefined = 0x55AA;

static void level_completed_bonuses_animation();
static void level_player_death_animation();

static const uint8_t next_level_tbl[] = { 0xFF, 0x0C, 0x0B, 0x0A, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x0E };

static void set_level_palette() {
	g_sys.set_screen_palette(palettes_tbl[g_vars.level_num], 0, 16, 6);
}

static int load_level_data_get_tilemap_size(int num, const uint8_t *lev, const uint8_t *uni) {
	static const uint8_t level_height_tbl[] = { 0x31, 0x68, 0x31, 0x2D, 0x80, 0x80, 0x80, 0x56, 0x6E, 0x0C, 0x18, 0x33, 0x33, 0x26, 0xAD, 0x54 };
	g_vars.tilemap.h = level_height_tbl[num];
	const int tilemap_size = level_height_tbl[num] << 8;
	g_vars.tilemap.size = tilemap_size;
	int offset = 0;
	for (int i = 0; i < 512; i += 2) {
		const uint16_t num = READ_LE_UINT16(lev + tilemap_size + i);
		if (num == 0xFFFF) {
			continue;
		}
		if (num < 0x100) {
			offset += 128;
		} else {
			/* 'UNION' tile */
		}
	}
	return tilemap_size + 512 + offset;
}

static bool level_check_tilemap_offset(uint16_t offset) {
	const uint8_t x = offset & 255;
	const uint8_t y = offset >> 8;
	if (y >= g_vars.tilemap.h) {
		print_debug(DBG_GAME, "invalid tilemap offset %d (%d,%d)", (int16_t)offset, x, y);
		return false;
	}
	return true;
}

uint8_t level_get_tile(uint16_t offset) {
	if (level_check_tilemap_offset(offset)) {
		return g_res.leveldat[offset];
	}
	return 0;
}

static void level_set_tile(uint16_t offset, uint8_t tile_num) {
	if (level_check_tilemap_offset(offset)) {
		g_res.leveldat[offset] = tile_num;
	}
}

static void load_level_data_init_animated_tiles() {
	for (int i = 0; i < 256; ++i) {
		if ((g_res.level.tile_attributes2[i] & 0x80) == 0) { /* not animated */
			g_vars.tile_tbl1[i] = i;
			g_vars.tile_tbl2[i] = i;
			g_vars.tile_tbl3[i] = i;
			g_vars.animated_tile_flag_tbl[i] = 0;
		} else {
			int j = i;
			g_vars.tile_tbl1[i] = j;
			g_vars.tile_tbl2[i + 2] = j;
			g_vars.tile_tbl3[i + 1] = j;
			++j;
			g_vars.tile_tbl1[i + 1] = j;
			g_vars.tile_tbl2[i] = j;
			g_vars.tile_tbl3[i + 2] = j;
			++j;
			g_vars.tile_tbl1[i + 2] = j;
			g_vars.tile_tbl2[i + 1] = j;
			g_vars.tile_tbl3[i] = j;

			g_vars.animated_tile_flag_tbl[i] = 1;
			g_vars.animated_tile_flag_tbl[i + 1] = 1;
			g_vars.animated_tile_flag_tbl[i + 2] = 1;

			i += 2;
		}
	}
	g_vars.level_animated_tiles_current_tbl = g_vars.tile_tbl1;
}

static void load_level_data_init_transparent_tiles() {
	const int tiles_offset = g_vars.tilemap.size + 512;
	const int tiles_count = (g_res.levellen - tiles_offset) / 128;
	int count = 1;
	for (int i = 0; i < tiles_count; ++i) {
		const uint8_t num = i;
		const uint8_t *tiledat = g_res.leveldat + tiles_offset + num * 128;
		uint8_t mask_opaque = 0xFF;
		uint8_t mask_transparent = 0;
		for (int j = 0; j < 128; ++j) {
			mask_opaque &= tiledat[j];
			mask_transparent |= tiledat[j];
		}
		if (mask_transparent == 0) {
		} else if (mask_opaque == 0xFF) {
			g_vars.tilemap.redraw_flag2 = 1;
		} else {
			++count;
			g_vars.tilemap.redraw_flag2 = 1;
		}
	}
}

static void load_level_data_fix_items_spr_num() {
	const int16_t offset = g_res.level.items_spr_num_offset;
	for (int i = 0; i < MAX_LEVEL_ITEMS; ++i) {
		const uint16_t num = g_res.level.items_tbl[i].spr_num;
		if (num != 0xFFFF) {
			g_res.level.items_tbl[i].spr_num = num - offset + 53;
		}
	}
	for (int i = 0; i < MAX_LEVEL_PLATFORMS; ++i) {
		const uint16_t num = g_res.level.platforms_tbl[i].spr_num;
		if (num != 0xFFFF) {
			g_res.level.platforms_tbl[i].spr_num = num - offset + 53;
		}
	}
}

static void load_level_data_fix_monsters_spr_num() {
	if (g_res.level.items_spr_num_offset != 0xFFFF) {
		for (int i = 0; i < g_res.level.monsters_count; ++i) {
			uint16_t num = g_res.level.monsters_tbl[i].spr_num;
			if (num != 0xFFFF) {
				if (num >= g_res.level.monsters_spr_num_offset) {
					num -= g_res.level.monsters_spr_num_offset;
					num += g_res.spr_monsters_offset;
					g_res.level.monsters_tbl[i].spr_num = num;
				} else if (num >= g_res.level.items_spr_num_offset) {
					num -= g_res.level.items_spr_num_offset;
					num += 53;
					g_res.level.monsters_tbl[i].spr_num = num;
				}
			}
		}
	}
	g_res.level.items_spr_num_offset = 53;
	g_res.level.monsters_spr_num_offset = g_res.spr_monsters_offset;
}

static void load_level_data_init_secret_bonus_tiles() {
	int count = 0;
	for (int i = 0; i < MAX_LEVEL_BONUSES; ++i) {
		struct level_bonus_t *bonus = &g_res.level.bonuses_tbl[i];
		const uint16_t offset = bonus->pos;
		if (offset != 0xFFFF) {
			const uint8_t tile_num = level_get_tile(offset);
			level_set_tile(offset, bonus->tile_num0);
			bonus->tile_num1 = tile_num;
			++count;
		}
	}
	g_vars.level_complete_secrets_count = count;
}

static int compare_level_item_x_pos(const void *item1, const void *item2) {
	return ((const struct level_item_t *)item1)->x_pos < ((const struct level_item_t *)item2)->x_pos;
}

static void load_level_data_init_password_items() {
	struct level_item_t *items_ptr_tbl[MAX_LEVEL_ITEMS];
	int count = 0;
	for (int i = 0; i < MAX_LEVEL_ITEMS; ++i) {
		struct level_item_t *item = &g_res.level.items_tbl[i];
		const uint16_t num = item->spr_num - 283;
		if (num > 15) { /* not 0-9A-F */
			continue;
		}
		items_ptr_tbl[count++] = item;
	}
	if (count == 0 || (count & 3) != 0) {
		return;
	}
	qsort(items_ptr_tbl, count, sizeof(struct level_item_t *), compare_level_item_x_pos);
	/* level password */
	const uint16_t r = random_get_number3(g_vars.level_num + (_expert ? 10 : 0));
	for (int j = 0; j < count; j += 4) {
		for (int i = 0; i < 4; ++i) {
			struct level_item_t *item = items_ptr_tbl[j + i];
			const int num = r >> ((3 - i) * 4);
			item->spr_num = 283 + (num & 15);
		}
	}
}

static void load_level_data_init_columns() {
	uint8_t *p = g_vars.columns_tiles_buf;
	for (int i = 0; i < MAX_LEVEL_COLUMNS; ++i) {
		struct level_column_t *column = &g_res.level.columns_tbl[i];
		if (column->tilemap_pos == 0xFFFF) {
			continue;
		}
		uint16_t offset = column->tilemap_pos - (column->y_target << 8);
		const int h = column->y_target + column->h;
		const int w = column->w;
		WRITE_LE_UINT16(p, offset); p += 2;
		*p++ = h;
		*p++ = w;
		for (int y = 0; y < h; ++y) {
			memcpy(p, g_res.leveldat + offset, w); p += w;
			offset += 0x100;
		}
		column->tiles_offset_buf = p - g_vars.columns_tiles_buf - w;
	}
	WRITE_LE_UINT16(p, 0xFFFF); p += 2;
	assert(p - g_vars.columns_tiles_buf <= sizeof(g_vars.columns_tiles_buf));
}

static void level_reset_columns_tiles() {
	const uint8_t *p = g_vars.columns_tiles_buf;
	while (1) {
		uint16_t offset = READ_LE_UINT16(p); p += 2;
		if (offset == 0xFFFF) {
			break;
		}
		const int h = *p++;
		const int w = *p++;
		for (int y = 0; y < h; ++y) {
			memcpy(g_res.leveldat + offset, p, w); p += w;
			offset += 0x100;
		}
	}
}

static void load_level_data(int num) {
	static const uint8_t num_tbl[] = { 0, 0, 0, 1, 1, 1, 2, 3, 3, 0, 4, 4, 4, 5, 0, 2 };
	char name[16];
	snprintf(name, sizeof(name), "BACK%d.SQZ", num_tbl[num]);
	uint8_t *data = load_file(name);
	if (data) {
		video_copy_img(data);
		free(data);
	}
	memset(&g_vars.objects_tbl[0], 0, sizeof(struct object_t) * OBJECTS_COUNT);
	const int level = (num < 9) ? ('1' + num) : ('A' + num - 9);
	snprintf(name, sizeof(name), "LEVEL%c.SQZ", level);
	free(g_res.leveldat);
	g_res.leveldat = load_file(name);
	g_res.levellen = g_uncompressed_size;
	const int offset = load_level_data_get_tilemap_size(num, g_res.leveldat, data);
	print_debug(DBG_GAME, "tilemap offset %d", offset);
	load_leveldat(g_res.leveldat + offset, 0);
	video_convert_tiles(g_res.leveldat + g_vars.tilemap.size + 512, offset - (g_vars.tilemap.size + 512));
	print_debug(DBG_GAME, "start_pos %d,%d", g_res.level.start_x_pos, g_res.level.start_y_pos);
	load_level_data_init_animated_tiles();
	load_level_data_init_transparent_tiles();
	g_vars.snow.pattern = (g_vars.level_num != 15) ? snow_pattern1_data : snow_pattern2_data;
	g_vars.snow.counter = 0;
	g_vars.tilemap_start_x_pos = g_res.level.start_x_pos;
	g_vars.tilemap_start_y_pos = g_res.level.start_y_pos;
	load_level_data_fix_items_spr_num();
	load_level_data_fix_monsters_spr_num();
	load_level_data_init_secret_bonus_tiles();
	load_level_data_init_password_items();
	g_res.restart = g_res.level;
	load_level_data_init_columns();
	int count = 0;
	for (int i = 0; i < MAX_LEVEL_ITEMS; ++i) {
		struct level_item_t *item = &g_res.level.items_tbl[i];
		const uint16_t spr_num = item->spr_num;
		if (spr_num >= 110 && spr_num <= 219) {
			if (spr_num >= 128 || spr_num <= 117) {
				++count;
			}
		}
	}
	g_vars.level_complete_bonuses_count = count;
	if (g_vars.level_num < 10 && next_level_tbl[g_vars.level_num] != 0xFF) {
		g_vars.level_complete_bonuses_count *= 2;
		g_vars.level_complete_secrets_count *= 2;
	}
}

static void level_draw_tile(int tile_num, int x, int y) {
	const int num = READ_LE_UINT16(g_res.leveldat + g_vars.tilemap.size + tile_num * 2);
	const uint8_t *tiledat;
	if (num < 0x100) {
		tiledat = g_res.leveldat + g_vars.tilemap.size + 512 + num * 128;
	} else {
		tiledat = g_res.uniondat + (num - 256) * 128;
	}
	video_draw_tile(tiledat, x * 16 - g_vars.tilemap.scroll_dx, y * 16 - g_vars.tilemap.scroll_dy);
}

static void level_update_tilemap() {
	const uint8_t mask = (g_vars.snow.value >= 20) ? 1 : 3;
	++g_vars.level_animated_tiles_counter;
	if ((g_vars.level_animated_tiles_counter & mask) == 0) {
		if (g_vars.level_animated_tiles_current_tbl == g_vars.tile_tbl1) {
			g_vars.level_animated_tiles_current_tbl = g_vars.tile_tbl2;
		} else if (g_vars.level_animated_tiles_current_tbl == g_vars.tile_tbl2) {
			g_vars.level_animated_tiles_current_tbl = g_vars.tile_tbl3;
		} else if (g_vars.level_animated_tiles_current_tbl == g_vars.tile_tbl3) {
			g_vars.level_animated_tiles_current_tbl = g_vars.tile_tbl1;
		}
	} else {
		if (!_redraw_tilemap) {
			return;
		}
	}
	if (GAME_SCREEN_W * GAME_SCREEN_H == 64000) {
		memcpy(g_res.vga, g_res.background, 320 * 200);
	} else {
		memset(g_res.vga, 0, GAME_SCREEN_W * GAME_SCREEN_H);
		for (int y = 0; y < MIN(200, GAME_SCREEN_H); ++y) {
			for (int x = 0; x < GAME_SCREEN_W; x += 320) {
				memcpy(g_res.vga + y * GAME_SCREEN_W + x, g_res.background + y * 320, MIN(320, GAME_SCREEN_W - x));
			}
		}
	}
	g_vars.tile_attr2_flags = 0;
	uint16_t offset = (g_vars.tilemap.y << 8) | g_vars.tilemap.x;
	for (int y = 0; y < (TILEMAP_SCREEN_H / 16) + 1; ++y) {
		for (int x = 0; x < (TILEMAP_SCREEN_W / 16) + 1; ++x) {
			const uint8_t tile_num = level_get_tile(offset + x);
			g_vars.tile_attr2_flags |= g_res.level.tile_attributes2[tile_num];
			if (_redraw_tilemap || g_vars.animated_tile_flag_tbl[tile_num] != 0) {
				const uint8_t num = g_vars.level_animated_tiles_current_tbl[tile_num];
				level_draw_tile(num, x, y);
			}
		}
		offset += 256;
	}
}

static void level_draw_tilemap() {
	if (_redraw_tilemap) {
		return;
	}
	if (g_vars.tilemap.redraw_flag1 == 0) {
		const bool changed = (g_vars.tilemap.x != g_vars.tilemap.prev_x) || (g_vars.tilemap.y != g_vars.tilemap.prev_y);
		if (!changed) {
			return;
		}
		g_vars.tilemap.prev_x = g_vars.tilemap.x;
		g_vars.tilemap.prev_y = g_vars.tilemap.y;
		if (g_vars.tilemap.redraw_flag2 == 0) {
			return;
		}
	}
	g_vars.tilemap.redraw_flag1 = 0;
	g_vars.tilemap.redraw_flag2 = 0;
	g_vars.tile_attr2_flags = 0;
	uint16_t offset = (g_vars.tilemap.y << 8) | g_vars.tilemap.x;
	for (int y = 0; y < (TILEMAP_SCREEN_H / 16) + 1; ++y) {
		for (int x = 0; x < TILEMAP_SCREEN_W / 16; ++x) {
			const uint8_t tile_num = level_get_tile(offset + x);
			g_vars.tile_attr2_flags |= g_res.level.tile_attributes2[tile_num];
			level_draw_tile(tile_num, x, y);
		}
		offset += 256;
	}
}

static bool level_adjust_hscroll_left() {
	if (g_vars.tilemap.x == 0) {
		return true;
	}
	--g_vars.tilemap.x;
	return false;
}

static uint16_t tilemap_end_xpos() {
	int end_x = (g_vars.objects_tbl[1].x_pos >> 4) - (TILEMAP_SCREEN_W / 16);
	if (g_res.level.tilemap_w < end_x) {
		end_x = 256 - (TILEMAP_SCREEN_W / 16);
	} else {
		end_x = g_res.level.tilemap_w;
	}
	return end_x;
}

static bool level_adjust_hscroll_right() {
	if (g_vars.tilemap.x >= tilemap_end_xpos()) {
		return true;
	}
	++g_vars.tilemap.x;
	return false;
}

static void level_adjust_x_scroll() {
	if (!g_options.dos_scrolling && g_vars.tilemap_noscroll_flag == 0) {
		const int x1 = TILEMAP_SCROLL_W * 2;
		const int x2 = TILEMAP_SCREEN_W - TILEMAP_SCROLL_W * 2;
		int tilemap_xpos = (g_vars.tilemap.x << 4) + g_vars.tilemap.scroll_dx;
		const int player_xpos = g_vars.objects_tbl[1].x_pos - tilemap_xpos;
		if (player_xpos > x2) {
			tilemap_xpos += (player_xpos - x2);
			const int tilemap_xend = tilemap_end_xpos() << 4;
			if (tilemap_xpos > tilemap_xend) {
				tilemap_xpos = tilemap_xend;
			}
		} else if (player_xpos < x1) {
			tilemap_xpos += (player_xpos - x1);
			if (tilemap_xpos < 0) {
				tilemap_xpos = 0;
			}
		}
		g_vars.tilemap.x = (tilemap_xpos >> 4);
		g_vars.tilemap.scroll_dx = (tilemap_xpos & 15);
		g_vars.tilemap.scroll_dx &= ~1;
		g_vars.level_xscroll_center_flag = 0;
		return;
	}

	const int x_pos = (g_vars.objects_tbl[1].x_pos >> 4) - g_vars.tilemap.x;
	if (x_pos >= (TILEMAP_SCREEN_W / 16) || g_vars.level_force_x_scroll_flag != 0 || g_vars.objects_tbl[1].x_velocity != 0) {
		if (g_vars.level_xscroll_center_flag != 0) {
			if (g_vars.level_xscroll_center_flag != 1) {
				if (x_pos < 15) {
					if (!level_adjust_hscroll_left()) {
						return;
					}
				}
			} else {
				if (x_pos > 5) {
					if (!level_adjust_hscroll_right()) {
						return;
					}
				}
			}
			g_vars.level_xscroll_center_flag = 0;
		} else {
			if (g_vars.objects_tbl[1].x_velocity != 0) {
				if (g_vars.objects_tbl[1].x_velocity < 0) {
					if (x_pos <= 4) {
						g_vars.level_xscroll_center_flag = 2;
					}
				} else {
					if (x_pos >= 16) {
						g_vars.level_xscroll_center_flag = 1;
					}
				}
			} else {
				if (x_pos < 10) {
					if (x_pos <= 4) {
						g_vars.level_xscroll_center_flag = 2;
					}
				} else {
					if (x_pos >= 16) {
						g_vars.level_xscroll_center_flag = 1;
					}
				}
			}
		}
	} else {
		g_vars.level_xscroll_center_flag = 0;
	}
}

static bool level_adjust_vscroll_down(int dl) {
	const int end_y = g_vars.tilemap.h - (TILEMAP_SCREEN_H / 16);
	if (g_vars.tilemap.y >= end_y) {
		return false;
	}
	++g_vars.tilemap.redraw_flag1;
	g_vars.tilemap.scroll_dy += dl;
	if (g_vars.tilemap.scroll_dy < 16) {
		return true;
	}
	g_vars.tilemap.scroll_dy -= 16;
	++g_vars.tilemap.y;
	if (g_vars.tilemap.y >= end_y) {
		g_vars.tilemap.scroll_dy = 0;
	}
	return true;
}

static bool level_adjust_vscroll_up(int dl) {
	if (g_vars.tilemap.y == 0) {
		return false;
	}
	++g_vars.tilemap.redraw_flag1;
	g_vars.tilemap.scroll_dy -= dl;
	if (g_vars.tilemap.scroll_dy >= 0) {
		return true;
	}
	g_vars.tilemap.scroll_dy += 16;
	--g_vars.tilemap.y;
	return true;
}

static void level_adjust_y_scroll() {
	if (!g_vars.tilemap_adjust_player_pos_flag && (g_res.level.scrolling_mask & 4) != 0) {
		level_adjust_vscroll_down(1);
		return;
	}
	if (g_vars.objects_tbl[1].data.p.y_velocity == 0) {
		g_vars.level_yscroll_center_flag = 0;
	}
	const int player_yorigin_diff = (g_vars.objects_tbl[1].y_pos >> 4) - g_vars.tilemap.y;
	if (g_vars.objects_tbl[1].data.p.y_velocity != 0) {
		if (player_yorigin_diff >= 9) {
			g_vars.tilemap_yscroll_diff = 3;
			++g_vars.level_yscroll_center_flag;
		} else if (player_yorigin_diff <= 2) {
			g_vars.tilemap_yscroll_diff = 8;
			++g_vars.level_yscroll_center_flag;
		}
	} else {
		if ((g_res.level.scrolling_mask & 1) == 0) {
			if (player_yorigin_diff >= 10) {
				g_vars.tilemap_yscroll_diff = 9;
				++g_vars.level_yscroll_center_flag;
			} else if (player_yorigin_diff <= 3) {
				g_vars.tilemap_yscroll_diff = 8;
				++g_vars.level_yscroll_center_flag;
			}
		} else {
			if (player_yorigin_diff >= 8) {
				g_vars.tilemap_yscroll_diff = 7;
				++g_vars.level_yscroll_center_flag;
			} else if (player_yorigin_diff <= 5) {
				g_vars.tilemap_yscroll_diff = 6;
				++g_vars.level_yscroll_center_flag;
			}
		}
	}
	if (g_vars.level_yscroll_center_flag == 0) {
		return;
	}
	const int bottom = (g_res.level.scrolling_top + (TILEMAP_SCREEN_H / 16)) << 4;
	if (g_vars.objects_tbl[1].y_pos > bottom || g_res.level.scrolling_top >= g_vars.tilemap.y - 1) {
		if (g_vars.level_yscroll_center_flag == 0) {
			return;
		}
		if (g_vars.tilemap_yscroll_diff == player_yorigin_diff) {
			g_vars.level_yscroll_center_flag = 0;
			return;
		}
		if (g_vars.tilemap_yscroll_diff < player_yorigin_diff) {
			if (g_vars.objects_tbl[1].y_pos <= bottom && g_vars.tilemap.y > g_res.level.scrolling_top) {
				g_vars.level_yscroll_center_flag = 0;
				return;
			}
			int dl = 16;
			if (!g_vars.tilemap_adjust_player_pos_flag) {
				const int dy = g_vars.objects_tbl[1].y_pos - g_vars.tilemap.scroll_dy;
				const int index = dy - ((g_vars.tilemap.y + g_vars.tilemap_yscroll_diff) << 4);
				if (index >= 0 && index < 132) {
					dl = vscroll_offsets_data[index];
				} else {
					print_warning("Invalid scroll down delta %d", index);
					return;
				}
			}
			if (!level_adjust_vscroll_down(dl)) {
				g_vars.level_yscroll_center_flag = 0;
			}
			return;
		}
	}
	int dl = 16;
	if (!g_vars.tilemap_adjust_player_pos_flag) {
		const int dy = g_vars.objects_tbl[1].y_pos - g_vars.tilemap.scroll_dy;
		const int index = ((g_vars.tilemap.y + g_vars.tilemap_yscroll_diff) << 4) - dy;
		if (index >= 0 && index < 132) {
			dl = vscroll_offsets_data[index];
		} else {
			print_warning("Invalid scroll up delta %d", index);
			return;
		}
	}
	if (!level_adjust_vscroll_up(dl)) {
		g_vars.level_yscroll_center_flag = 0;
	}
}

static void level_update_scrolling() {
	if (g_vars.tilemap_noscroll_flag != 0) {
		return;
	}
	if ((g_res.level.scrolling_mask & 2) == 0) {
		level_adjust_x_scroll();
	}
	level_adjust_y_scroll();
}

static void level_init_tilemap() {
	video_transition_close();
	g_vars.tilemap.x = 0;
	g_vars.tilemap.y = 0;
	g_vars.tilemap.scroll_dx = 0;
	g_vars.tilemap.scroll_dy = 0;
	g_vars.level_animated_tiles_current_tbl = g_vars.tile_tbl1;
	do {
		g_vars.tilemap_adjust_player_pos_flag = true;
		level_update_scrolling();
		g_vars.tilemap_adjust_player_pos_flag = false;
	} while ((g_vars.level_yscroll_center_flag | g_vars.level_xscroll_center_flag) != 0);
	print_debug(DBG_GAME, "init_tilemap x_pos %d y_pos %d scrolling 0x%x", g_vars.tilemap.x, g_vars.tilemap.y, g_res.level.scrolling_mask);
	if ((g_res.level.scrolling_mask & 2) == 0) {
		const int x = (g_vars.objects_tbl[1].x_pos >> 4) - g_vars.tilemap.x;
		if (x >= 12) {
			for (int i = 0; i < 10; ++i) {
				level_adjust_hscroll_right();
			}
		}
	}
	g_vars.tilemap.redraw_flag2 = 1;
	g_vars.tilemap.prev_x = _undefined;
	g_vars.tilemap.prev_y = _undefined;
	level_draw_tilemap();
	video_transition_open();
}

void level_player_die() {
	g_vars.restart_level_flag = 2;
	if (g_vars.player_lifes != 0) {
		if ((g_options.cheats & CHEATS_UNLIMITED_LIFES) == 0) {
			--g_vars.player_lifes;
			g_vars.player_energy = 0;
		}
	} else {
		g_vars.player_death_flag = 1;
	}
}

static void level_player_reset() {
	if (g_vars.player_nojump_counter > 0) {
		--g_vars.player_nojump_counter;
	}
	g_vars.player_anim2_counter = 0;
	g_vars.player_tile_flags = 2;
	g_vars.player_prev_y_pos = g_vars.objects_tbl[1].y_pos;
}

static void level_init_object_hit_from_player_pos() {
	struct object_t *obj = g_vars.current_hit_object;
	obj->x_pos = g_vars.objects_tbl[1].x_pos;
	obj->y_pos = g_vars.objects_tbl[1].y_pos;
	obj->spr_num = 53;
	--obj;
	if (obj < &g_vars.objects_tbl[6]) {
		obj = &g_vars.objects_tbl[10];
	}
	g_vars.current_hit_object = obj;
}

static void level_init_object_hit_from_xy_pos(int16_t x_pos, int16_t y_pos) {
	struct object_t *obj = g_vars.current_hit_object;
	obj->x_pos = x_pos;
	obj->y_pos = y_pos;
	obj->spr_num = 53;
	--obj;
	if (obj < &g_vars.objects_tbl[6]) {
		obj = &g_vars.objects_tbl[10];
	}
	g_vars.current_hit_object = obj;
}

static void level_update_tile_type_2() {
	if (g_vars.level_num == 14) {
		g_vars.player_death_flag = 0xFF;
		return;
	}
	if (g_options.cheats & CHEATS_NO_HIT) {
		return;
	}
	level_player_die();
	g_vars.restart_level_flag = 2;
}

static int level_get_tile_player_offset(uint8_t attr) {
	if ((attr & 0x30) == 0) {
		return attr;
	}
	int x = (g_vars.objects_tbl[1].x_pos & 15) / 3;
	if (attr & 0x10) {
		x = (attr & 15) + x;
	} else {
		x = (attr & 15) - x;
	}
	return x;
}

static void level_update_tile_attr1_helper(uint16_t offset) {
	g_vars.objects_tbl[1].x_friction = 0;
	if (g_vars.objects_tbl[1].data.p.y_velocity < 0) {
		g_vars.player_tile_flags |= 1;
		return;
	}
	g_vars.player_gravity_flag = 0;
	g_vars.objects_tbl[1].y_pos &= ~15;
	uint8_t al = level_get_tile(offset);
	al = g_res.level.tile_attributes3[al];
	if (al != 0) {
		int dy = level_get_tile_player_offset(al);
		int bx = g_vars.objects_tbl[1].data.p.y_velocity >> 4;
		if (bx > 0 && dy >= bx) {
			dy = bx;
		}
		g_vars.objects_tbl[1].y_pos += dy;
	} else {
		al = level_get_tile(offset - 0x100);
		al = g_res.level.tile_attributes3[al];
		if (al != 0) {
			int ax = level_get_tile_player_offset(al);
			if (ax < 16) {
				ax -= 16;
				g_vars.objects_tbl[1].y_pos += ax;
			}
		}
	}
	if (g_vars.player_jumping_counter > 4) {
		level_init_object_hit_from_player_pos();
		const int dy = g_vars.objects_tbl[1].y_pos - g_vars.player_prev_y_pos;
		if (dy >= 32 && g_vars.objects_tbl[1].data.p.y_velocity >= 80) {
			g_vars.player_prev_y_pos = g_vars.objects_tbl[1].y_pos;
			if (g_vars.player_jumping_counter >= 20 && g_vars.objects_tbl[1].data.p.y_velocity > 160) {
				g_vars.shake_screen_counter = 8;
			}
			if (g_vars.player_jumping_counter > 10) {
				if ((g_res.level.scrolling_mask & 1) == 0) {
					g_vars.objects_tbl[1].data.p.y_velocity = -32;
				}
				g_vars.objects_tbl[1].spr_num = (g_vars.objects_tbl[1].spr_num & 0xE000) | 12;
				g_vars.player_jumping_counter = 0;
				return;
			}
		}
	}
	g_vars.objects_tbl[1].data.p.y_velocity = 0;
	level_player_reset();
}

static void level_update_tile_attr1_type_1(uint16_t offset) {
	level_update_tile_attr1_helper(offset);
}

static void level_update_tile_attr1_type_2(uint16_t offset) {
	level_update_tile_attr1_helper(offset);
	g_vars.objects_tbl[1].x_friction = 1;
}

static void level_update_tile_attr1_type_3(uint16_t offset) {
	level_update_tile_attr1_helper(offset);
	g_vars.objects_tbl[1].x_friction = 2;
}

static void level_update_tile_attr1_type_4(uint16_t offset) {
	level_update_tile_attr1_helper(offset);
	g_vars.objects_tbl[1].x_friction = 3;
}

static void level_update_tile_attr1_type_5(uint16_t offset) {
	g_vars.objects_tbl[1].x_friction = 0;
	if (g_vars.player_action_counter == 0) {
		level_update_tile_attr1_helper(offset);
	} else {
		g_vars.player_tile_flags |= 1;
	}
}

static void level_update_tile_attr1_type_0(uint16_t offset) {
	if (g_vars.objects_tbl[1].data.p.y_velocity == 0) {
		uint8_t al = level_get_tile(offset + 0x100);
		al = g_res.level.tile_attributes3[al];
		if (al != 0 && level_get_tile_player_offset(al) < 16) {
			offset += 0x100;
			g_vars.objects_tbl[1].y_pos += 16;
			level_update_tile_attr1_type_1(offset);
			return;
		}
	}
	g_vars.player_tile_flags |= 1;
}

static void level_update_tile_attr2_type1() {
	if (g_vars.objects_tbl[1].data.p.y_velocity != 0) {
		g_vars.objects_tbl[1].data.p.y_velocity = 0;
		g_vars.objects_tbl[1].y_pos &= ~15;
		g_vars.objects_tbl[1].y_pos += 16;
		return;
	}
	const uint8_t spr_num = g_vars.objects_tbl[1].spr_num;
	if (spr_num == 10 || spr_num == 21) {
		return;
	}
	static int16_t data[6];
	const uint8_t bh = (data[1] >> 4) & 255;
	const uint8_t bl = (data[0] >> 4) & 255;
	uint16_t offset = (bh << 8) | bl;
	uint8_t tile_num = level_get_tile(offset);
	if (g_res.level.tile_attributes0[tile_num] & 1) {
		int dx = 16;
		tile_num = level_get_tile(offset - 1);
		if ((g_res.level.tile_attributes0[tile_num] & 1) == 0) {
			dx = -dx;
		}
		data[0] += dx;
	}
}

static void level_update_player_jump();

static void level_update_tile0(uint16_t offset) {
	if (g_vars.objects_tbl[1].y_pos <= -1) {
		level_update_player_jump();
		g_vars.player_tile_flags = 0xFF;
		return;
	}
	const uint16_t offset_nextline = offset + 0x100;
	int tile_num = level_get_tile(offset_nextline);
	const uint8_t attr1 = g_res.level.tile_attributes1[tile_num];
	if (g_vars.decor_tile0_offset != offset_nextline) {
		/* update tiles below the player */
		if (g_vars.decor_tile0_offset != _undefined) {
			tile_num = level_get_tile(g_vars.decor_tile0_offset);
			while (1) {
				--tile_num;
				if ((g_res.level.tile_attributes2[tile_num] & 0x20) == 0) {
					g_vars.decor_tile0_offset = _undefined;
					break;
				}
				level_set_tile(g_vars.decor_tile0_offset, tile_num);
			}
		}
		tile_num = level_get_tile(offset_nextline);
		if (g_res.level.tile_attributes2[tile_num] & 0x20) {
			g_vars.decor_tile0_offset = offset_nextline;
			++tile_num;
			level_set_tile(g_vars.decor_tile0_offset, tile_num);
		}
	}
	switch (attr1) {
	case 0:
		level_update_tile_attr1_type_0(offset_nextline);
		break;
	case 1:
		level_update_tile_attr1_type_1(offset_nextline);
		break;
	case 2:
		level_update_tile_attr1_type_2(offset_nextline);
		break;
	case 3:
		level_update_tile_attr1_type_3(offset_nextline);
		break;
	case 4:
		level_update_tile_attr1_type_4(offset_nextline);
		break;
	case 5:
		level_update_tile_attr1_type_5(offset_nextline);
		break;
	case 6:
		/* spikes */
		level_update_tile_type_2();
		break;
	default:
		print_warning("Unhandled level_update_tile0 attr1 %d", attr1);
		break;
	}
	if (offset < 0x100 || g_vars.objects_tbl[1].data.p.y_velocity >= 0) {
		return;
	}
	offset -= 0x100;
	uint8_t ah = level_get_tile(offset);
	uint8_t al = level_get_tile(offset + 0x100);
	al = g_res.level.tile_attributes0[al];
	SWAP(ah, al);
	al = g_res.level.tile_attributes2[al] & 15;
	switch (al) {
	case 0:
		break;
	case 1:
		level_update_tile_attr2_type1();
		break;
	case 2:
		level_update_tile_type_2();
		break;
	default:
		print_warning("Unhandled level_update_tile0 attr2 %d", al);
		break;
	}
	if ((ah & 1) == 0 || g_vars.objects_tbl[1].y_pos <= 0) {
		return;
	}
	int dx = (g_vars.objects_tbl[1].x_velocity > 0) ? -1 : 1;
	offset += dx;
	al = level_get_tile(offset + 0x100);
	al = g_res.level.tile_attributes0[al];
	if (al != 0) {
		dx = -dx;
		offset += 2 * dx;
		al = level_get_tile(offset + 0x100);
		al = g_res.level.tile_attributes0[al];
		if (al != 0) {
			return;
		}
	}
	g_vars.objects_tbl[1].x_pos += 2 * dx;
}

static void level_update_tile_type_0(uint16_t offset) {
	const int tile_num = level_get_tile(offset);
	if ((g_res.level.tile_attributes2[tile_num] & 0x10) == 0) {
		return;
	}
	for (int i = 0; i < 20; ++i) {
		struct fly_t *p = &g_vars.fly_tbl[i];
		if (p->x_pos != _undefined) {
			continue;
		}
		p->x_pos = g_vars.objects_tbl[1].x_pos << 3;
		p->y_pos = g_vars.objects_tbl[1].y_pos << 3;
		p->x_delta = 0;
		p->y_delta = 0;
		p->unk7 = 0;
		break;
	}
}

static void level_update_tile_type_1(uint16_t offset) {
	g_vars.objects_tbl[1].x_pos -= g_vars.objects_tbl[1].x_velocity >> 4;
	g_vars.objects_tbl[1].x_velocity = 0;
}

static void level_update_tile1(uint16_t offset) {
	const int tile_num = level_get_tile(offset);
	const uint8_t type = g_res.level.tile_attributes0[tile_num];
	switch (type) {
	case 0:
		level_update_tile_type_0(offset);
		break;
	case 1:
		level_update_tile_type_1(offset);
		break;
	case 2:
		level_update_tile_type_2();
		break;
	default:
		print_warning("Unhandled level_update_tile1 type %d", type);
		break;
	}
}

static void level_update_tile2(uint16_t offset) {
	const int tile_num = level_get_tile(offset);
	const uint8_t type = g_res.level.tile_attributes0[tile_num];
	if (type == 2 || type == 4) {
		switch (type) {
		case 2:
			level_update_tile_type_2();
			break;
		default:
			print_warning("Unhandled level_update_tile2 type %d", type);
			break;
		}
	}
}

static void level_reset_objects() {
	memset(&g_vars.objects_tbl[1], 0, sizeof(struct object_t) * (OBJECTS_COUNT - 1));
	for (int i = 0; i < OBJECTS_COUNT; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[i];
		obj->spr_num = 0xFFFF;
		obj->hit_counter = 0;
	}
	g_vars.objects_tbl[1].x_pos = (g_options.start_xpos16 < 0) ? g_res.level.start_x_pos : (g_options.start_xpos16 << 4);
	g_vars.objects_tbl[1].y_pos = (g_options.start_ypos16 < 0) ? g_res.level.start_y_pos : (g_options.start_ypos16 << 4);
}

static void level_reset() {
	g_vars.tilemap_noscroll_flag = 0;
	g_vars.player_nojump_counter = 0;
	g_vars.player_action_counter = 0;
	g_vars.restart_level_flag = 0;
	g_vars.player_death_flag = 0;
	g_vars.level_completed_flag = 0;
	g_vars.light.palette_flag1 = 0;
	g_vars.light.palette_flag2 = 0;

	g_vars.objects_tbl[1].data.p.special_anim_num = 0;
	g_vars.tilemap.prev_x = _undefined;
	g_vars.current_hit_object = &g_vars.objects_tbl[6];
	level_reset_objects();
	set_level_palette();
	for (int i = 0; i < 256; ++i) {
		/* const uint8_t dh = */ random_get_number();
		const uint8_t al = random_get_number();
		/* this matches disassembly but is likely a typo and should rather be '(dh << 8) | al' */
		g_vars.snow.random_tbl[i] = (al << 8) | al;
	}
	for (int i = 0; i < 20; ++i) {
		g_vars.fly_tbl[i].x_pos = _undefined;
	}
	level_reset_columns_tiles();
	for (int i = 0; i < 20; ++i) {
		g_vars.orb_tbl[i].x_pos = 0xFFFF;
	}
	memset(g_vars.boss_level5.leaf_tbl, 0xFF, sizeof(g_vars.boss_level5.leaf_tbl));
	g_vars.player_energy = 3;
	memset(&g_vars.boss_level9, 0, sizeof(g_vars.boss_level9));

	g_vars.decor_tile0_offset = _undefined;
	g_vars.tilemap_yscroll_diff = 0;
}

static uint16_t level_get_player_tile_pos() {
	const int y = (g_vars.objects_tbl[1].y_pos) >> 4;
	const int x = (g_vars.objects_tbl[1].x_pos) >> 4;
	return (y << 8) | x;
}

static struct object_t *level_add_object75_score(struct object_t *ref_obj, int num) {
	const int score_index = num - 74;
	if (score_index >= 0 && score_index <= 16) {
		g_vars.score += score_tbl[score_index];
	}
	for (int i = 0; i < 16; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[75 + i];
		if (obj->spr_num == 0xFFFF) {
			obj->spr_num = num;
			obj->x_pos = ref_obj->x_pos;
			obj->y_pos = ref_obj->y_pos;
			obj->data.t.counter = 44;
			if (ref_obj >= &g_vars.objects_tbl[23]) {
				struct level_item_t *item = ref_obj->data.t.ref;
				if (item) {
					assert(item >= &g_res.level.items_tbl[0] && item < &g_res.level.items_tbl[MAX_LEVEL_ITEMS]);
					item->spr_num = 0xFFFF;
				}
			}
			return obj;
		}
	}
	return 0;
}

static void level_update_object_anim(const uint8_t *anim) {
	int16_t num = READ_LE_UINT16(anim);
	if (num < 0) {
		anim += num;
		num = READ_LE_UINT16(anim);
	}
	g_vars.player_current_anim_type = num >> 8;
	g_vars.objects_tbl[1].spr_num = (num & 0x1FFF) | ((g_vars.objects_tbl[1].data.p.hdir < 0) ? 0x8000 : 0);
	g_vars.objects_tbl[1].data.p.anim = anim + 2;
}

static void level_update_objects_hit_animation() {
	struct object_t *obj = g_vars.current_hit_object;
	for (int i = 0; i < 5; ++i) {
		const int num = (obj->spr_num & 0x1FFF) + 1;
		--obj->y_pos;
		if (num < 58) {
			obj->spr_num = num;
		} else {
			obj->spr_num = 0xFFFF;
		}
		--obj;
		if (obj < &g_vars.objects_tbl[6]) {
			obj = &g_vars.objects_tbl[10];
		}
	}
}

void level_add_object23_bonus(int x_vel, int y_vel, int count) {
	int counter = 0;
	for (int i = 0; i < 32; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[23 + i];
		if (obj->spr_num == 0xFFFF) {
			obj->spr_num = g_vars.current_bonus.spr_num;
			obj->hit_counter = 0;
			obj->data.t.counter = 198;
			obj->x_pos = g_vars.current_bonus.x_pos;
			obj->y_pos = g_vars.current_bonus.y_pos;
			obj->x_velocity = x_vel;
			obj->data.t.y_velocity = y_vel;
			obj->data.t.ref = 0;
			x_vel = -x_vel;
			++counter;
			if ((counter & 1) == 0) {
				if (counter > 12) {
					x_vel = 0;
				}
				x_vel -= 16;
				y_vel -= 16;
			}
			--count;
			if (count == 0) {
				return;
			}
		}
	}
}

bool level_objects_collide(const struct object_t *si, const struct object_t *di) {
	const int dx = abs(si->x_pos - di->x_pos);
	if (dx >= 64) {
		return false;
        }
	const int dy = abs(si->y_pos - di->y_pos);
	if (dy >= 70) {
		return false;
	}
	int num = si->spr_num;
	int a = si->y_pos;
	int d = di->y_pos;
	if (a < d) {
		num = di->spr_num;
		SWAP(a, d);
		SWAP(si, di);
	}
	num &= 0x1FFF;
	int b = spr_size_tbl[num * 2 + 1];
	a -= b;
	if (a >= d) {
		return false;
	}
	if (g_vars.player_using_club_flag == 0) {
		d -= a;
		if (g_vars.objects_tbl[1].data.p.y_velocity >= 128 || (d <= (b >> 1) && si != &g_vars.objects_tbl[1])) {
			++g_vars.player_jump_monster_flag;
			g_vars.monster.collide_y_dist = d;
		}
	}
	d = si->x_pos - spr_offs_tbl[(si->spr_num & 0x1FFF) * 2];
	a = di->x_pos - spr_offs_tbl[(di->spr_num & 0x1FFF) * 2];
	b = spr_size_tbl[(di->spr_num & 0x1FFF) * 2];
        if (a >= d) {
                SWAP(a, d);
                b = spr_size_tbl[(si->spr_num & 0x1FFF) * 2];
        }
	if (g_vars.player_using_club_flag == 0) {
		b >>= 1;
	}
	return (a + b > d);
}

static void level_monster_update_anim(struct object_t *obj) {
	const uint8_t *p = obj->data.m.anim;
	do {
		p += 2;
	} while (READ_LE_UINT16(p) != 0x7D00);
	obj->data.m.anim = p + 2;
}

static void level_monster_die(struct object_t *obj, struct level_monster_t *m, struct object_t *obj_player) {
	const int num = m->score + 74;
	static const uint8_t data[] = { 1, 2, 3, 4, 6, 8 };
	int count = data[(obj->data.m.hit_jump_counter >> 3) & 7];
	const int x_pos = obj->x_pos;
	const int y_pos = obj->y_pos;
	do {
		struct object_t *score_obj = level_add_object75_score(obj, num);
		if (score_obj) {
			score_obj->data.t.counter = count << 2;
		}
		obj->y_pos += 7;
		obj->x_pos += 9;
	} while (--count != 0);
	obj->x_pos = x_pos;
	obj->y_pos = y_pos;
	obj->data.m.state = 0xFF;
	if ((m->flags & 1) == 0) {
		g_vars.current_bonus.x_pos = obj->x_pos;
		g_vars.current_bonus.y_pos = obj->y_pos;
		g_vars.current_bonus.spr_num = 0x2046; /* bones */
		level_add_object23_bonus(48, -128, 6);
	} else {
		level_monster_update_anim(obj);
		if ((m->flags & ~0x37) != 0x88) {
			m->flags &= ~8;
		}
		int dy = MIN(g_vars.player_club_power, 25);
		dy = (-dy) << 3;
		obj->data.m.y_velocity = dy;
		int dx = dy >> 1;
		if ((obj_player->spr_num & 0x8000) == 0) {
			dx = -dx;
		}
		obj->data.m.x_velocity = dx;
	}
}

static bool level_collide_axe_monsters(struct object_t *axe_obj) {
	for (int i = 0; i < MONSTERS_COUNT; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[11 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		if (obj->data.m.state == 0xFF) {
			continue;
		}
		struct level_monster_t *m = obj->data.m.ref;
		if (m->flags & 0x10) {
			continue;
		}
		if (!level_objects_collide(axe_obj, obj)) {
			continue;
		}
		obj->data.m.flags |= 0x40;
		obj->data.m.energy -= g_vars.player_club_power;
		if (obj->data.m.energy < 0) {
			play_sound(2);
			level_monster_die(obj, m, axe_obj);
		} else {
			obj->x_pos -= obj->data.m.x_velocity >> 2;
		}
		axe_obj->spr_num = 0xFFFF;
		return true;
	}
	return false;
}

static uint16_t level_get_random_bonus_spr_num() {
	uint8_t num;
	while (1) {
		num = random_get_number() & 0x7F;
		if (num < 0x5F) {
			break;
		}
	}
	return 0x2080 + num;
}

void level_add_bonuses_4x() {
	int x_vel = 32;
	int y_vel = -160;
	for (int i = 0; i < 4; ++i) {
		g_vars.current_bonus.spr_num = level_get_random_bonus_spr_num();
		level_add_object23_bonus(x_vel, y_vel, 4);
		x_vel = -x_vel;
		if (x_vel >= 0) {
			x_vel -= 16;
			y_vel -= 16;
		}
	}
}

static bool level_update_found_bonus_tile(struct level_bonus_t *bonus) {
	const uint16_t pos = bonus->pos;
	bonus->pos = 0xFFFF;
	level_set_tile(pos, bonus->tile_num1);
	int8_t al = (pos & 0xFF) - g_vars.tilemap.x;
	if (al < 0 || al >= (TILEMAP_SCREEN_W / 16)) {
		return false;
	}
	int8_t ah = (pos >> 8) - g_vars.tilemap.y;
	if (ah < 0 || ah >= (TILEMAP_SCREEN_H / 16) + 1) {
		return false;
	}
	return false;
}

static bool level_handle_bonuses_found(struct object_t *obj, struct level_bonus_t *bonus) {
	const uint16_t pos = bonus->pos;
	const int y_pos = ((pos >> 8)   << 4) + 8;
	const int x_pos = ((pos & 0xFF) << 4) + 12;
	level_init_object_hit_from_xy_pos(x_pos, y_pos);
	if (bonus->count & 0x80) {
		--bonus->count;
		if (bonus->count & 0x80) {
			return false;
		}
		g_vars.current_bonus.x_pos = (pos & 0xFF) << 4;
		g_vars.current_bonus.y_pos = (pos >> 8)   << 4;
		int num = 0;
		while (1) {
			num = random_get_number() & 7;
			if (num != 0) {
				break;
			}
		}
		g_vars.current_bonus.spr_num = 110 + num;
		g_vars.current_bonus.y_pos -= 112;
		level_add_object23_bonus(0, 0, 1);
		++g_vars.level_current_secrets_count;
	} else {
		static uint8_t draw_counter = 0;
		const int diff = abs(g_vars.level_draw_counter - draw_counter);
		draw_counter = g_vars.level_draw_counter;
		if (diff < 6) {
			return true;
		}
		g_vars.current_bonus.x_pos = (pos & 0xFF) << 4;
		g_vars.current_bonus.y_pos = (pos >> 8)   << 4;
		int x_vel, count = 1;
		if (bonus->count & 0x40) {
			if (bonus->count == 64) {
				bonus->count = 0;
				if (g_vars.level_num == 3) {
					g_vars.current_bonus.spr_num = 306;
				} else if (g_vars.level_num == 7 || g_vars.level_num == 6) {
					g_vars.current_bonus.spr_num = 300;
				} else {
					g_vars.current_bonus.spr_num = 308;
				}
				level_add_object23_bonus(32, -48, 2);
				if (g_vars.level_num == 8) {
					g_vars.current_bonus.spr_num = 310;
					count = 2;
				} else {
					g_vars.current_bonus.spr_num = 229;
					count = 4;
				}
			} else {
				if (g_vars.level_num == 3) {
					g_vars.current_bonus.spr_num = 306;
				} else if (g_vars.level_num == 7 || g_vars.level_num == 6) {
					g_vars.current_bonus.spr_num = 300;
				} else {
					g_vars.current_bonus.spr_num = 308;
				}
				level_add_object23_bonus(48, -96, 4);
				goto decrement_bonus;
			}
		} else {
			g_vars.current_bonus.spr_num = level_get_random_bonus_spr_num();
		}
		x_vel = 48;
		if (obj < &g_vars.objects_tbl[1]) {
			if (g_vars.objects_tbl[1].data.p.hdir >= 0) {
				x_vel = -x_vel;
			}
		} else if (obj->x_velocity >= 0) {
			x_vel = -x_vel;
		}
		level_add_object23_bonus(x_vel, -112, count);
decrement_bonus:
		--bonus->count;
		if ((bonus->count & 0x80) == 0) {
			return true;
		}
	}
	return level_update_found_bonus_tile(bonus);
}

static bool level_collide_axe_bonuses(struct object_t *obj) {
	const int obj_x = obj->x_pos >> 4;
	const int obj_y = obj->y_pos - 16;
	for (int i = 0; i < MAX_LEVEL_BONUSES; ++i) {
		struct level_bonus_t *bonus = &g_res.level.bonuses_tbl[i];
		if (bonus->pos == 0xFFFF) {
			continue;
		}
		const int bonus_x = bonus->pos & 0xFF;
		if (abs(bonus_x - obj_x) > 1) {
			continue;
		}
		const int bonus_y = bonus->pos >> 8;
		if (abs((bonus_y << 4) - obj_y) >= 16) {
			continue;
		}
		obj->spr_num = 0xFFFF;
		if (level_handle_bonuses_found(obj, bonus)) {
			continue;
		}
		memset(g_vars.level_bonuses_count_tbl, 0, 80);
		int count = 0;
		const uint16_t pos = bonus->pos;
		for (int j = 0; j < MAX_LEVEL_BONUSES; ++j) {
			struct level_bonus_t *bonus2 = &g_res.level.bonuses_tbl[j];
			if (bonus2->pos == 0xFFFF || bonus2->pos == pos) {
				continue;
			}
			if (abs((bonus2->pos & 0xFF) - bonus_x) > 1) {
				continue;
			}
			if (abs((bonus2->pos >> 8) - bonus_y) > 1) {
				continue;
			}
			if (g_vars.level_bonuses_count_tbl[j] == 0) {
				++g_vars.level_bonuses_count_tbl[j];
				++g_vars.level_current_secrets_count;
				++count;
			}
		}
		if (count == 0) {
			return true;
		}
	}
	return false;
}

static void level_update_objects_axe() {
	g_vars.player_using_club_flag = true;
	for (int i = 0; i < 4; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[2 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		if (level_collide_axe_monsters(obj) || level_collide_axe_bonuses(obj)) {
			continue;
		}
	}
	if (g_vars.player_flying_flag == 0) {
		struct object_t *obj = &g_vars.objects_tbl[0];
		if (obj->spr_num != 0xFFFF) {
			if (level_collide_axe_monsters(obj) || level_collide_axe_bonuses(obj)) {
				if (g_vars.objects_tbl[1].data.p.y_velocity != 0) {
					g_vars.objects_tbl[1].data.p.y_velocity = -80;
				}
			}
		}
		g_vars.player_using_club_flag = false;
	}
}


static int level_get_tile_monster_offset(uint8_t tile_num, struct object_t *obj) {
	const uint8_t attr = g_res.level.tile_attributes1[tile_num];
	if ((attr & 0x30) == 0) {
		return attr;
	}
	int x = (obj->x_pos & 15) / 3;
	if (attr & 0x10) {
		x = (attr & 15) + x;
	} else {
		x = (attr & 15) - x;
	}
	return x;
}

static void level_update_monster_pos(struct object_t *obj, struct level_monster_t *m) {
	const uint16_t pos = ((obj->y_pos >> 4) << 8) | (obj->x_pos >> 4);
	uint8_t dl = level_get_tile(pos);
	uint8_t dh = level_get_tile(pos - 0x100);
	int x_vel = obj->data.m.x_velocity;
	if (x_vel != 0 && x_vel <= 1) {
		x_vel = -x_vel;
	}
	uint8_t al = level_get_tile(pos + x_vel - 0x100);
	al = g_res.level.tile_attributes0[al];
	if ((m->flags & 0x40) != 0 && (m->flags & 0x80) != 0) {
		if (al != 0) {
			obj->x_pos -= obj->data.m.x_velocity >> 4;
		} else {
			monster_change_prev_anim(obj);
			obj->data.m.y_velocity = 0;
			m->flags &= ~0x80;
			obj->y_pos -= 16;
		}
	} else {
		if (al != 0) {
			if (m->flags & 0x40) {
				monster_change_next_anim(obj);
				m->flags |= 0x80;
				obj->data.m.y_velocity &= ~15;
				obj->x_pos += obj->data.m.x_velocity >> 2;
				return;
			}
			obj->data.m.x_velocity = -obj->data.m.x_velocity;
			obj->x_pos += obj->data.m.x_velocity >> 4;
		}
		obj->y_pos -= 16;
		SWAP(dl, dh);
		al = g_res.level.tile_attributes1[dl];
		if (al == 0) {
			obj->y_pos += 16;
			dl = dh;
			al = g_res.level.tile_attributes1[dl];
			if (al == 0) {
				if (obj->data.m.y_velocity < 256) {
					obj->data.m.y_velocity += 16;
				}
				return;
			}
		}
		const int dy = level_get_tile_monster_offset(dl, obj);
		obj->y_pos &= ~15;
		obj->y_pos += dy;
		if ((m->flags & 0x20) == 0) {
			int y_vel = (-obj->data.m.y_velocity) >> 1;
			if (abs(y_vel) <= 32) {
				y_vel = 0;
			}
			obj->data.m.y_velocity = y_vel;
		} else {
			obj->data.m.y_velocity = 0;
		}
	}
}

extern void boss_update();

extern void monster_func1(int type, struct object_t *obj); /* update */
extern bool monster_func2(int type, struct level_monster_t *m); /* init */

static void level_update_objects_monsters() {
	boss_update();
	for (int i = 0; i < MONSTERS_COUNT; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[11 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		print_debug(DBG_GAME, "monster #%d pos %d,%d spr %d", i, obj->x_pos, obj->y_pos, obj->spr_num);
		obj->y_pos += obj->data.m.y_velocity >> 4;
		if (obj->data.m.x_velocity != -1) {
			obj->x_pos += obj->data.m.x_velocity >> 4;
		}
		struct level_monster_t *m = obj->data.m.ref;
		if (m->flags & 8) {
			level_update_monster_pos(obj, m);
		}
		const uint8_t *p = obj->data.m.anim;
		int16_t num;
		while (1) {
			num = READ_LE_UINT16(p);
			if (num >= 0) {
				break;
			}
			p += num;
		}
		uint16_t dx = g_res.spr_monsters_offset + (num & 0x1FFF);
		g_vars.monster.hit_mask = ((num >> 8) & 0xE0) | (g_vars.player_hit_monster_counter & 0xFF);
		if (g_vars.player_hit_monster_counter == 0) {
			obj->data.m.anim = p + 2;
		} else {
			if (g_vars.player_hit_monster_counter == 7) {
				g_vars.shake_screen_counter = 9;
			}
			const uint16_t *q = monster_spr_tbl;
			const uint16_t *end = &monster_spr_tbl[48];
			dx &= 0x1FFF;
			while (q < end) {
				if (dx < q[0]) {
					obj->data.m.anim = p + 2;
					break;
				}
				if (dx <= q[1]) {
					dx = q[2] + 53;
					break;
				}
				q += 3;
			}
			if (q >= end) {
				print_warning("level_update_objects_monsters spr %d hit %d not found", dx, g_vars.player_hit_monster_counter);
				continue;
			}
		}
		dx &= 0x1FFF;
		if (obj->data.m.x_velocity < 0) {
			dx |= 0x8000;
		}
		obj->spr_num &= 0x6000;
		obj->spr_num |= dx;
		const int type = (m->type & 0x7F);
		monster_func1(type, obj);
	}
	for (int i = 0; i < g_res.level.monsters_count; ++i) {
		struct level_monster_t *m = &g_res.level.monsters_tbl[i];
		if (m->spr_num != 0xFFFF && (m->flags & 4) == 0 && (_expert || (m->type & 0x80) == 0)) {
			const int type = m->type & 0x7F;
			if (!monster_func2(type, m)) {
				continue;
			}
			/* monster type */
			const uint8_t *p = monster_anim_tbl;
			const uint8_t *end = &monster_anim_tbl[1100];
			do {
				p += 2;
			} while (p < end && (READ_LE_UINT16(p) != 0x7D01 || READ_LE_UINT16(p + 2) != type));
			p += 4;
			/* monster sprite */
			const int spr_num = m->spr_num - g_res.spr_monsters_offset;
			while (p < end && READ_LE_UINT16(p) != spr_num) {
				p += 2;
			}
			if (p >= end) {
				print_warning("level_update_objects_monsters type %d spr %d not found", type, spr_num);
				continue;
			}
			g_vars.monster.current_object->data.m.anim = p;
		}
	}
}

static void level_update_objects_club_projectiles() {
	for (int i = 0; i < 4; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[2 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		if ((obj->spr_num & 0x2000) == 0) {
			obj->spr_num = 0xFFFF;
			continue;
		}
		obj->x_pos += obj->x_velocity >> 4;
		obj->y_pos += obj->data.c.y_velocity >> 4;
		const uint8_t *anim = obj->data.c.anim;
		int16_t num = READ_LE_UINT16(anim);
		if (num < 0) {
			anim += num;
			num = READ_LE_UINT16(anim);
		}
		obj->data.c.anim = anim + 2;
		obj->spr_num = num | ((obj->x_velocity < 0) ? 0x8000 : 0);
		switch (obj->x_friction) {
		case 0:
			obj->data.c.y_velocity += 32;
			break;
		case 1:
			obj->data.c.y_velocity -= 16;
			break;
		default:
			print_warning("Unhandled level_update_objects_club_projectiles type %d", obj->x_friction);
			break;
		}
	}
}

static void level_update_objects_bonuses() {
	for (int i = 0; i < 32; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[23 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		--obj->data.t.counter;
		if (obj->data.t.counter == 0) {
			obj->hit_counter = 15;
			continue;
		}
		if (obj->data.t.counter < 0) {
			if (obj->hit_counter == 0) {
				obj->spr_num = 0xFFFF;
			}
			continue;
		}
		obj->x_pos += obj->x_velocity >> 4;
		if (obj->x_pos < 0) {
			obj->x_pos = 0;
			obj->x_velocity = -obj->x_velocity;
		}
		obj->y_pos += (obj->data.t.y_velocity >> 4);
		const int dy = obj->data.t.y_velocity + 9;
		if (dy < 256) {
			obj->data.t.y_velocity = dy;
		}
		const int num = obj->spr_num & 0x1FFF;
		if (num == 229) {
			continue;
		}
		if (num == 300 || num == 306 || num == 308 || num == 310) {
			if (obj->data.t.counter > 50) {
				obj->data.t.counter = 50;
			}
			continue;
		}
		const uint16_t pos = ((obj->y_pos >> 4) << 8) | (obj->x_pos >> 4);
		if (dy > 0) {
			const int tile_num = level_get_tile(pos);
			uint8_t al = g_res.level.tile_attributes1[tile_num];
			if (al != 0) {
				/* bounce on the ground */
				obj->data.t.y_velocity = -obj->data.t.y_velocity;
				obj->data.t.y_velocity >>= 1;
				const bool x_direction = (obj->x_velocity < 0);
				const int dx = (obj->x_velocity < 0) ? -8 : 8;
				obj->x_velocity -= dx;
				if (x_direction != (obj->x_velocity < 0)) {
					obj->x_velocity = 0;
				}
			}
		} else {
			const int tile_num = level_get_tile(pos - 0x100);
			uint8_t al = g_res.level.tile_attributes0[tile_num];
			if (al != 0) {
				obj->x_velocity = -obj->x_velocity;
				obj->x_pos += obj->x_velocity >> 4;
			}
		}
		if (g_vars.level_draw_counter & 1) {
			continue;
		}
		if (obj->x_velocity == 0) {
			continue;
		}
		if (num < 73) {
			obj->spr_num = num + 1;
		} else if (num == 73) {
			obj->spr_num = 70;
		}
	}
}

static void level_update_objects_bonus_scores() {
	for (int i = 0; i < 16; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[75 + i];
		if (obj->spr_num != 0xFFFF) {
			--obj->y_pos;
			--obj->data.t.counter;
			if (obj->data.t.counter == 0) {
				obj->spr_num = 0xFFFF;
			}
		}
	}
}

static bool level_update_objects_decors_helper(struct object_t *obj) {
	if (g_vars.level_force_x_scroll_flag != 0) {
		return false;
	}
	if (obj->y_pos <= g_vars.objects_tbl[1].y_pos) {
		return false;
	}
	const int prev_y_pos = g_vars.objects_tbl[1].y_pos;
	if (g_vars.current_platform_dy >= 0) {
		g_vars.objects_tbl[1].y_pos += g_vars.current_platform_dy;
	}
	const int prev_spr_num = g_vars.objects_tbl[1].spr_num;
	g_vars.objects_tbl[1].spr_num = 7;
	const bool ret = level_objects_collide(&g_vars.objects_tbl[1], obj);
	g_vars.objects_tbl[1].y_pos = prev_y_pos;
	g_vars.objects_tbl[1].spr_num = prev_spr_num;
	if (!ret) {
		return false;
	}
	g_vars.level_force_x_scroll_flag = 1;
	g_vars.objects_tbl[1].x_friction = 0;
	g_vars.objects_tbl[1].x_pos += g_vars.current_platform_dx;
	level_player_reset();
	const int y = obj->y_pos - spr_size_tbl[(obj->spr_num & 0x1FFF) * 2 + 1];
	if (g_vars.objects_tbl[1].y_pos > y) {
		g_vars.objects_tbl[1].y_pos = y + 1;
		g_vars.objects_tbl[1].data.p.y_velocity = 1;
	} else {
		g_vars.objects_tbl[1].data.p.y_velocity = g_vars.current_platform_dy << 4;
	}
	return true;
}

static void level_update_objects_decors() {
	g_vars.level_force_x_scroll_flag = 0;
	int count = 7;
	struct object_t *obj = &g_vars.objects_tbl[91];
	for (int i = 0; i < MAX_LEVEL_PLATFORMS; ++i) {
		struct level_platform_t *platform = &g_res.level.platforms_tbl[i];
		if (platform->spr_num == 0xFFFF) {
			continue;
		}
		if ((platform->flags & 15) == 8) {
			g_vars.current_platform_dx = 0;
			platform->y_pos -= platform->type8.y_delta;
			if (platform->type8.state == 0) {
				int a = platform->type8.y_delta - 8;
				if (a < 0) {
					a = 0;
					platform->type8.y_velocity = 0;
				}
				platform->type8.y_delta = a;
				if (platform->flags & 0x40) {
					if (platform->type8.y_velocity != 0 && --platform->type8.counter == 0) {
						platform->type8.state = 1;
						platform->type8.y_velocity = 0;
					}
				}
			} else if (platform->type8.state == 1) {
				int y = platform->type8.y_velocity;
				if (y < 192) {
					platform->type8.y_velocity += 8;
				}
				y >>= 4;
				g_vars.current_platform_dy = y;
				platform->type8.y_delta += y;
				const int tile_x =  platform->x_pos >> 4;
				const int tile_y = (platform->y_pos + platform->type8.y_delta) >> 4;
				const int y2 = g_vars.tilemap.h - 1 - tile_y;
				if (y2 < 0) {
					if ((-y2) >= 3) {
						platform->type8.state = 2;
						platform->type8.counter = 22;
					}
				} else {
					const uint8_t tile_num = level_get_tile((tile_y << 8) | tile_x);
					const uint8_t tile_attr1 = g_res.level.tile_attributes1[tile_num];
					if (tile_attr1 != 0 && tile_attr1 != 6) {
						platform->type8.state = 2;
						platform->type8.counter = 22;
					}
				}
			} else if (platform->type8.state == 2) {
				if ((platform->flags & 0x40) == 0 && --platform->type8.counter == 0) {
					platform->type8.state = 0;
					platform->type8.counter = platform->type8.unk9;
				}
			}
			platform->y_pos += platform->type8.y_delta;
		} else {
			g_vars.current_platform_dx = 0;
			g_vars.current_platform_dy = 0;
			if (platform->other.velocity != 0 || platform->other.max_velocity < 0 || (platform->flags & 0xC0) != 0) {
				if (platform->other.max_velocity > platform->other.velocity) {
					++platform->other.velocity;
				} else if (platform->other.max_velocity < platform->other.velocity) {
					--platform->other.velocity;
				}
				int al = platform->other.velocity;
				int ah = platform->other.velocity;
				const int type = platform->flags & 7;
				if (type == 0) {
					al = 0;
					ah = -ah;
				} else if (type == 1) {
					ah = -ah;
				} else if (type == 2) {
					ah = 0;
				} else if (type == 4) {
					al = 0;
				} else if (type == 5) {
					al = -al;
				} else if (type == 6) {
					al = -al;
					ah = 0;
				} else if (type == 7) {
					al = -al;
					ah = -ah;
				}
				g_vars.current_platform_dx = al;
				platform->x_pos += al;
				g_vars.current_platform_dy = ah;
				platform->y_pos += ah;
				if (platform->other.max_velocity == platform->other.velocity) {
					uint16_t ax = platform->other.counter + 1;
					if (platform->other.unkA == ax) {
						platform->other.max_velocity = -platform->other.max_velocity;
						ax = 0;
					}
					platform->other.counter = ax;
				}
			}
		}
		const int x_pos = platform->x_pos - (g_vars.tilemap.x << 4);
		if (x_pos >= TILEMAP_SCREEN_W + 32 || x_pos <= -32) {
			continue;
		}
		const int y_pos = platform->y_pos - (g_vars.tilemap.y << 4);
		if (y_pos >= TILEMAP_SCREEN_H + 32 || y_pos <= -32) {
			continue;
		}
		obj->x_pos = platform->x_pos;
		obj->y_pos = platform->y_pos;
		obj->spr_num = platform->spr_num;
		obj->data.t.ref = platform;
		if (g_vars.objects_tbl[1].data.p.y_velocity > -16) {
			platform->flags |= 0x40;
			if (level_update_objects_decors_helper(obj)) {
				++obj;
				--count;
				if (count == 0) {
					return;
				}
				continue;
			}
		}
		platform->flags &= ~0x40;
		obj->y_pos -= 2;
		++obj;
		--count;
		if (count == 0) {
			return;
		}
	}
	do {
		obj->spr_num = 0xFFFF;
		++obj;
	} while (--count != 0);
}

static void level_update_player_x_velocity() {
	bool left = false;
	int x_vel = g_vars.objects_tbl[1].x_velocity;
	if (x_vel < 0) {
		x_vel = -x_vel;
		left = true;
	}
	x_vel -= 12 >> g_vars.objects_tbl[1].x_friction;
	if (x_vel < 0) {
		x_vel = 0;
	}
	if (left) {
		x_vel = -x_vel;
	}
	g_vars.objects_tbl[1].x_velocity = x_vel;
}

static void level_update_player_hdir_x_velocity(int x_vel) {
	int16_t f = 0;
	if (g_vars.input.key_hdir) {
		f = g_vars.objects_tbl[1].data.p.hdir << 4;
	}
	f >>= g_vars.objects_tbl[1].x_friction;
	const int16_t dx = g_vars.objects_tbl[1].x_velocity + f;
	if (dx < x_vel) {
		x_vel = -x_vel;
		if (dx > x_vel) {
			x_vel = dx;
		}
	}
	g_vars.objects_tbl[1].x_velocity = x_vel;
}

static void level_update_screen_x_velocity() {
	if ((g_options.cheats & CHEATS_NO_WIND) == 0) {
		g_vars.objects_tbl[1].x_velocity -= g_vars.snow.value >> 3;
	}
	if (g_vars.objects_tbl[1].x_velocity < -96) {
		g_vars.objects_tbl[1].x_velocity = -96;
	}
}

static void level_update_player_y_velocity(int y_vel) {
	int f = 16;
	if (g_vars.player_gravity_flag == 1) {
		f = 4;
		y_vel >>= 3;
	}
	const int y = g_vars.objects_tbl[1].data.p.y_velocity + f;
	if (y < y_vel) {
		y_vel = y;
	}
	g_vars.objects_tbl[1].data.p.y_velocity = y_vel;
}

static void level_update_player_jump() {
	level_update_player_hdir_x_velocity(80);
	level_update_player_y_velocity(192);
	uint8_t num;
	if (g_vars.player_flying_flag != 0) {
		num = 45;
		if (g_vars.objects_tbl[1].data.p.y_velocity >= 0) {
			++num;
		}
	} else {
		if (g_vars.objects_tbl[1].data.p.y_velocity <= 0) {
			return;
		}
		g_vars.player_nojump_counter = 6;
		if (g_vars.player_anim_0x40_flag != 0) {
			return;
		}
		num = 12;
		if (g_vars.player_jumping_counter >= 12) {
			++num;
		}
	}
	g_vars.objects_tbl[1].spr_num = num | ((g_vars.objects_tbl[1].data.p.hdir < 0) ? 0x8000 : 0);
}

static const uint8_t *level_update_player_anim1_num(uint8_t num, uint16_t anim) {
	assert((anim & 1) == 0);
	assert(anim == num * 2);
	if (g_vars.objects_tbl[1].data.p.special_anim_num != num) {
		g_vars.objects_tbl[1].data.p.special_anim_num = num;
		g_vars.objects_tbl[1].data.p.anim = object_anim_tbl[anim / 2];
	}
	return g_vars.objects_tbl[1].data.p.anim;
}

static const uint8_t *level_update_player_anim2_num(uint8_t num, uint16_t anim) {
	assert((anim & 1) == 0);
	assert(anim == num * 2);
	if (g_vars.objects_tbl[1].data.p.current_anim_num != num) {
		g_vars.objects_tbl[1].data.p.current_anim_num = num;
		g_vars.objects_tbl[1].data.p.anim = object_anim_tbl[anim / 2];
	}
	return g_vars.objects_tbl[1].data.p.anim;
}

static struct object_t *level_find_object_club_projectile() {
	for (int i = 0; i < 4; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[2 + i];
		if (obj->spr_num == 0xFFFF) {
			return obj;
		}
	}
	return 0;
}

static void level_update_player_anim_3_6_7(uint8_t al) {
	const uint8_t *p = level_update_player_anim2_num(al, al * 2);
	level_update_object_anim(p);
	level_update_player_x_velocity();
	if (g_vars.player_moving_counter < UCHAR_MAX) {
		++g_vars.player_moving_counter;
	}
	al = club_anim_tbl[g_vars.player_club_type].power;
	if (g_vars.player_club_powerup_duration != 0) {
		al <<= 2;
	}
	g_vars.player_club_power = al;
	g_vars.player_anim_0x40_flag = (g_vars.player_current_anim_type & 0x40) ^ 0x40;
	if (g_vars.player_anim_0x40_flag == 0) {
		g_vars.player_club_anim_duration = club_anim_tbl[g_vars.player_club_type].a;
		if (g_vars.player_club_type == 0) {
			play_sound(5);
		} else if (g_vars.player_club_type == 1) {
			play_sound(0);
		} else {
			play_sound(10);
		}
		int16_t dy = 0;
		al = g_vars.objects_tbl[1].data.p.current_anim_num;
		if (al != 6) {
			dy = -32;
			if (al != 3) {
				dy = -48;
				if (g_vars.level_draw_counter & 3) {
					level_init_object_hit_from_player_pos();
				}
			}
		}
		if (g_vars.level_force_x_scroll_flag == 0) {
			/* off the ground when using the club */
			g_vars.objects_tbl[1].data.p.y_velocity += dy;
		}
		al = club_anim_tbl[g_vars.player_club_type].c;
		if (al & 1) {
			/* club with projectile */
			struct object_t *obj = level_find_object_club_projectile();
			if (obj) {
				obj->x_friction = (al >> 1) & 3;
				const uint8_t *p = club_anim_tbl[g_vars.player_club_type].anim;
				do {
					p += 2;
				} while (READ_LE_UINT16(p) != 0x55AA);
				p += 6;
				obj->data.c.anim = p;
				int num = READ_LE_UINT16(p);
				int16_t y_vel = READ_LE_UINT16(p - 2);
				int16_t x_vel = READ_LE_UINT16(p - 4);

				obj->data.c.y_velocity = y_vel;
				if (g_vars.objects_tbl[1].data.p.hdir < 0) {
					num |= 0x8000;
					x_vel = -x_vel;
				}
				obj->spr_num = num;
				obj->x_velocity = x_vel;
				obj->x_pos = g_vars.objects_tbl[1].x_pos + (x_vel >> 4);
				obj->y_pos = g_vars.objects_tbl[1].y_pos + (y_vel >> 4);
				g_vars.objects_tbl[0].spr_num = 0xFFFF;
				return;
			}
		}
		if (g_vars.player_jumping_counter != 0) {
			g_vars.objects_tbl[0].spr_num = 0xFFFF;
			return;
		}
	}
	const uint16_t spr_num = g_vars.objects_tbl[1].spr_num;
	p = club_anim_tbl[g_vars.player_club_type].anim;
	while (READ_LE_UINT16(p) != 0x55AA) {
		if (READ_LE_UINT16(p) == (spr_num & 0x1FFF)) {
			int16_t num = READ_LE_UINT16(p + 2);
			int16_t x = READ_LE_UINT16(p + 4);
			if (spr_num & 0x8000) {
				x = -x;
			}
			int16_t y = READ_LE_UINT16(p + 6);
			g_vars.objects_tbl[0].spr_num = num | (spr_num & 0x8000);
			g_vars.objects_tbl[0].x_pos = g_vars.objects_tbl[1].x_pos + (g_vars.objects_tbl[1].x_velocity >> 4) - x;
			g_vars.objects_tbl[0].y_pos = g_vars.objects_tbl[1].y_pos + (g_vars.objects_tbl[1].data.p.y_velocity >> 4) - y;
			break;
		}
		p += 8;
	}
}

static void level_update_player_anim_0(uint8_t al) {
	if (g_vars.player_anim_0x40_flag != 0) {
		level_update_player_anim_3_6_7(g_vars.objects_tbl[1].data.p.current_anim_num);
		return;
	}
	g_vars.player_unk_counter1 = 0;
	level_update_screen_x_velocity();
	level_update_player_x_velocity();
	if (g_vars.level_force_x_scroll_flag == 0 && g_vars.objects_tbl[1].data.p.y_velocity != 0) {
		if (g_vars.player_anim2_counter > 4) {
			level_update_screen_x_velocity();
		}
		return;
	}
	const int x_vel = abs(g_vars.objects_tbl[1].x_velocity);
	if (x_vel >= 8) {
		const bool same_direction = ((g_vars.objects_tbl[1].spr_num & 0x8000) != 0) && (g_vars.objects_tbl[1].x_velocity < 0);
		if (same_direction) {
			const uint8_t *anim = level_update_player_anim1_num(18, 36);
			level_update_object_anim(anim);
			if ((g_vars.level_draw_counter & 3) == 0) {
				level_init_object_hit_from_player_pos();
			}
			g_vars.objects_tbl[1].data.p.current_anim_num = 0;
			return;
		}

	} else if (x_vel == 0) {
		if (g_vars.player_moving_counter >= 30) {
			if ((g_vars.input.key_right & g_vars.input.key_left) == 0) {
				/* player stopped moving, show exhausted animation */
				g_vars.player_moving_counter -= 3;
				const uint8_t *anim = level_update_player_anim1_num(16, 32);
				level_update_object_anim(anim);
				g_vars.objects_tbl[1].data.p.current_anim_num = 0;
				return;
			}
		} else {
			if ((g_vars.input.key_right & g_vars.input.key_left) != 0) {
				const uint8_t *anim = level_update_player_anim1_num(19, 38);
				level_update_object_anim(anim);
				if ((g_res.level.scrolling_mask & 2) == 0 && g_vars.tilemap_noscroll_flag == 0) {
					const int dx = (g_vars.objects_tbl[1].x_pos >> 4) - g_vars.tilemap.x;
					if ((g_vars.objects_tbl[1].spr_num & 0x8000) == 0) {
						if (dx > 2) {
							level_adjust_hscroll_right();
						}
					} else {
						if (dx < 17) {
							level_adjust_hscroll_left();
						}
					}
				}
				g_vars.objects_tbl[1].data.p.current_anim_num = 0;
				return;
			}
			/* show idle animation */
			static const uint16_t data[] = { 16, 96, 128, 176, 256, 512 };
			const uint16_t timer = timer_get_counter() & 511;
			int offset = 0;
			while (1) {
				if (timer < data[offset]) {
					break;
				}
				++offset;
				if (timer < data[offset]) {
					const uint8_t *anim = level_update_player_anim1_num(17, 34);
					level_update_object_anim(anim);
					g_vars.objects_tbl[1].data.p.current_anim_num = 0;
					return;
				}
				++offset;
			}
		}
	}
	const uint8_t *p = object_anim_tbl[al];
	g_vars.objects_tbl[1].data.p.anim = p;
	g_vars.objects_tbl[1].spr_num = READ_LE_UINT16(p) | ((g_vars.objects_tbl[1].data.p.hdir < 0) ? 0x8000 : 0);
	g_vars.objects_tbl[1].data.p.special_anim_num = 0;
	g_vars.objects_tbl[1].data.p.current_anim_num = 0;
}

static void level_update_player_anim_1(uint8_t al) {
	if (g_vars.player_anim_0x40_flag != 0) {
		level_update_player_anim_3_6_7(g_vars.objects_tbl[1].data.p.current_anim_num);
		return;
	}
	const int x_vel = abs(g_vars.objects_tbl[1].x_velocity);
	if (x_vel >= 64 && g_vars.player_flying_flag != 0) {
		if (g_vars.player_unk_counter1 < UCHAR_MAX) {
			++g_vars.player_unk_counter1;
		}
		if (g_vars.player_unk_counter1 == 23) {
			level_init_object_hit_from_player_pos();
		}
	}
	if (g_vars.player_moving_counter < UCHAR_MAX) {
		++g_vars.player_moving_counter;
	}
	level_update_player_hdir_x_velocity(80);
	level_update_screen_x_velocity();
	const uint8_t *p = level_update_player_anim2_num(al, al * 2);
	level_update_object_anim(p);
}

static void level_update_player_anim_2(uint8_t al) {
	if (g_vars.player_anim_0x40_flag != 0) {
		level_update_player_anim_3_6_7(g_vars.objects_tbl[1].data.p.current_anim_num);
		return;
	}
	if (g_vars.player_nojump_counter != 0) {
		level_update_player_anim_0(al);
		return;
	}
	g_vars.level_force_x_scroll_flag = 0;
	uint8_t value = g_vars.player_anim2_counter;
	++g_vars.player_anim2_counter;
	if (value < 9) {
		static const int8_t y_tbl[] = { -65, -51, -35, -20, -10, -5, -2, -1, 0 };
		int16_t ax = y_tbl[value];
		if (g_vars.player_flying_flag != 0) {
			ax >>= 1;
		}
		g_vars.objects_tbl[1].data.p.y_velocity += ax;
	} else {
		level_update_player_y_velocity(192);
	}
	if (g_vars.objects_tbl[1].x_velocity >= 48) {
		level_update_player_x_velocity();
	} else {
		level_update_player_hdir_x_velocity(48);
	}
	const uint8_t *p = level_update_player_anim2_num(2, 4);
	level_update_object_anim(p);
	level_update_screen_x_velocity();
	level_update_screen_x_velocity();
}

static void level_update_player_club_power() {
	if (g_vars.player_club_powerup_duration <= 48) {
		g_vars.player_club_powerup_duration += 2;
	}
}

static void level_update_player_anim_4(uint8_t al) {
	if (g_vars.player_anim_0x40_flag != 0) {
		level_update_player_anim_3_6_7(g_vars.objects_tbl[1].data.p.current_anim_num);
		return;
	}
	g_vars.player_moving_counter = 0;
	g_vars.player_action_counter = 4;
	level_update_player_club_power();
	const int x_vel = abs(g_vars.objects_tbl[1].x_velocity);
	if (x_vel > 32) {
		level_update_player_anim_0(al);
	} else {
		level_update_player_hdir_x_velocity(32);
		const uint8_t *p = level_update_player_anim2_num(al, al * 2);
		level_update_object_anim(p);
	}
}

static void level_update_player_anim_5(uint8_t al) {
	if (g_vars.player_anim_0x40_flag != 0) {
		level_update_player_anim_3_6_7(g_vars.objects_tbl[1].data.p.current_anim_num);
		return;
	}
	g_vars.player_unk_counter1 = 0;
	g_vars.player_action_counter = 4;
	level_update_player_club_power();
	const uint8_t *p = level_update_player_anim2_num(al, al * 2);
	level_update_object_anim(p);
	level_update_player_x_velocity();
	level_update_player_club_power();
}

static void level_update_player_anim_8(uint8_t al) {
	level_update_screen_x_velocity();
	level_update_player_x_velocity();
	const uint8_t *p = level_update_player_anim2_num(al, al * 2);
	level_update_object_anim(p);
}

static void level_update_player_decor() {
	const int y_pos = (g_vars.objects_tbl[1].y_pos >> 4) - 1;
	const int spr_num = g_vars.objects_tbl[1].spr_num & 0x1FFF;
	int spr_h = spr_size_tbl[spr_num * 2 + 1];
	int dx = 9;
	if (g_vars.objects_tbl[1].x_velocity < 0) {
		dx = -9;
	} else if (g_vars.objects_tbl[1].x_velocity == 0) {
		dx = 0;
	}
	const int x_pos = g_vars.objects_tbl[1].x_pos >> 4;
	const int player_ypos_diff = abs((g_vars.objects_tbl[1].y_pos >> 4) - g_vars.tilemap.y);
	if (player_ypos_diff > (TILEMAP_SCREEN_H / 16)) {
		level_update_tile_type_2();
	} else {
		const int player_xpos_diff = abs((g_vars.objects_tbl[1].x_pos >> 4) - g_vars.tilemap.x);
		if (player_xpos_diff > (TILEMAP_SCREEN_W / 16)) {
			level_update_tile_type_2();
		} else if ((g_res.level.scrolling_mask & 4) != 0 && g_vars.objects_tbl[1].y_pos < (g_vars.tilemap.y << 4)) {
			level_update_tile_type_2();
		} else if (g_vars.objects_tbl[1].y_pos >= 0 && g_vars.objects_tbl[1].y_pos > ((g_vars.tilemap.h + 1) << 4)) {
			level_update_tile_type_2();
		}
	}
	g_vars.player_tile_flags = 0;
	level_update_tile0((y_pos << 8) | x_pos);
	if (g_vars.player_tile_flags == 1) {
		if (g_vars.level_force_x_scroll_flag != 0) {
			level_player_reset();
			g_vars.player_jumping_counter = 0;
		} else {
			level_update_player_jump();
			if (g_vars.objects_tbl[1].data.p.y_velocity > 0) {
				++g_vars.player_jumping_counter;
			}
		}
	} else {
		g_vars.player_jumping_counter = 0;
	}
	if (g_vars.objects_tbl[1].y_pos <= 0) {
		return;
	}
	int pos = (y_pos << 8) | ((dx + g_vars.objects_tbl[1].x_pos) >> 4);
	level_update_tile1(pos);
	while (1) {
		pos -= 0x100;
		if (pos < 0) {
			break;
		}
		spr_h -= 16;
		if (spr_h <= 0) {
			break;
		}
		level_update_tile2(pos);
	}
}

static void level_update_player_flying() {
	if (g_vars.player_flying_flag == 0) {
		return;
	}
	if (g_vars.player_gravity_flag != 0) {
		g_vars.objects_tbl[1].spr_num &= 0x8000;
		const int x_vel = abs(g_vars.objects_tbl[1].x_velocity);
		int num = 48;
		if (x_vel < 64) {
			++num;
			if (x_vel < 32) {
				++num;
			}
		}
		if (g_vars.player_flying_anim_index < 3) {
			num = 51 - g_vars.player_flying_anim_index;
		}
		g_vars.objects_tbl[1].spr_num |= num;
	}
	const uint8_t *p = &player_flying_anim_data[14];
	if ((g_vars.input.key_up | g_vars.input.key_down) == 0 && g_vars.player_flying_anim_index != 3) {
		if (g_vars.player_flying_anim_index < 3) {
			++g_vars.player_flying_anim_index;
		} else {
			--g_vars.player_flying_anim_index;
		}
	}
	const uint16_t player_spr_num = g_vars.objects_tbl[1].spr_num;
	for (uint16_t num; (num = READ_LE_UINT16(p)) != 0; p += 6) {
		if (num != (player_spr_num & 0x1FFF)) {
			continue;
		}
		int x_delta = (int8_t)p[4];
		int spr_num = READ_LE_UINT16(p + 2);
		if (spr_num >= 121) {
			 spr_num = READ_LE_UINT16(player_flying_anim_data + g_vars.player_flying_anim_index * 2);
		}
		if (player_spr_num & 0x8000) {
			spr_num |= 0x8000;
			x_delta = -x_delta;
		}
		if (g_vars.player_unk_counter1 > 24 && (spr_num & 0x1FFF) < 121) {
			++spr_num;
		}
		g_vars.objects_tbl[0].spr_num = spr_num;
		g_vars.objects_tbl[0].x_pos += x_delta;
		g_vars.objects_tbl[0].y_pos += (int8_t)p[5];
		break;
	}
}

static void level_update_player() {
	g_vars.objects_tbl[0].spr_num = 0xFFFF;
	if (g_vars.input.keystate[0x3B]) {
		if (g_vars.restart_level_flag == 0) {
			level_player_die();
			g_vars.restart_level_flag = 1;
			return;
		}
	}
	if (g_vars.input.keystate[0x3C]) {
		g_vars.player_death_flag = 1;
		return;
	}
	input_check_ctrl_alt_w();
	if (_demo_inputs) {
		if (g_vars.input.demo_offset < g_res.keyblen) {
			if (g_vars.input.demo_offset == 0) {
				if (g_res.keyb[1] == 0xFF) {
					g_vars.input.demo_offset += 2;
				}
			}
			if (g_vars.input.demo_counter == 0) {
				const uint16_t num = READ_LE_UINT16(g_res.keyb + g_vars.input.demo_offset);
				g_vars.input.demo_offset += 2;
				g_vars.input.demo_mask = num & 255;
				g_vars.input.demo_counter = num >> 8;
			} else {
				--g_vars.input.demo_counter;
			}
			g_vars.input.key_left = (g_vars.input.demo_mask & 1) != 0;
			g_vars.input.key_right = (g_vars.input.demo_mask & 2) != 0;
			g_vars.input.key_up = (g_vars.input.demo_mask & 4) != 0;
			g_vars.input.key_down = (g_vars.input.demo_mask & 8) != 0;
			g_vars.input.key_space = (g_vars.input.demo_mask & 0x10) != 0;
		} else {
			g_vars.player_death_flag = 1;
		}
	}
	g_vars.input.key_vdir = g_vars.input.key_up | g_vars.input.key_down;
	g_vars.input.key_hdir = g_vars.input.key_left | g_vars.input.key_right;
	if (g_vars.input.key_right != 0) {
		if (g_vars.input.key_left == 0) {
			if (g_vars.objects_tbl[1].data.p.hdir != 1) {
				g_vars.objects_tbl[1].data.p.hdir = 1;
				g_vars.player_update_counter = 0;
			}
		}
	} else {
		if (g_vars.input.key_left != 0) {
			if (g_vars.objects_tbl[1].data.p.hdir != -1) {
				g_vars.objects_tbl[1].data.p.hdir = -1;
				g_vars.player_update_counter = 0;
			}
		}
	}
	uint16_t mask = 0;
	mask |= g_vars.input.key_right & 1;
	mask <<= 1;
	mask |= g_vars.input.key_left & 1;
	mask <<= 1;
	mask |= g_vars.input.key_up & 1;
	mask <<= 1;
	mask |= g_vars.input.key_down & 1;
	mask <<= 1;
	mask |= g_vars.input.key_space & 1;
	if (g_vars.player_club_anim_duration != 0) {
		mask = 0;
	}
	uint8_t dl = g_vars.objects_tbl[1].hit_counter;
	g_vars.objects_tbl[0].hit_counter = dl;
	uint8_t al = player_anim_lut[mask];
	if (dl >= 22) {
		al = 8;
	}
	if (g_vars.objects_tbl[1].data.p.current_anim_num != al) {
		g_vars.player_update_counter = 0;
		g_vars.objects_tbl[1].data.p.special_anim_num = 0;
	}
	if (g_vars.player_update_counter < USHRT_MAX) {
		++g_vars.player_update_counter;
	}
	if (g_vars.player_flying_flag != 0) {
		mask += 0x40;
		if ((g_vars.player_flying_flag &= 1) != 0) {
			if (g_vars.input.key_up) {
				if (g_vars.player_flying_anim_index == 6) {
					if (g_vars.objects_tbl[1].data.p.y_velocity > 16) {
						g_vars.player_flying_flag |= 2;
					}
				} else {
					++g_vars.player_flying_anim_index;
				}
				if (g_vars.player_flying_counter != 0) {
					--g_vars.player_flying_counter;
					if (g_vars.player_flying_anim_index >= 4) {
						g_vars.objects_tbl[1].data.p.y_velocity = -64;
					}
					goto update_pos;
				}
			}
			if (g_vars.input.key_down) {
				if (g_vars.player_flying_counter != 0) {
					--g_vars.player_flying_counter;
				}
				goto update_pos;
			} else {
				if (g_vars.player_flying_anim_index != 0) {
					--g_vars.player_flying_anim_index;
				}
				g_vars.player_flying_flag |= 2;
				if (g_vars.player_flying_anim_index <= 1) {
					const int x = abs(g_vars.objects_tbl[1].x_velocity >> 4);
					if (g_vars.player_flying_counter < x * 5) {
						g_vars.player_flying_counter += x;
					}
				}
				goto update_pos;
			}
		} else {
			if (g_vars.objects_tbl[1].data.p.y_velocity > 160) {
				g_vars.player_flying_flag = 1;
			}
		}
	}
	switch (al) {
	case 0:
		level_update_player_anim_0(al);
		break;
	case 1:
		level_update_player_anim_1(al);
		break;
	case 2:
		level_update_player_anim_2(al);
		break;
	case 3:
	case 6:
	case 7:
		level_update_player_anim_3_6_7(al);
		break;
	case 4:
		level_update_player_anim_4(al);
		break;
	case 5:
		level_update_player_anim_5(al);
		break;
	case 8:
		level_update_player_anim_8(al);
		break;
	default:
		print_warning("Unhandled anim_lut %d mask %d", al, mask);
		break;
	}
update_pos:
	{
		const int x_pos = (g_vars.objects_tbl[1].x_velocity >> 4) + g_vars.objects_tbl[1].x_pos;
		const int end_x = (g_res.level.tilemap_w + 20) << 4;
		if (end_x > x_pos && x_pos >= 8 && x_pos < 511 * 8) {
			g_vars.objects_tbl[1].x_pos = x_pos;
		}
	}
	g_vars.objects_tbl[1].y_pos += g_vars.objects_tbl[1].data.p.y_velocity >> 4;
	level_update_player_decor();
	level_update_player_flying();
	if (g_vars.player_club_powerup_duration > 0) {
		--g_vars.player_club_powerup_duration;
	}
	if (g_vars.player_club_anim_duration > 0) {
		--g_vars.player_club_anim_duration;
	}
	if (g_vars.shake_screen_counter > 0) {
		--g_vars.shake_screen_counter;
	}
	if (g_vars.restart_level_flag > 0) {
		--g_vars.restart_level_flag;
	}
	if (g_vars.player_action_counter > 0) {
		--g_vars.player_action_counter;
	}
	if (g_vars.player_bonus_letters_blinking_counter > 0) {
		--g_vars.player_bonus_letters_blinking_counter;
	}
	if (g_vars.player_hit_monster_counter > 0) {
		--g_vars.player_hit_monster_counter;
	}
}

static void level_add_object23_bones() {
	g_vars.current_bonus.x_pos = g_vars.objects_tbl[1].x_pos;
	g_vars.current_bonus.y_pos = g_vars.objects_tbl[1].y_pos - 48;
	const int count = g_vars.player_energy * 6 + g_vars.bonus_energy_counter;
	if (count != 0) {
		g_vars.player_energy = 0;
		g_vars.bonus_energy_counter = 0;
		g_vars.current_bonus.spr_num = 0x2046; /* bones */
		level_add_object23_bonus(48, -128, count);
	}
}

static void level_clear_item(struct object_t *obj) {
	struct level_item_t *item = obj->data.t.ref;
	if (item) {
		assert(item >= &g_res.level.items_tbl[0] && item < &g_res.level.items_tbl[MAX_LEVEL_ITEMS]);
		item->spr_num = 0xFFFF;
	}
}

static void level_update_player_collision() {
	struct object_t *obj_player = &g_vars.objects_tbl[1];
	/* monsters */
	if (g_vars.objects_tbl[1].hit_counter == 0) {
		for (int i = 0; i < MONSTERS_COUNT; ++i) {
			struct object_t *obj = &g_vars.objects_tbl[11 + i];
			if (obj->spr_num == 0xFFFF || (obj->spr_num & 0x2000) == 0) {
				continue;
			}
			struct level_monster_t *m = obj->data.m.ref;
			if (m->flags & 0x10) {
				continue;
			}
			if (obj->data.m.state == 0xFF) {
				continue;
			}
			if (g_options.cheats & CHEATS_NO_HIT) {
				continue;
			}
			if (!level_objects_collide(obj_player, obj)) {
				continue;
			}
			if (g_vars.player_hit_monster_counter != 0) {
				level_monster_die(obj, m, &g_vars.objects_tbl[1]);
				continue;
			}
			if (!g_vars.player_jump_monster_flag || g_vars.objects_tbl[1].data.p.y_velocity < 0) {
				play_sound(9);
				m->flags &= ~1;
				g_vars.objects_tbl[1].hit_counter = 44;
				g_vars.player_anim_0x40_flag = 0;
				g_vars.objects_tbl[1].data.p.y_velocity = -128;
				g_vars.objects_tbl[1].x_velocity = -(g_vars.objects_tbl[1].x_velocity << 2);
				g_vars.player_gravity_flag = 0;
				if (g_vars.player_flying_flag != 0) {
					g_vars.player_flying_flag = 0;
				} else {
					if ((g_options.cheats & CHEATS_UNLIMITED_ENERGY) == 0) {
						--g_vars.player_energy;
						if (g_vars.player_energy < 0) {
							level_player_die();
						}
					}
				}
				return;
			}
			if (!g_vars.player_gravity_flag) {
				/* jumping on a monster */
				play_sound(3);
				int num = obj->data.m.hit_jump_counter >> 2;
				if (num != 11) {
					obj->data.m.hit_jump_counter += 4;
					++num;
					if ((num & 1) == 0) {
						static const uint16_t data[] = { 0xFF46, 0xE0, 0xE1, 0x12D, 0x12E, 0x12F };
						level_add_object75_score(obj_player, (int16_t)data[num >> 1]);
					}
				}
				g_vars.objects_tbl[1].data.p.y_velocity = (g_vars.input.key_up != 0) ? -224 : -64;
				g_vars.player_jumping_counter = 0;
				g_vars.objects_tbl[1].y_pos -= g_vars.monster.collide_y_dist;
			} else if (g_vars.objects_tbl[1].data.p.y_velocity <= 32) {
				g_vars.objects_tbl[1].data.p.y_velocity = -96;
				g_vars.player_jumping_counter = 0;
				g_vars.objects_tbl[1].y_pos -= g_vars.monster.collide_y_dist;
			} else {
				int num = (obj->data.m.hit_jump_counter) & 3;
				level_add_object75_score(obj_player, 82 + (num << 1));
				obj->spr_num |= 0x4000;
				if (num == 2) {
					obj->data.m.state = 0xFF;
					level_monster_update_anim(obj);
					m->flags &= ~8;
					obj->data.m.y_velocity = -200;
					int delta = abs(g_vars.objects_tbl[1].data.p.y_velocity);
					if (obj->x_pos <= g_vars.objects_tbl[1].x_pos) {
						delta = -delta;
					}
					obj->data.m.x_velocity = delta * 3;
				} else {
					++obj->data.m.hit_jump_counter;
					g_vars.objects_tbl[1].data.p.y_velocity = -96;
					g_vars.player_jumping_counter = 0;
					g_vars.objects_tbl[1].y_pos -= g_vars.monster.collide_y_dist;
				}
			}
			return;
		}
	}
	/* bonuses */
	for (int i = 0; i < 52; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[23 + i];
		if (obj->spr_num == 0xFFFF || (obj->spr_num & 0x2000) == 0) {
			continue;
		}
		if (obj->data.t.counter > 188) {
			continue;
		}
		if (!level_objects_collide(obj, obj_player)) {
			continue;
		}
		g_vars.current_bonus.spr_num = obj->spr_num;
		const int num = (obj->spr_num & 0x1FFF) - 53;
		obj->spr_num = 0xFFFF;
		if (num == 226) {
			if (g_vars.level_num == 2) {
				g_vars.level_num = 12;
			} else if (g_vars.level_num == 13) {
				g_vars.level_num = 2;
			} else if (g_vars.level_num == 6) {
				g_vars.level_num = 14;
			} else if (g_vars.level_num == 15) {
				g_vars.level_num = 6;
			}
			g_vars.level_completed_flag = 1;
			return;
		} else if (num == 258) {
			g_vars.level_completed_flag = 0xFF; /* game completed */
			return;
		} else if (num == 228) { /* checkpoint */
			g_vars.tilemap_start_x_pos = g_vars.objects_tbl[1].x_pos;
			g_vars.tilemap_start_y_pos = g_vars.objects_tbl[1].y_pos;
			for (int i = 0; i < MAX_LEVEL_ITEMS; ++i) {
				struct level_item_t *item = &g_res.level.items_tbl[i];
				if (item->spr_num == 280) {
					++item->spr_num;
				}
			}
			struct level_item_t *item = obj->data.t.ref;
			if (item) {
				assert(item >= &g_res.level.items_tbl[0] && item < &g_res.level.items_tbl[MAX_LEVEL_ITEMS]);
				item->spr_num -= 1;
			}
		} else if (num == 174) {
			++g_vars.player_lifes;
			play_sound(4);
			level_add_object75_score(&g_vars.objects_tbl[1], 227);
			level_clear_item(obj);
		} else if (num == 13) {
			play_sound(8);
			g_vars.player_club_type = 0;
			level_clear_item(obj);
		} else if (num == 182) {
			play_sound(8);
			g_vars.player_club_type = 1;
			level_clear_item(obj);
		} else if (num == 44) {
			play_sound(8);
			g_vars.player_club_type = 2;
			level_clear_item(obj);
		} else if (num == 224) {
			play_sound(8);
			g_vars.player_club_type = 3;
			level_clear_item(obj);
		} else if (num <= 20) {
			play_sound(8);
			level_clear_item(obj);
			++g_vars.bonus_energy_counter;
			if (g_vars.bonus_energy_counter >= 6) {
				if (g_vars.player_energy != 3) {
					++g_vars.player_energy;
					g_vars.bonus_energy_counter = 0;
					level_add_object75_score(obj, 226);
				}
			}
		} else if (num <= 44) {
			play_sound(8);
			const int index = num - 39;
			if (index >= 0 && index <= 4) {
				g_vars.player_bonus_letters_mask |= (1 << index);
				level_clear_item(obj);
			}
		} else if (num <= 50) {
			play_sound(8);
			g_vars.player_utensils_mask |= 1 << (num - 45);
			for (int j = 0; j < MAX_LEVEL_ITEMS; ++j) {
				struct level_item_t *item = &g_res.level.items_tbl[j];
				if (item->spr_num == 278) { /* end of level semaphore */
					++item->spr_num;
				}
			}
			level_clear_item(obj);
		} else if (num <= 64) { /* food */
			play_sound(4);
			if (obj->data.t.y_velocity >= 128) {
				obj->data.t.y_velocity = -obj->data.t.y_velocity;
				int x = 32;
				if ((random_get_number() & 1) != 0) {
					x = -x;
					g_vars.shake_screen_counter = 7;
				}
				obj->x_velocity = x;
				obj->spr_num = g_vars.current_bonus.spr_num;
			} else {
				const int index = num - 57;
				++g_vars.level_items_count_tbl[index];
				++g_vars.level_items_total_count;
				level_add_object75_score(obj, score_spr_lut[index] + 74);
				if (obj->data.t.ref) {
					++g_vars.level_complete_bonuses_count;
				}
			}
		} else if (num <= 74) {
			play_sound(8);
			if (g_vars.player_flying_flag == 0) {
				g_vars.player_flying_flag = 1;
				g_vars.objects_tbl[1].data.p.current_anim_num = 0xFF;
				level_clear_item(obj);
			}
		} else if (num <= 166) {
			play_sound(8);
			if (num == 145) { /* tap */
				for (int i = 0; i < 20; ++i) {
					g_vars.fly_tbl[i].x_pos = _undefined;
				}
			}
			const int index = num - 57;
			++g_vars.level_items_count_tbl[index];
			++g_vars.level_items_total_count;
			level_add_object75_score(obj, score_spr_lut[index] + 74);
		} else if (num == 167 || num == 168) {
			play_sound(1);
			g_vars.objects_tbl[1].hit_counter = 44;
			g_vars.objects_tbl[1].data.p.special_anim_num = 0;
			g_vars.objects_tbl[1].data.p.current_anim_num = 8;
			level_add_object23_bones();
			g_vars.shake_screen_counter = 7;
			level_clear_item(obj);
			level_add_object75_score(obj, 228);
		} else if (num == 173) {
			play_sound(4);
			if (g_vars.player_energy < 3) {
				++g_vars.player_energy;
				level_clear_item(obj);
			}
		} else if (num == 169) {
			play_sound(0);
			for (int i = 0; i < MONSTERS_COUNT; ++i) {
				struct object_t *obj = &g_vars.objects_tbl[11 + i];
				if (obj->spr_num == 0xFFFF) {
					continue;
				}
				struct level_monster_t *m = obj->data.m.ref;
				if (m->flags & 0x10) {
					continue;
				}
				if ((obj->spr_num & 0x2000) == 0) {
					continue;
				}
				level_monster_die(obj, m, obj_player);
			}
			g_vars.shake_screen_counter = 9;
			level_clear_item(obj);
			level_add_object75_score(obj, 230);
		} else if (num == 170) { /* bomb */
			play_sound(0);
			for (int i = 0; i < MONSTERS_COUNT; ++i) {
				struct object_t *obj = &g_vars.objects_tbl[11 + i];
				if (obj->spr_num == 0xFFFF) {
					continue;
				}
				struct level_monster_t *m = obj->data.m.ref;
				if (m->flags & 0x10) {
					continue;
				}
				if ((obj->spr_num & 0x2000) == 0) {
					continue;
				}
				g_vars.current_bonus.x_pos = obj->x_pos;
				g_vars.current_bonus.y_pos = obj->y_pos;
				obj->data.m.state = 0xFF;
				obj->spr_num = 0xFFFF;
				level_add_bonuses_4x();
			}
			level_clear_item(obj);
		} else if (num == 181) { /* light off */
			if (g_vars.light.state != 1) {
				play_sound(1);
				g_vars.light.palette_flag2 = 0;
				g_vars.light.palette_flag1 = 1;
				g_vars.light.palette_counter = 0;
				g_vars.light.state = 1;
			}
			level_clear_item(obj);
		} else if (num == 180) { /* light on */
			if (g_vars.light.state != 0) {
				g_vars.light.palette_flag1 = 0;
				g_vars.light.palette_flag2 = 1;
				g_vars.light.palette_counter = 0;
				g_vars.light.state = 0;
			}
			level_clear_item(obj);
		} else if (num == 458 || num == 459) { /* boss projectiles */
			if (g_options.cheats & CHEATS_UNLIMITED_ENERGY) {
				continue;
			}
			if (g_vars.player_energy < 1) {
				level_player_die();
			} else {
				play_sound(1);
				g_vars.objects_tbl[1].hit_counter = 44;
				g_vars.objects_tbl[1].data.p.special_anim_num = 0;
				g_vars.objects_tbl[1].data.p.current_anim_num = 8;
				--g_vars.player_energy;
				const uint8_t energy = g_vars.player_energy;
				g_vars.player_energy = 1;
				level_add_object23_bones();
				g_vars.player_energy = energy;
			}
		}
	}
}

static void level_update_objects_items() {
	struct object_t *obj = &g_vars.objects_tbl[55];
	int count = 20;
	for (int i = 0; i < MAX_LEVEL_ITEMS; ++i) {
		struct level_item_t *level_item = &g_res.level.items_tbl[i];
		if (level_item->spr_num == 0xFFFF) {
			continue;
		}
		const int16_t x = (level_item->x_pos >> 4) - g_vars.tilemap.x;
		if (x < 0 || x > 22) {
			continue;
		}
		const int16_t y = (level_item->y_pos >> 4) - g_vars.tilemap.y;
		if (y < 0 || y > 43) {
			continue;
		}
		obj->x_pos = level_item->x_pos;
		int16_t dy_pos = 0;
		if (g_vars.level_draw_counter & 1) {
			dy_pos = level_item->y_delta + 1;
			level_item->y_delta = dy_pos;
			if (dy_pos >= 4) {
				dy_pos = 1 - dy_pos;
				level_item->y_delta = dy_pos;
			}
		}
		level_item->y_pos += dy_pos;
		obj->y_pos = level_item->y_pos;
		obj->spr_num = level_item->spr_num;
		obj->data.t.ref = level_item;
		++obj;
		if (--count == 0) {
			return;
		}
	}
	do {
		obj->spr_num = 0xFFFF;
		++obj;
	} while (--count != 0);
}

static void level_update_gates() {
	if (g_vars.player_action_counter == 0) {
		return;
	}
	if (g_vars.player_flying_flag != 0) {
		return;
	}
	uint16_t pos = level_get_player_tile_pos();
	for (int i = 0; i < MAX_LEVEL_GATES; ++i) {
		struct level_gate_t *gate = &g_res.level.gates_tbl[i];
		if (gate->enter_pos == pos) {
			g_vars.objects_tbl[1].x_velocity = 0;
			g_vars.objects_tbl[1].data.p.y_velocity = 0;
			video_transition_close();
			g_vars.tilemap.scroll_dy = 0;
			const uint8_t tmp = g_res.level.tilemap_w;
			g_res.level.tilemap_w = 256 - 20;
			const uint16_t dst_pos = gate->dst_pos;
			g_vars.objects_tbl[1].x_pos = (dst_pos & 255) << 4;
			g_vars.objects_tbl[1].y_pos = (dst_pos >>  8) << 4;
			const uint8_t tilemap_y = gate->tilemap_pos >> 8;
			while (g_vars.tilemap.y != tilemap_y) {
				if (g_vars.tilemap.y > tilemap_y) {
					level_adjust_vscroll_up(16);
				} else {
					level_adjust_vscroll_down(16);
				}
			}
			const uint8_t tilemap_x = gate->tilemap_pos & 255;
			while (g_vars.tilemap.x != tilemap_x) {
				if (g_vars.tilemap.x > tilemap_x) {
					level_adjust_hscroll_left();
				} else {
					level_adjust_hscroll_right();
				}
			}
			g_res.level.tilemap_w = tmp;
			g_vars.tilemap_noscroll_flag = gate->scroll_flag;
			g_vars.player_action_counter = 0;
			if (g_vars.level_num == 5) {
				/* exit to boss (tree) */
				g_res.level.scrolling_mask &= 1;
				g_vars.boss_level5.unk1 = 0;
				g_vars.boss_level5.energy = 10;
				g_vars.boss_level5.state = 0;
				g_vars.boss_level5.spr103_pos = 0;
				g_vars.boss_level5.spr106_pos = 1;
				g_vars.boss_level5.unk6 = 0;
				g_vars.boss_level5.tick_counter = 110;
				g_vars.boss_level5.idle_counter = 8;
				memset(&g_vars.objects_tbl[91], 0xFF, sizeof(struct object_t) * 6);
			}
			level_draw_tilemap();
			level_update_objects_decors();
			level_update_objects_items();
			level_update_objects_monsters();
			video_transition_open();
			break;
		}
	}
}

static void level_update_columns() {
	const uint16_t player_pos = level_get_player_tile_pos();
	for (int i = 0; i < MAX_LEVEL_COLUMNS; ++i) {
		struct level_column_t *column = &g_res.level.columns_tbl[i];
		uint16_t pos = column->trigger_pos;
		if (pos == 0xFFFF) {
			continue;
		}
		if (pos != 0xFFFE) {
			const int x = -(pos - player_pos);
			if (x <= 8) {
				column->trigger_pos = 0xFFFE;
				g_vars.shake_screen_counter = 7;
			}
			continue;
		}
		g_vars.shake_screen_counter = 7;
		if ((g_vars.level_draw_counter & 3) != 0) {
			continue;
		}
		/* scroll the column up */
		pos = column->tilemap_pos - 0x100;
		for (int y = 0; y < column->h; ++y) {
			for (int x = 0; x < column->w; ++x) {
				g_res.leveldat[pos + x] = g_res.leveldat[pos + 0x100 + x];
			}
			pos += 0x100;
		}
		column->tilemap_pos -= 0x100;
		const uint16_t offset = column->tiles_offset_buf;
		for (int x = 0; x < column->w; ++x) {
			g_res.leveldat[pos + x] = g_vars.columns_tiles_buf[offset + x];
		}
		column->tiles_offset_buf -= column->w;
		--column->y_target;
		if (column->y_target == 0) {
			column->trigger_pos = 0xFFFF;
		}
	}
}

static void level_draw_front_tile(uint8_t tile_num, int x, int y) {
	const int num = g_res.level.front_tiles_lut[tile_num];
	assert(num < (g_res.frontlen / 128));
	g_sys.render_add_sprite(RENDER_SPR_FG, num, x * 16 - g_vars.tilemap.scroll_dx, y * 16 - g_vars.tilemap.scroll_dy, 0);
}

static void level_draw_front_tiles() {
	if ((g_vars.tile_attr2_flags & 0x40) == 0) { /* no front tiles */
		return;
	}
	uint16_t offset = (g_vars.tilemap.y << 8) | g_vars.tilemap.x;
	for (int y = 0; y < (TILEMAP_SCREEN_H / 16) + 1; ++y) {
		for (int x = 0; x < (TILEMAP_SCREEN_W / 16) + 1; ++x) {
			const uint8_t tile_num = level_get_tile(offset + x);
			if (g_res.level.tile_attributes2[tile_num] & 0x40) {
				level_draw_front_tile(tile_num, x, y);
			}
		}
		offset += 256;
	}
}

static void level_clear_items_spr_num_tbl() {
	memset(g_vars.level_items_count_tbl, 0, sizeof(g_vars.level_items_count_tbl));
	g_vars.level_items_total_count = 0;
}

static bool level_update_objects_anim() {
	if (g_vars.level_completed_flag == 1) {
		if (g_vars.level_num < 10) {
			level_completed_bonuses_animation();
		}
		++g_vars.level_num;
		level_reset();
		return false;
	} else if (g_vars.level_completed_flag & 0x80) {
		if (g_vars.level_num < 10) {
			g_vars.level_num = next_level_tbl[g_vars.level_num];
		} else {
			int num = 0;
			while (next_level_tbl[num] != g_vars.level_num) {
				++num;
			}
			g_vars.level_num = num;
			if (num < 10) {
				level_completed_bonuses_animation();
			} else {
				++g_vars.level_num;
			}
		}
		level_reset();
		return false;
	} else if (g_vars.restart_level_flag == 1) {
		level_player_death_animation();
		if (g_vars.player_death_flag == 0) {
			uint16_t spr_num_tbl[MAX_LEVEL_ITEMS];
			for (int i = 0; i < MAX_LEVEL_ITEMS; ++i) {
				spr_num_tbl[i] = g_res.level.items_tbl[i].spr_num;
			}
			for (int i = 0; i < MAX_LEVEL_BONUSES; ++i) {
				g_vars.level_bonuses_count_tbl[i] = (g_res.level.bonuses_tbl[i].pos != 0xFFFF) ? 1 : 0;
			}
			level_reset();
			g_vars.objects_tbl[1].x_pos = g_vars.tilemap_start_x_pos;
			g_vars.objects_tbl[1].y_pos = g_vars.tilemap_start_y_pos;
			level_init_tilemap();
			for (int i = 0; i < MAX_LEVEL_ITEMS; ++i) {
				const uint16_t spr_num = spr_num_tbl[i];
				const uint16_t num = spr_num - 53;
				if (num == 13 || num == 44 || num == 65 || num == 169 || num == 170 || num == 182 || num == 224) {
					continue;
				}
				g_res.level.items_tbl[i].spr_num = spr_num;
			}
			for (int i = 0; i < MAX_LEVEL_BONUSES; ++i) {
				if (g_vars.level_bonuses_count_tbl[i] == 0) {
					g_res.level.bonuses_tbl[i].pos = 0xFFFF;
				}
			}
			level_clear_items_spr_num_tbl();
			return true;
		}
	} else if (g_vars.player_death_flag == 1) {
		level_player_death_animation();
	} else if (g_vars.player_death_flag == 0xFF) {
		do_theend_screen();
		return false;
	} else {
		return true;
	}
	input_check_ctrl_alt_e();
	play_music(17);
	g_vars.objects_tbl[1].spr_num = 13;
	g_vars.level_num = 0;
	g_vars.score = 0;
	g_vars.score_extra_life = 0;
	level_clear_items_spr_num_tbl();
	return false;
}

static void level_clear_panel() {
	g_vars.panel.score = 0xFFFFFFFF;
	g_vars.panel.lifes = 0xFF;
	g_vars.panel.energy = 0xFF;
	g_vars.panel.bonus_letters_mask = 0xFF;
	const uint8_t *src = g_res.allfonts + 41 * 48;
	video_draw_panel(src);
}

static void level_draw_panel() {
	const uint8_t *src = g_res.allfonts + 41 * 48;
	video_draw_panel(src);
	if (g_vars.player_lifes != g_vars.panel.lifes || _redraw_panel) {
		g_vars.panel.lifes = g_vars.player_lifes;
		video_draw_panel_number(0x1CED, MIN(g_vars.panel.lifes, 9));
	}
	if (g_vars.score != g_vars.panel.score || _redraw_panel) {
		g_vars.panel.score = g_vars.score;
		const int score_digits = _score_8_digits ? 8 : 7;
		const int offset = _score_8_digits ? 0x1CF0 : 0x1CF1;
		int score = g_vars.score * 10;
		for (int i = 0; i < score_digits; ++i) {
			const int digit = score % 10;
			score /= 10;
			video_draw_panel_number(offset + (score_digits - 1 - i) * 16 / 8, digit);
		}
	}
	if (g_vars.player_energy != g_vars.panel.energy || _redraw_panel) {
		g_vars.panel.energy = g_vars.player_energy;
		for (int i = 0; i < g_vars.player_energy; ++i) {
			video_draw_panel_number(0x1D01 + i * 16 / 8, 10);
		}
		if (g_vars.player_energy > 3) {
			video_draw_panel_number(0x1D01 + 3 * 16 / 8, 11);
		}
	}
	uint8_t mask;
	if (g_vars.player_bonus_letters_blinking_counter != 0) {
		g_vars.panel.bonus_letters_mask = 0xFF;
		mask = 0x1F;
		if ((g_vars.level_draw_counter & 1) == 0) {
			mask = 0;
		}
	} else {
		mask = g_vars.player_bonus_letters_mask;
		if (g_vars.player_bonus_letters_mask != g_vars.panel.bonus_letters_mask) {
			g_vars.panel.bonus_letters_mask = g_vars.player_bonus_letters_mask;
		}
	}
	static const uint16_t panel_bonus_letters_pos_tbl[] = { 0x1C91, 0x1BF2, 0x1CE3, 0x1C6C, 0x1C1D };
	for (int i = 0; i < 5; ++i) {
		if ((mask & (1 << i)) != 0) {
			video_draw_panel_number(panel_bonus_letters_pos_tbl[i], 12 + i);
		}
	}
}

static void level_update_panel() {
	const int count = (g_vars.score / 25000) - g_vars.score_extra_life;
	if (count != 0) {
		g_vars.score_extra_life += count;
		g_vars.player_lifes += count;
		level_add_object75_score(&g_vars.objects_tbl[1], 227);
	}
	level_draw_panel();
}

static void level_update_light_palette() {
	if ((g_vars.light.palette_flag1 | g_vars.light.palette_flag2) != 0) {
		++g_vars.light.palette_counter;
		uint8_t palette[16 * 3];
		const uint8_t *src_pal = palettes_tbl[g_vars.level_num];
		const uint8_t *dst_pal = light_palette_data;
		bool changed = false;
		if (g_vars.light.palette_flag2 != 0) {
			SWAP(src_pal, dst_pal);
		}
		for (int i = 0; i < 16 * 3; ++i) {
			int diff = src_pal[i] - dst_pal[i];
			int step = g_vars.light.palette_counter;
			if (diff < 0) {
				diff = -diff;
				step = -step;
			}
			if (diff <= g_vars.light.palette_counter) {
				palette[i] = dst_pal[i];
			} else {
				palette[i] = src_pal[i] - step;
				changed = true;
			}
		}
		if (!changed) {
			g_vars.light.palette_flag1 = 0;
			g_vars.light.palette_flag2 = 0;
		}
		g_sys.set_screen_palette(palette, 0, 16, 6);
	}
}

static void level_sync() {
	update_input();
	g_sys.update_screen(g_res.vga, 1);
	g_sys.render_clear_sprites();
	const int diff = (g_vars.timestamp + (1000 / 30)) - g_sys.get_timestamp();
	g_sys.sleep(MAX(diff, 10));
	g_vars.timestamp = g_sys.get_timestamp();
}

static void level_draw_objects() {
	++g_vars.level_draw_counter;
	for (int i = OBJECTS_COUNT - 1; i >= 0; --i) {
		struct object_t *obj = &g_vars.objects_tbl[i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		if (obj->hit_counter != 0) {
			--obj->hit_counter;
		}
		if (obj->hit_counter != 0 && (g_vars.level_draw_counter & 3) != 0) {
			continue;
		}
		const int spr_num = obj->spr_num & 0x1FFF;
		const bool spr_hflip = (obj->spr_num & 0x8000) != 0;
		int spr_y_pos = obj->y_pos - ((g_vars.tilemap.y << 4) + g_vars.tilemap.scroll_dy);
		int spr_h = spr_size_tbl[2 * spr_num + 1];
		spr_y_pos -= spr_h;
		if (spr_y_pos > TILEMAP_SCREEN_H || spr_y_pos + spr_h < 0) {
			continue;
		}
		int spr_x_pos = obj->x_pos - ((g_vars.tilemap.x << 4) + g_vars.tilemap.scroll_dx);
		int spr_w = spr_offs_tbl[2 * spr_num];
		if (spr_hflip) {
			spr_w = spr_size_tbl[2 * spr_num] - spr_w;
		}
		spr_x_pos -= spr_w;
		if (spr_x_pos > TILEMAP_SCREEN_W || spr_x_pos + spr_w < 0) {
			continue;
		}
		video_draw_sprite(spr_num, spr_x_pos, spr_y_pos, spr_hflip);
		obj->spr_num |= 0x2000;
		obj->spr_num &= ~0x4000;
	}
}

static void level_draw_orbs() {
	for (int i = 0; i < 20; ++i) {
		struct orb_t *p = &g_vars.orb_tbl[i];
		if (p->x_pos == 0xFFFF) {
			continue;
		}

		const int rx = ((int8_t)cos_tbl[p->index_tbl]) >> 2;
		p->x_pos += (rx * p->radius) >> 4;

		const int ry = ((int8_t)sin_tbl[p->index_tbl]) >> 2;
		p->y_pos += (ry * p->radius) >> 4;

		const int y_pos = p->y_pos - g_vars.tilemap.scroll_dy - (g_vars.tilemap.y << 4);
		if (y_pos >= 0 && y_pos < TILEMAP_SCREEN_H) {
			const int x_pos = p->x_pos - g_vars.tilemap.scroll_dx - (g_vars.tilemap.x << 4);
			if (x_pos >= 0 && x_pos < TILEMAP_SCREEN_W) {
				video_put_pixel(x_pos, y_pos, 15);
			}
		}
		p->x_pos = 0xFFFF;
	}
}

static void level_draw_flies() {
	for (int i = 0; i < 20; ++i) {
		struct fly_t *p = &g_vars.fly_tbl[i];
		if (p->x_pos == _undefined) {
			continue;
		}
		--p->unk6;
		if (p->unk6 & 0x80) {
			p->unk6 = (random_get_number2() & 7) + 3;
		}
		int x_vel = random_get_number() & 15;
		if (g_vars.objects_tbl[1].x_pos < (p->x_pos >> 3)) {
			x_vel = -x_vel;
		}
		if (p->unk7 & 7) {
			x_vel = -x_vel;
		}
		int y_vel = random_get_number() & 15;
		if (g_vars.objects_tbl[1].y_pos < (p->y_pos >> 3)) {
			y_vel = -y_vel;
		}
		if (p->unk7 & 7) {
			y_vel = -y_vel;
		}
		p->x_delta = random_get_number2() & 31;
		if (x_vel < 0) {
			p->x_delta = -p->x_delta;
		}
		if (x_vel == 0) {
			p->y_delta = 2;
		} else {
			p->y_delta = MIN((abs(y_vel) << 3) / abs(x_vel), 32);
		}
		if (y_vel < 0) {
			p->y_delta = -p->y_delta;
		}
		if (p->unk7 & 7) {
			--p->unk7;
		}
		p->x_pos += p->x_delta;
		p->y_pos += p->y_delta;
		if (p->unk6) {
		} else {
		}
		const int x_pos = (p->x_pos >> 3) - ((g_vars.tilemap.x << 4) + g_vars.tilemap.scroll_dx);
		if (x_pos < 0 || x_pos >= TILEMAP_SCREEN_W) {
			continue;
		}
		const int y_pos = (p->y_pos >> 3) - ((g_vars.tilemap.y << 4) + g_vars.tilemap.scroll_dy);
		if (y_pos < 0 || y_pos >= TILEMAP_SCREEN_H) {
			continue;
		}
		video_put_pixel(x_pos, y_pos, 15);
	}
}

static void level_draw_snow() {
	++g_vars.snow.counter;
	if ((g_vars.level_draw_counter & 3) == 0) {
		const uint16_t *p = g_vars.snow.pattern;
		if (p[0] != 0xFFFF) {
			if (p[0] < g_vars.snow.counter) {
				g_vars.snow.value += p[1];
				if (p[3] < g_vars.snow.counter) {
					g_vars.snow.counter += 3;
				}
			}
			g_vars.snow.value = MIN(g_vars.snow.value, p[2]);
		}
	}
	if (g_vars.snow.value == 0) {
		return;
	}
	const int half = g_vars.snow.value >> 1;
	uint16_t *p = g_vars.snow.random_tbl;
	for (int i = 0; i < g_vars.snow.value; ++i) {
		p[i] += 79;
		uint8_t al = p[i];
		uint8_t cl = ((al << 1) | (al >> 7)) & 3;
		uint16_t pos = (p[i] - g_vars.tilemap.x) & 0x1FFF;
		if (pos >= 7000) {
			pos -= 7000;
			p[i] = pos;
		}
		const int y_pos = (pos / 40);
		const int x_pos = (pos % 40) * 8 + cl * 2;
		video_put_pixel(x_pos, y_pos, 15);
		if ((g_vars.snow.value - i) > half) {
			video_put_pixel(x_pos, y_pos + 1, 15);
		}
	}
	const uint8_t num = random_get_number();
	const uint8_t hi = random_get_number();
	const uint8_t lo = random_get_number();
	g_vars.snow.random_tbl[num] = (hi << 8) | lo;
}

static void level_update_player_bonuses() {
	if (g_vars.player_bonus_letters_mask == 0x1F) { /* found the 5 bonuses */
		g_vars.current_bonus.x_pos = g_vars.objects_tbl[1].x_pos;
		g_vars.current_bonus.y_pos = g_vars.objects_tbl[1].y_pos - 112;
		g_vars.current_bonus.spr_num = 110;
		level_add_object23_bonus(0, 0, 1);
		g_vars.player_bonus_letters_mask = 0;
		g_vars.player_bonus_letters_blinking_counter = 44; /* blink the letters in the panel */
	} else if ((g_vars.player_utensils_mask & 0x38) == 0x38) {
		g_vars.player_utensils_mask &= ~0x38;
		g_vars.player_hit_monster_counter = 660;
	}
}

static void level_shake_screen() {
	if (g_vars.shake_screen_counter < 1) {
		return;
	} else if (g_vars.shake_screen_counter == 1) {
		g_vars.shake_screen_voffset = 0;
	} else if ((g_vars.level_draw_counter & 1) == 0) {
		g_vars.shake_screen_voffset = 0;
	} else {
		if (g_vars.objects_tbl[1].data.p.current_anim_num != 5 && g_vars.objects_tbl[1].data.p.current_anim_num != 32) {
			g_vars.objects_tbl[1].y_pos -= 12;
		}
		++g_vars.shake_screen_counter;
		g_vars.shake_screen_voffset = g_vars.shake_screen_counter;
	}
}

static void level_player_death_animation() {
	play_sound(7);
	g_vars.objects_tbl[1].hit_counter = 0;
	g_vars.objects_tbl[1].spr_num = 33;
	g_vars.objects_tbl[1].data.p.y_velocity = 15;
	g_vars.objects_tbl[1].x_velocity = (g_vars.objects_tbl[1].x_pos < ((g_vars.tilemap.x + 10) << 4)) ? 5 : -5;
	for (int i = 0; i < 60; ++i) {
		g_vars.objects_tbl[0].spr_num = 0xFFFF;
		level_update_objects_hit_animation();
		level_update_objects_monsters();
		level_update_objects_club_projectiles();
		level_update_objects_bonuses();
		level_update_objects_bonus_scores();
		level_update_tilemap();
		level_draw_tilemap();
		level_draw_panel();
		level_draw_orbs();
		level_draw_objects();
		level_draw_front_tiles();
		level_draw_flies();
		level_draw_snow();
		level_sync();
		g_vars.objects_tbl[1].x_pos += g_vars.objects_tbl[1].x_velocity;
		const int y_velocity = MAX(g_vars.objects_tbl[1].data.p.y_velocity - 1, -16);
		g_vars.objects_tbl[1].y_pos -= y_velocity;
		g_vars.objects_tbl[1].data.p.y_velocity = y_velocity;
	}
	video_transition_close();
}

static void level_completed_bonuses_animation_draw_score() {
	const int score_digits = _score_8_digits ? 8 : 7;
	video_draw_string2(0x230, "SCORE");
	int score = g_vars.score * 10;
	for (int i = 0; i < score_digits; ++i) {
		const int digit = score % 10;
		score /= 10;
		video_draw_number(0x23C + (score_digits - 1 - i) * 16 / 8, digit);
	}
	if (g_res.dos_demo) {
		return;
	}
	video_draw_string2(0x410, "LEVEL COMPLETED");
	int percentage = 100;
	const int total = g_vars.level_complete_secrets_count + g_vars.level_complete_bonuses_count;
	if (total != 0) {
		const int current = g_vars.level_current_secrets_count + g_vars.level_current_bonuses_count;
		percentage = (current * 100) / total;
	}
	for (int i = 0; i < 3; ++i) {
		const int digit = percentage % 10;
		percentage /= 10;
		video_draw_number(0x430 + (2 - i) * 16 / 8, digit);
		if (percentage == 0) {
			break;
		}
	}
	video_draw_character_spr(0x436, 0x1A);
}

static void level_completed_bonuses_animation_fixup_object4_spr_num() {
	if (g_vars.level_draw_counter & 3) {
		return;
	}
	int spr_num = (g_vars.objects_tbl[4].spr_num & 0x1FFF) + 1;
	if (spr_num >= 110) {
		spr_num = 104;
	}
	g_vars.objects_tbl[4].spr_num = spr_num;
}

static int level_completed_bonuses_animation_helper() {
	int bp = 0;
	do {
		level_update_object_anim(g_vars.objects_tbl[1].data.p.anim);
		video_clear();
		level_completed_bonuses_animation_fixup_object4_spr_num();
		level_draw_objects();
		level_completed_bonuses_animation_draw_score();
		level_sync();
		for (int i = 0; i < 20; ++i) {
			struct object_t *obj = &g_vars.objects_tbl[55 + i];
			if (obj->spr_num == 0xFFFF) {
				continue;
			}
			int y_velocity = obj->data.t.y_velocity;
			if (y_velocity < 128) {
				y_velocity += 8;
				obj->data.t.y_velocity = y_velocity;
			}
			obj->y_pos += y_velocity >> 4;
			++bp;
			if (obj->y_pos >= 145) {
				const int spr_num = obj->spr_num & 0x1FFF;
				g_vars.score += score_tbl[score_spr_lut[spr_num - 110]];
				obj->spr_num = 0xFFFF;
				play_sound(8);
			}
		}
	} while ((g_vars.level_draw_counter & 7) != 0);
	return bp;
}

static void level_completed_bonuses_animation() {
	if (g_vars.light.state != 0) {
		g_vars.light.palette_flag1 = 0;
		g_vars.light.palette_flag2 = 1;
		g_vars.light.palette_counter = 0;
	}
	play_music(15);
	g_vars.objects_tbl[1].y_pos -= g_vars.tilemap.y << 4;
	g_vars.objects_tbl[1].x_pos -= g_vars.tilemap.x << 4;
	g_vars.tilemap.y = 0;
	g_vars.tilemap.x = 0;
	g_vars.objects_tbl[1].data.p.hdir = 0;
	g_vars.objects_tbl[1].x_velocity = 0;
	level_completed_bonuses_animation_draw_score();
	g_vars.player_flying_flag = 0;
	g_vars.objects_tbl[1].data.p.anim = object_anim_tbl[1];
	while (1) {
		level_update_object_anim(g_vars.objects_tbl[1].data.p.anim);

		bool flag = false;

		int x_pos = 60;
		int x_offs = 2;
		const int dx = g_vars.objects_tbl[1].x_pos - x_pos;
		if (dx < 0) {
			x_offs = -x_offs;
		}
		if (dx >= 2) {
			x_pos = g_vars.objects_tbl[1].x_pos - x_offs;
			flag = true;
		}
		g_vars.objects_tbl[1].x_pos = x_pos;

		int y_pos = 175;
		int y_offs = 2;
		const int dy = g_vars.objects_tbl[1].y_pos - y_pos;
		if (dy < 0) {
			y_offs = -y_offs;
		}
		if (dy >= 2) {
			y_pos = g_vars.objects_tbl[1].y_pos - y_offs;
			flag = true;
		}
		g_vars.objects_tbl[1].y_pos = y_pos;

		video_clear();
		level_completed_bonuses_animation_draw_score();
		level_draw_objects();
		level_sync();
		if (g_sys.input.quit) {
			return;
		}
		if (!flag) {
			break;
		}
	}
	g_vars.objects_tbl[2].x_pos = 360;
	g_vars.objects_tbl[2].y_pos = 175;
	g_vars.objects_tbl[2].spr_num = 100;
	g_vars.objects_tbl[3].x_pos = 360;
	g_vars.objects_tbl[3].y_pos = 148;
	g_vars.objects_tbl[3].spr_num = 98;
	g_vars.objects_tbl[4].x_pos = 360;
	g_vars.objects_tbl[4].y_pos = 155;
	g_vars.objects_tbl[4].spr_num = 104;
	do {
		level_update_object_anim(g_vars.objects_tbl[1].data.p.anim);
		video_clear();
		level_completed_bonuses_animation_fixup_object4_spr_num();
		level_completed_bonuses_animation_draw_score();
		level_draw_objects();
		level_sync();
		if (g_sys.input.quit) {
			return;
		}
		g_vars.objects_tbl[2].x_pos -= 3;
		g_vars.objects_tbl[3].x_pos -= 3;
		g_vars.objects_tbl[4].x_pos -= 3;
	} while (g_vars.objects_tbl[2].x_pos > 155);
	if (g_vars.level_items_total_count != 0) {
		g_vars.objects_tbl[1].data.p.anim = object_anim_tbl[18];
		g_vars.objects_tbl[1].x_velocity = 64;
		g_vars.objects_tbl[1].x_friction = 2;
		while (1) {
			level_update_object_anim(g_vars.objects_tbl[1].data.p.anim);
			video_clear();
			level_completed_bonuses_animation_fixup_object4_spr_num();
			level_completed_bonuses_animation_draw_score();
			level_draw_objects();
			level_sync();
			if (g_sys.input.quit) {
				return;
			}
			level_update_player_x_velocity();
			const int x_velocity = g_vars.objects_tbl[1].x_velocity >> 4;
			g_vars.objects_tbl[1].x_pos += x_velocity;
			if (x_velocity == 0) {
				break;
			}
		}
		g_vars.objects_tbl[1].data.p.anim = object_anim_tbl[17];
		uint16_t di = 0;
		uint16_t bp = level_completed_bonuses_animation_helper();
		uint8_t al = 0;
		while (!g_sys.input.quit) {
			if (di >= 113) {
				di = 0;
			}
			if (g_vars.level_items_count_tbl[di] != 0) {
				for (int i = 0; i < 20; ++i) {
					struct object_t *obj = &g_vars.objects_tbl[55 + i];
					if (obj->spr_num == 0xFFFF) {
						obj->spr_num = 110 + di;
						obj->x_pos = 155;
						obj->y_pos = 0;
						obj->data.t.y_velocity = 0;
						--g_vars.level_items_count_tbl[di];
						++di;
						break;
					}
				}
				bp = level_completed_bonuses_animation_helper();
				al = 0;
				continue;
			}
			++al;
			if (al < 113) {
				++di;
			} else {
				if (bp == 0) {
					break;
				}
				bp = level_completed_bonuses_animation_helper();
				al = 0;
			}
		}
	}
	g_vars.objects_tbl[1].data.p.anim = object_anim_tbl[1];
	do {
		level_update_object_anim(g_vars.objects_tbl[1].data.p.anim);
		video_clear();
		level_completed_bonuses_animation_fixup_object4_spr_num();
		level_completed_bonuses_animation_draw_score();
		level_draw_objects();
		level_sync();
		if (g_sys.input.quit) {
			return;
		}
		g_vars.objects_tbl[2].x_pos -= 2;
		g_vars.objects_tbl[3].x_pos -= 2;
		g_vars.objects_tbl[4].x_pos -= 2;
		if (g_vars.objects_tbl[4].x_pos < 0) {
			g_vars.objects_tbl[1].x_pos += 3;
		} else {
			g_vars.objects_tbl[1].x_pos += 2;
		}
	} while (g_vars.objects_tbl[2].x_pos > -52);
}

static void update_object_demo_animation(struct object_t *obj) {
	const uint8_t *p = obj->data.m.anim;
	int16_t num = READ_LE_UINT16(p);
	if (num < 0) {
		p += num;
		num = READ_LE_UINT16(p);
	}
	int spr_num = (num & 0x1FFF) + g_res.spr_monsters_offset;
	if (obj->data.m.x_velocity < 0) {
		num |= 0x8000;
	}
	obj->spr_num = spr_num;
	obj->data.m.anim = p + 2;
}

void do_demo_animation() {
	g_vars.objects_tbl[1].x_pos = 0;
	g_vars.objects_tbl[1].y_pos = 169;
	g_vars.objects_tbl[1].data.p.hdir = 0;
	g_vars.objects_tbl[1].x_velocity = 0;
	static const uint8_t data[] = {
		0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 2, 0, 3, 0, 3, 0,
		4, 0, 4, 0, 5, 0, 5, 0, 0xE8, 0xFF
	};
	g_vars.objects_tbl[1].data.p.anim = data;
	g_vars.objects_tbl[2].x_pos = 48;
	g_vars.objects_tbl[2].y_pos = 169;
	g_vars.objects_tbl[2].x_velocity = 0;
	g_vars.objects_tbl[2].data.m.anim = &monster_anim_tbl[0x2EA];
	g_vars.player_flying_flag = 0;
	int counter = 0;
	do {
		++counter;
		level_update_object_anim(g_vars.objects_tbl[1].data.p.anim);
		update_object_demo_animation(&g_vars.objects_tbl[2]);
		level_draw_objects();
		level_sync();
		int dx = 5;
		g_vars.objects_tbl[1].x_pos += dx;
		if ((counter & 3) == 0) {
			--dx;
		}
		g_vars.objects_tbl[2].x_pos += dx;
	} while ((g_vars.objects_tbl[1].spr_num & 0x2000) != 0);
	g_vars.objects_tbl[1].data.p.hdir = -1;
	g_vars.objects_tbl[2].data.m.x_velocity = -1;
	g_vars.objects_tbl[2].data.m.y_velocity = 0;
	for (int i = 1; i < 3; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[2 + i];
		*obj = g_vars.objects_tbl[2];
	}
	for (int i = 0; i < 3; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[2 + i];
		obj->x_pos = g_vars.objects_tbl[1].x_pos + i * 30;
		obj->y_pos = g_vars.objects_tbl[1].y_pos - i * 3;
	}
	do {
		level_update_object_anim(g_vars.objects_tbl[1].data.p.anim);
		for (int i = 0; i < 3; ++i) {
			struct object_t *obj = &g_vars.objects_tbl[2 + i];
			obj->x_pos -= 5;
			update_object_demo_animation(obj);
			if (obj->y_pos > 169) {
				obj->data.m.y_velocity = -obj->data.m.y_velocity;
			}
			obj->data.m.y_velocity += 8;
			obj->y_pos += (obj->data.m.y_velocity >> 4);
		}
		level_draw_objects();
		level_sync();
		g_vars.objects_tbl[1].x_pos -= 7;
	} while (g_vars.objects_tbl[1].x_pos >= 0);
}

void do_level() {
	static const uint8_t music_tbl[] = { 9, 9, 0, 0, 0, 13, 4, 4, 10, 13, 16, 16, 16, 9, 14, 4 };
	play_music(music_tbl[g_vars.level_num]);
	load_level_data(g_vars.level_num);
	set_level_palette();
	video_load_sprites();
	video_load_front_tiles();
	video_clear();
	level_reset();
	level_reset_objects();
	level_init_tilemap();
	level_clear_panel();
	level_draw_panel();
	level_sync();
	g_vars.monster.type10_dist = 0;
	g_vars.monster.type0_hdir = 0;
	random_reset();
	while (!g_sys.input.quit) {
		level_update_objects_hit_animation();
		level_update_objects_axe();
		level_update_objects_monsters();
		level_update_objects_club_projectiles();
		level_update_objects_bonuses();
		level_update_objects_bonus_scores();
		level_update_objects_decors();
		level_update_player();
		level_update_player_collision();
		level_update_objects_items();
		level_update_gates();
		level_update_columns();
		g_vars.tilemap_adjust_player_pos_flag = false;
		level_update_scrolling();
		level_update_tilemap();
		level_draw_tilemap();
		level_draw_panel();
		level_draw_orbs();
		level_draw_objects();
		level_draw_front_tiles();
		level_draw_flies();
		level_draw_snow();
		if (!level_update_objects_anim()) {
			break;
		}
		level_update_panel();
		level_sync();
		level_update_light_palette();
		level_update_player_bonuses();
		level_shake_screen();
	}
}

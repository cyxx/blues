
/* bosses logic : gorilla, tree, minotaur */

#include "game.h"
#include "resource.h"
#include "util.h"

static void level_update_objects_boss_energy(int count) {
	if (count > 8) {
		count = 8;
	}
	if (count != 0) {
		play_music(13);
		for (int i = 0; i < count; ++i) {
			struct object_t *obj = &g_vars.objects_tbl[108 + i];
			obj->spr_num = 0x135;
			obj->x_pos = 8 + i * 5;
			obj->y_pos = 170;
		}
	}
	for (int i = count; i < 8; ++i) {
		g_vars.objects_tbl[108 + i].spr_num = 0xFFFF;
	}
}

static void level_update_objects_boss_hit_player() {
	--g_vars.bonus_energy_counter;
	if (g_vars.bonus_energy_counter < 0) {
		g_vars.bonus_energy_counter = 5;
		if ((g_options.cheats & CHEATS_UNLIMITED_ENERGY) == 0) {
			--g_vars.player_energy;
			if (g_vars.player_energy < 0) {
				level_player_die();
			}
		}
	}
	g_vars.current_bonus.x_pos = g_vars.objects_tbl[1].x_pos;
	g_vars.current_bonus.y_pos = g_vars.objects_tbl[1].y_pos - 48;
	g_vars.current_bonus.spr_num = 0x2046; /* bones */
	const int x_vel = (g_vars.bonus_energy_counter & 1) != 0 ? -48 : 48;
	const int y_vel = -128;
	level_add_object23_bonus(x_vel, y_vel, 1);
}

static void level_update_boss_gorilla_collide_proj(struct object_t *obj_player, struct object_t *obj) {
	if (obj->spr_num == 0xFFFF || (obj->spr_num & 0x2000) == 0) {
		return;
	}
	if (!level_objects_collide(obj_player, obj)) {
		return;
	}
	const int count = 4 - g_res.level.boss_speed;
	if (g_vars.boss.change_counter < count) {
		g_vars.boss.change_counter = 0;
	} else {
		g_vars.boss.change_counter -= count;
	}
	if (g_options.cheats & CHEATS_NO_HIT) {
		return;
	}
	level_update_objects_boss_hit_player();
	obj_player->hit_counter = 44;
	g_vars.player_anim_0x40_flag = 0;
	obj_player->data.p.y_velocity = -128;
	obj_player->x_friction = 3;
	obj_player->x_velocity = (g_vars.objects_tbl[1].x_pos >= g_res.level.boss_x_pos) ? 128 : -128;
	g_vars.player_gravity_flag = 0;
}

static void level_update_boss_gorilla_hit_player() {
	if (g_vars.restart_level_flag != 0) {
		return;
	}
	level_update_boss_gorilla_collide_proj(&g_vars.objects_tbl[1], g_vars.boss.obj2);
	level_update_boss_gorilla_collide_proj(&g_vars.objects_tbl[1], g_vars.boss.obj1);
	if (level_objects_collide(&g_vars.objects_tbl[1], g_vars.boss.obj3)) {
		if (g_vars.player_jump_monster_flag != 0) {
			g_vars.objects_tbl[1].data.p.y_velocity = (g_vars.input.key_up != 0) ? -128 : -64;
		}
	}
}

static void level_update_boss_gorilla_tick() {
	if (g_vars.boss.change_counter < UCHAR_MAX) {
		++g_vars.boss.change_counter;
	}
}

static uint16_t level_update_boss_gorilla_find_pos(uint16_t num1, uint16_t num2) {
	const uint16_t *p = boss_gorilla_spr_tbl;
	const uint16_t *end = &boss_gorilla_spr_tbl[138];
	while (p + 3 <= end) {
		if (p[0] == num1 && p[1] == num2) {
			return p[2]; /* dy,dx packed as a uint16_t */
		}
		p += 3;
	}
	print_warning("boss_gorilla spr range (%d,%d) not found", num1, num2);
	return 0xFFFF;
}

static void level_update_boss_gorilla_init_objects(const uint16_t *p) {
	g_vars.boss.parts[0].spr_num = *p++;
	g_vars.boss.parts[1].spr_num = *p++;
	uint16_t d;
	int8_t dx, dy;

	d = level_update_boss_gorilla_find_pos(g_vars.boss.parts[0].spr_num, g_vars.boss.parts[1].spr_num);
	dx = d & 255;
	if (g_vars.boss.hdir) {
		dx = -dx;
	}
	g_vars.boss.parts[1].x_pos = dx + g_vars.boss.parts[0].x_pos;
	dy = d >> 8;
	g_vars.boss.parts[1].y_pos = dy + g_vars.boss.parts[0].y_pos;
	for (int i = 2; i <= 4; ++i) {
		g_vars.boss.parts[i].spr_num = *p++;
		d = level_update_boss_gorilla_find_pos(g_vars.boss.parts[1].spr_num, g_vars.boss.parts[i].spr_num);
		dx = d & 255;
		if (g_vars.boss.hdir) {
			dx = -dx;
		}
		g_vars.boss.parts[i].x_pos = dx + g_vars.boss.parts[1].x_pos;
		dy = d >> 8;
		g_vars.boss.parts[i].y_pos = dy + g_vars.boss.parts[1].y_pos;
	}

	if (g_vars.boss.anim_num & 0x40) {
		g_vars.boss.parts[1].y_pos += 2;
		++g_vars.boss.parts[2].y_pos;
		++g_vars.boss.parts[4].y_pos;
		++g_vars.boss.parts[3].y_pos;
	}
	if (g_vars.boss.hdir) {
		g_vars.boss.parts[0].spr_num |= 0x8000;
		g_vars.boss.parts[1].spr_num |= 0x8000;
		g_vars.boss.parts[2].spr_num |= 0x8000;
		g_vars.boss.parts[3].spr_num |= 0x8000;
		g_vars.boss.parts[4].spr_num |= 0x8000;
	}
	for (int i = 0; i < 5; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[103 + i];
		const uint16_t addr = *p++;
		if (addr == 0xA609) {
			g_vars.boss.obj1 = obj;
		}
		if (addr == 0xA60F) {
			g_vars.boss.obj2 = obj;
		}
		if (addr == 0xA603) {
			g_vars.boss.obj3 = obj;
		}
		assert(addr == 0xA5F7 || addr == 0xA5FD || addr == 0xA603 || addr == 0xA609 || addr == 0xA60F);
		const int num = (addr - 0xA5F7) / 6;
		obj->x_pos = g_vars.boss.parts[num].x_pos;
		obj->y_pos = g_vars.boss.parts[num].y_pos;
		obj->spr_num = g_vars.boss.parts[num].spr_num;
	}
}

static bool level_update_boss_gorilla_collides_obj3(struct object_t *obj) {
	if (obj->spr_num == 0xFFFF) {
		return false;
	}
	const int spr_num = g_vars.boss.obj1->spr_num & 0x1FFF;
	if ((spr_num == 0x196 || spr_num == 0x195) && level_objects_collide(obj, g_vars.boss.obj1)) {
		obj->spr_num = 0xFFFF;
		return false;
	}
	return level_objects_collide(obj, g_vars.boss.obj3);
}

static void level_update_boss_gorilla_helper2() {
	for (int i = 0; i < 6; ++i) {
		if (i == 1) {
			continue;
		}
		struct object_t *obj = &g_vars.objects_tbl[i];
		if (!level_update_boss_gorilla_collides_obj3(obj)) {
			continue;
		}
		if (abs(g_vars.level_draw_counter - g_vars.boss.draw_counter) <= 22) {
			break;
		}
		g_vars.boss.draw_counter = g_vars.level_draw_counter;
		for (int j = 103; j <= 107; ++j) {
			g_vars.objects_tbl[j].spr_num |= 0x4000;
		}
		g_vars.player_flying_flag = 0;
		level_update_boss_gorilla_tick();
		g_res.level.boss_energy -= g_vars.player_club_power;
		if (g_res.level.boss_energy < 0) {
			g_res.level.boss_state = 6;
			g_vars.boss.y_velocity = -240;
			break;
		}
		if (g_vars.player_club_power > 20) {
			g_vars.boss.x_velocity = (g_res.level.boss_x_pos >= g_vars.objects_tbl[1].x_pos) ? 48 : -48;
			g_vars.boss.y_velocity = (g_vars.boss.y_velocity > 0) ? -128 : -64;
			g_res.level.boss_state = 5;
		}
		g_vars.boss.state_counter = 0;
		obj->spr_num = 0xFFFF;
		break;
	}
}

static void level_update_boss_gorilla() {
	if (g_vars.objects_tbl[103].spr_num == 0xFFFF || g_res.level.boss_state == 6 || (g_vars.objects_tbl[103].spr_num & 0x2000) == 0) {
		level_update_objects_boss_energy(0);
	} else {
		level_update_objects_boss_energy(g_res.level.boss_energy >> 3);
	}
	const int x = (g_vars.boss.x_velocity >> 4) + g_res.level.boss_x_pos;
	if (x >= 0) {
		if (g_res.level.boss_xmin <= x && g_res.level.boss_xmax >= x) {
			g_res.level.boss_x_pos = x;
		}
	}
	g_res.level.boss_y_pos += (g_vars.boss.y_velocity >> 4);
	if (g_vars.boss.y_velocity >= 0) {
		const uint16_t pos = ((g_res.level.boss_y_pos >> 4) << 8) | (g_res.level.boss_x_pos >> 4);
		const uint8_t tile_num = level_get_tile(pos);
		if (g_res.level.tile_attributes1[tile_num] != 0) {
			g_res.level.boss_y_pos &= ~15;
			if (g_vars.boss.y_velocity != 0) {
				g_vars.shake_screen_counter = 7;
			}
			g_vars.boss.y_velocity = 0;
			g_vars.boss.x_velocity = 0;
		} else {
			if (g_vars.boss.y_velocity < 224) {
				g_vars.boss.y_velocity += 16;
			}
		}
	} else {
		if (g_vars.boss.y_velocity < 224) {
			g_vars.boss.y_velocity += 16;
		}
	}
	g_vars.boss.hdir = (g_res.level.boss_x_pos < g_vars.objects_tbl[1].x_pos);
	g_vars.boss.x_dist = abs(g_vars.objects_tbl[1].x_pos - g_res.level.boss_x_pos);
	if (g_vars.boss.x_dist > 400) {
		return;
	}
	if (abs(g_vars.objects_tbl[1].y_pos - g_res.level.boss_y_pos) > 250) {
		return;
	}
	if (g_res.level.boss_state == 0) { /* waiting player to be near */
		if (g_vars.boss.x_dist >= 250) {
			g_vars.boss.change_counter = 0;
			static const uint8_t data_st0[] = {
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0xF4
			};
			g_vars.boss.next_anim = data_st0;
		} else {
			g_vars.boss.state_counter = 0;
			g_res.level.boss_state = 1;
		}
	} else if (g_res.level.boss_state == 1) {
		if (g_vars.boss.x_dist >= 250) {
			g_res.level.boss_state = 0;
		} else {
			++g_vars.boss.state_counter;
			if (g_res.level.boss_energy < 60) {
				g_res.level.boss_state = 2;
			} else {
				static const uint8_t data_st1[] = {
					0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0xFA
				};
				g_vars.boss.next_anim = data_st1;
				if (g_vars.boss.state_counter >= 110) {
					g_res.level.boss_state = 2;
					level_update_boss_gorilla_tick();
					g_vars.boss.state_counter = 0;
				} else if (g_vars.player_anim_0x40_flag != 0 && (g_vars.level_draw_counter & 7) == 0) {
					level_update_boss_gorilla_tick();
				} else if (g_vars.boss.change_counter >= 10) {
					g_res.level.boss_state = 3;
					g_vars.boss.state_counter = 0;
				} else if (g_vars.boss.change_counter > 3) {
					g_res.level.boss_state = 2;
					g_vars.boss.state_counter = 0;
				}
			}
		}
	} else if (g_res.level.boss_state == 2) {
		++g_vars.boss.state_counter;
		if (g_vars.boss.state_counter != 44) {
			if (g_vars.boss.state_counter > 44) {
				if (g_vars.boss.y_velocity > 0) {
					static const uint8_t data_st2a[] = {
						0x04, 0xFF
					};
					static const uint8_t data_st2b[] = {
						0x07, 0xFF
					};
					g_vars.boss.next_anim = (g_res.level.boss_energy >= 100) ? data_st2a : data_st2b;
				}
				if (g_vars.boss.x_dist <= 80) {
					g_vars.boss.state_counter = 0;
					g_res.level.boss_state = 3;
					g_vars.boss.change_counter = 11;
				} else if (g_vars.boss.state_counter == 88) {
					static const uint8_t data_st2[] = {
						0x03, 0xFF
					};
					g_vars.boss.next_anim = data_st2;
					int dx = g_res.level.boss_x_pos - g_vars.objects_tbl[1].x_pos;
					int x_vel = 48;
					int y_vel = abs(dx);
					if (y_vel > g_res.level.boss_speed * 80) {
						y_vel = -96;
					} else {
						y_vel = (y_vel / 14) << 4;
						x_vel = y_vel >> 1;
						y_vel = -y_vel;
					}
					g_vars.boss.y_velocity = y_vel;
					if (dx >= 0) {
						x_vel = -x_vel;
					}
					g_vars.boss.x_velocity = x_vel;
				} else if (g_vars.boss.state_counter > 88) {
					if (g_vars.boss.y_velocity == 0) {
						g_vars.boss.change_counter = 3;
						g_vars.boss.state_counter = 0;
						g_res.level.boss_state = 1;
					}
				}
			} else {
				level_update_boss_gorilla_hit_player();
				if (g_vars.boss.x_dist < 75) {
					g_res.level.boss_state = 3;
					g_vars.boss.state_counter = 0;
				} else {
					static const uint8_t data_st2[] = {
						0x10, 0x50, 0x10, 0x11, 0x51, 0x11, 0x10, 0x50, 0x10, 0x11,
						0x51, 0x11, 0x10, 0x50, 0x10, 0x11, 0x51, 0x11, 0x00, 0x00,
						0x40, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x40, 0x40,
						0x00, 0x00, 0x40, 0x40, 0xDE
					};
					g_vars.boss.next_anim = data_st2;
				}
			}
		} else {
			level_update_boss_gorilla_hit_player();
			g_vars.boss.y_velocity = -224;
			if (g_res.level.boss_energy < 40 && g_vars.boss.x_dist < 80) {
				g_res.level.boss_state = 4;
				g_vars.boss.state_counter = 0;
			}
			static const uint8_t data_st2[] = {
				0x03, 0xFF
			};
			g_vars.boss.next_anim = data_st2;
		}
	} else if (g_res.level.boss_state == 3) { /* attack player */
		level_update_boss_gorilla_hit_player();
		const int counter = (abs(g_res.level.boss_energy - 50) > 25) ? 66 : 154;
		if (g_vars.boss.state_counter > counter) {
			g_vars.boss.state_counter = 0;
			g_res.level.boss_state = 2;
		} else {
			if (g_vars.boss.y_velocity == 0) {
				if (g_vars.boss.x_dist < 100) {
					if (g_vars.boss.x_dist <= 25) {
						static const uint8_t data_st3[] = {
							0x0D, 0x0D, 0x0E, 0x0E, 0xFC
						};
						g_vars.boss.next_anim = data_st3;
					} else if (g_vars.boss.x_dist <= 35) {
						g_res.level.boss_state = 4;
						g_vars.boss.state_counter = 0;
					} else {
						if (g_vars.boss.change_counter == 0) {
							g_res.level.boss_state = 7;
							g_vars.boss.state_counter = 0;
						}
						static const uint8_t data_st3[] = {
							0x0D, 0x0D, 0x0C, 0x0C, 0xFC
						};
						g_vars.boss.next_anim = data_st3;
					}
				} else {
					g_vars.boss.y_velocity = -81;
					g_vars.boss.x_velocity = (g_vars.objects_tbl[1].x_pos > g_res.level.boss_x_pos) ? 80 : -80;
				}
			} else if (g_vars.boss.y_velocity < 0) {
				static const uint8_t data_st3[] = {
					0x03, 0xFF
				};
				g_vars.boss.next_anim = data_st3;
			} else {
				static const uint8_t data_st3[] = {
					0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0xFA
				};
				g_vars.boss.next_anim = data_st3;
			}
		}
	} else if (g_res.level.boss_state == 4) {
		level_update_boss_gorilla_hit_player();
		++g_vars.boss.state_counter;
		if (g_vars.boss.state_counter > 66) {
			g_res.level.boss_state = 3;
			g_vars.boss.state_counter = 0;
		} else {
			static const uint8_t data_st4[] = {
				0x12, 0x12, 0x52, 0x12, 0x0E, 0x0E, 0x4E, 0x0E, 0xF8
			};
			g_vars.boss.next_anim = data_st4;
			if (g_vars.boss.y_velocity == 0) {
				g_vars.shake_screen_counter = 4;
			}
		}
	} else if (g_res.level.boss_state == 5) { /* hit by player */
		++g_vars.boss.state_counter;
		if (g_vars.boss.state_counter > 19) {
			g_vars.boss.state_counter = 0;
			g_res.level.boss_state = 1;
		} else {
			static const uint8_t data_st5[] = {
				0x05, 0xFF
			};
			g_vars.boss.next_anim = data_st5;
		}
	} else if (g_res.level.boss_state == 6) { /* defeated */
		static const uint8_t data_st6[] = {
			0x05, 0x05, 0x0F, 0x0F, 0xFC
		};
		g_vars.boss.next_anim = data_st6;
		if (g_vars.boss.y_velocity >= 0) {
			g_res.level.boss_state = 0xFF;
			for (int i = 0; i < 5; ++i) {
				g_vars.objects_tbl[103 + i].spr_num = 0xFFFF;
			}
			g_vars.current_bonus.x_pos = g_vars.objects_tbl[103].x_pos;
			g_vars.current_bonus.y_pos = g_vars.objects_tbl[103].y_pos;
			level_add_bonuses_4x();
			g_vars.current_bonus.x_pos += 32;
			g_vars.current_bonus.y_pos += 32;
			level_add_bonuses_4x();
			g_vars.current_bonus.x_pos -= 64;
			level_add_bonuses_4x();
			g_vars.current_bonus.x_pos -= 32;
			g_vars.current_bonus.y_pos -= 64;
			level_add_bonuses_4x();
			g_vars.current_bonus.spr_num = 99; /* end of level lighter */
			level_add_object23_bonus(0, 0, 1);
			return;
		}
	} else if (g_res.level.boss_state == 7) {
		++g_vars.boss.state_counter;
		if (g_vars.boss.state_counter > 44) {
			g_res.level.boss_state = 1;
		} else {
			static const uint8_t data_st7[] = {
				0x03, 0xFF
			};
			g_vars.boss.next_anim = data_st7;
			if (g_vars.boss.y_velocity == 0) {
				g_vars.boss.y_velocity = -97;
				g_vars.boss.x_velocity = (g_vars.objects_tbl[1].x_pos > g_res.level.boss_x_pos) ? -48 : 48;
			} else if (g_vars.boss.y_velocity > 0) {
				static const uint8_t data_st7[] = {
					0x04, 0xFF
				};
				g_vars.boss.next_anim = data_st7;
			}
		}
	}
	if (g_vars.boss.prev_anim != g_vars.boss.next_anim) {
		g_vars.boss.prev_anim = g_vars.boss.next_anim;
		g_vars.boss.current_anim = g_vars.boss.next_anim;
	}
	const uint8_t *p = g_vars.boss.current_anim;
	while (1) {
		const uint8_t num = *p;
		if (num & 0x80) {
			p += (int8_t)num;
		} else {
			g_vars.boss.current_anim = p + 1;
			g_vars.boss.anim_num = num;
			g_vars.boss.parts[0].x_pos = g_res.level.boss_x_pos;
			g_vars.boss.parts[0].y_pos = g_res.level.boss_y_pos;
			const uint16_t *p = &boss_gorilla_data[(num & ~0x40) * 20 / sizeof(uint16_t)];
			level_update_boss_gorilla_init_objects(p);
			level_update_boss_gorilla_helper2();
			break;
		}
	}
}

static void level_update_boss_tree() {
	level_update_objects_boss_energy((2 - g_vars.boss_level5.state) << 1);
	struct object_t *obj_player = &g_vars.objects_tbl[1];
	for (int i = 0; i < 5; ++i) {
		struct boss_level5_leaf_t *leaf = &g_vars.boss_level5.leaf_tbl[i];
		struct object_t *obj = &g_vars.objects_tbl[98 + i];
		if (leaf->spr_num == 0xFFFF) {
			continue;
		}
		if (g_vars.restart_level_flag == 0 && level_objects_collide(obj_player, obj)) {
			obj_player->hit_counter = 44;
			g_vars.player_anim_0x40_flag = 0;
			obj_player->data.p.y_velocity = -128;
			obj_player->x_friction = 3;
			obj_player->x_velocity = -128;
			level_update_objects_boss_hit_player();
			leaf->spr_num = 0xFFFF;
			obj->spr_num = 0xFFFF;
		} else {
			leaf->y_pos += leaf->y_delta >> 4;
			leaf->x_delta += leaf->dir;
			int dx = leaf->x_delta;
			if (dx >= 32 || dx < -32) {
				leaf->dir = -leaf->dir;
			}
			uint16_t spr_num = leaf->spr_num;
			dx >>= 4;
			if (dx != 0) {
				spr_num += (dx > 0) ? 1 : 2;
			}
			leaf->x_pos += dx;
			obj->x_pos = leaf->x_pos;
			obj->y_pos = leaf->y_pos;
			if (obj->y_pos > 2008) {
				leaf->spr_num = 0xFFFF;
				obj->spr_num = 0xFFFF;
			} else {
				obj->spr_num = spr_num;
			}
		}
	}
	g_vars.objects_tbl[105].spr_num = 0xFFFF;
	g_vars.objects_tbl[104].spr_num = 0xFFFF;
	if (g_vars.boss_level5.state < 2) {
		static const uint16_t pos2_data[] = {
			0x3D5, 0x7C1, 0x1AA, 0x3FA, 0x7B6, 0x1AB, 0x3E8, 0x7A8, 0x1AC,
			0x3FF, 0x788, 0x1AD, 0x3D9, 0x79B, 0x1AE, 0x3FF, 0x7A8, 0x1AF,
			0x405, 0x7A9, 0x1B7, 0, 0, 0xFFFF
		};
		const uint16_t *p = &pos2_data[g_vars.boss_level5.state * 6];
		g_vars.objects_tbl[105].x_pos = p[0];
		g_vars.objects_tbl[105].y_pos = p[1];
		g_vars.objects_tbl[105].spr_num = p[2];
		g_vars.objects_tbl[104].x_pos = p[3];
		g_vars.objects_tbl[104].y_pos = p[4];
		g_vars.objects_tbl[104].spr_num = p[5];
	}
	static const uint16_t pos1_data[] = {
		0x3A9, 0x7D4, 0x1A4, 0x3D1, 0x7B9, 0x1A5, 0x3A2, 0x7A6, 0x1A6,
		0x3B4, 0x79F, 0x1A7, 0x396, 0x78A, 0x1A8, 0x3B6, 0x79C, 0x1A9,
		0x3E0, 0x795, 0x1B6, 0, 0, 0xFFFF
	};
	const uint16_t *p = &pos1_data[g_vars.boss_level5.spr106_pos * 6];
	g_vars.objects_tbl[107].x_pos = p[0];
	g_vars.objects_tbl[107].y_pos = p[1];
	g_vars.objects_tbl[107].spr_num = p[2];
	g_vars.objects_tbl[106].x_pos = p[3];
	g_vars.objects_tbl[106].y_pos = p[4];
	g_vars.objects_tbl[106].spr_num = p[5];
	static const uint16_t pos3_data[] = {
		0x3E3, 0x773, 0x1B0, 0x3E4, 0x771, 0x1B1, 0x3E4, 0x773, 0x1B2
	};
	const uint16_t *q = &pos3_data[g_vars.boss_level5.spr103_pos * 3];
	g_vars.objects_tbl[103].x_pos = q[0];
	g_vars.objects_tbl[103].y_pos = q[1];
	g_vars.objects_tbl[103].spr_num = (g_vars.boss_level5.state != 0) ? 0xFFFF : q[2];
	if ((g_vars.level_draw_counter & 3) == 0) {
		++g_vars.boss_level5.spr103_pos;
		if (g_vars.boss_level5.spr103_pos > 2) {
			g_vars.boss_level5.spr103_pos = 0;
		}
	}
	if (g_vars.boss_level5.unk1 >= 1) {
		if (g_vars.boss_level5.unk1 == 1) {
			g_vars.boss_level5.unk6 = 2;
			--g_vars.boss_level5.spr106_pos;
			g_vars.boss_level5.tick_counter += 5;
		}
		--g_vars.boss_level5.unk1;
	} else if (g_vars.boss_level5.tick_counter == 0) {
		++g_vars.boss_level5.unk6;
		if (g_vars.boss_level5.unk6 > 2) {
			g_vars.boss_level5.unk6 = 0;
		}
		++g_vars.boss_level5.spr106_pos;
		if (g_vars.boss_level5.spr106_pos > 2) {
			g_vars.boss_level5.spr106_pos = 0;
		}
		uint8_t ah = 1;
		if (g_vars.boss_level5.spr106_pos != 0) {
			ah = 3;
			g_vars.shake_screen_counter = 4;
			obj_player->x_pos += 2;
			for (int i = 0; i < 5; ++i) {
				struct boss_level5_leaf_t *leaf = &g_vars.boss_level5.leaf_tbl[i];
				if (leaf->spr_num == 0xFFFF) {
					leaf->spr_num = 0x1B3;
					leaf->y_pos = obj_player->y_pos - 150;
					const uint8_t r = random_get_number();
					const int dx = r & 0x7C;
					leaf->x_pos = obj_player->x_pos + (((r & 4) != 0) ? dx : -dx);
					leaf->y_delta = ((r & 3) + 1) << 4;
					leaf->x_delta = 0;
					leaf->dir = 4;
					break;
				}
			}
		}
		g_vars.boss_level5.tick_counter = ah;
		--g_vars.boss_level5.idle_counter;
		if (g_vars.boss_level5.idle_counter == 0) {
			g_vars.boss_level5.idle_counter = (random_get_number() & 15) << 3;
			g_vars.boss_level5.tick_counter = 64 + ((random_get_number() & 15) << 3);
			g_vars.boss_level5.unk6 = 0;
		}
		if (g_vars.restart_level_flag == 0) {
			if (level_objects_collide(obj_player, &g_vars.objects_tbl[107]) || level_objects_collide(obj_player, &g_vars.objects_tbl[106])) {
				if (obj_player->y_pos <= 1979) {
					obj_player->data.p.y_velocity = -160;
				} else {
					obj_player->data.p.y_velocity = -160;
					obj_player->hit_counter = 44;
					g_vars.player_anim_0x40_flag = 0;
					obj_player->x_friction = 3;
					obj_player->x_velocity = -128;
					level_update_objects_boss_hit_player();
					level_update_objects_boss_hit_player();
					level_update_objects_boss_hit_player();
				}
			}
		}
	} else if (g_vars.restart_level_flag == 0) {
		if (level_objects_collide(obj_player, &g_vars.objects_tbl[107]) && g_vars.player_jump_monster_flag) {
			obj_player->data.p.y_velocity = g_vars.input.key_up ? -128 : -64;
		}
	}
	if (g_vars.restart_level_flag == 0) {
		struct object_t *_si = &g_vars.objects_tbl[103 + g_vars.boss_level5.state * 2];
		for (int i = 0; i < 6; ++i) {
			struct object_t *_di = &g_vars.objects_tbl[i];
			if (_di->spr_num == 0xFFFF || _di == obj_player) {
				continue;
			}
			g_vars.player_using_club_flag = 1;
			const bool ret = level_objects_collide(_si, _di);
			g_vars.player_using_club_flag = 0;
			if (ret) {
				_si->spr_num ^= 0x4000;
				if (g_vars.boss_level5.state != 0) {
					_si[-1].spr_num ^= 0x4000;
				}
				_di->spr_num = 0xFFFF;
				g_vars.boss_level5.unk1 = 6;
				if (g_vars.boss_level5.state < 2) {
					g_vars.boss_level5.unk6 = 3;
					g_vars.boss_level5.spr106_pos = 3;
				}
				--g_vars.boss_level5.energy;
				if (g_vars.boss_level5.energy == 0) {
					g_vars.boss_level5.energy = 7;
					++g_vars.boss_level5.state;
					if (g_vars.boss_level5.state == 3) {
						/* boss defeated */
						g_vars.current_bonus.x_pos = g_vars.objects_tbl[103].x_pos - 200;
						g_vars.current_bonus.y_pos = g_vars.objects_tbl[103].y_pos;
						level_add_bonuses_4x();
						g_vars.current_bonus.x_pos += 32;
						g_vars.current_bonus.y_pos += 32;
						level_add_bonuses_4x();
						g_vars.current_bonus.x_pos -= 64;
						level_add_bonuses_4x();
						g_vars.current_bonus.x_pos -= 32;
						g_vars.current_bonus.y_pos -= 64;
						level_add_bonuses_4x();
						g_vars.current_bonus.spr_num = 99; /* end of level lighter */
						level_add_object23_bonus(0, 0, 1);
						for (int i = 0; i < 5; ++i) {
							g_vars.objects_tbl[98 + i].spr_num = 0xFFFF;
							g_vars.objects_tbl[103 + i].spr_num = 0xFFFF;
						}
					}
				}
				break;
			}
		}
		if (obj_player->x_pos > 984) {
			obj_player->hit_counter = 44;
			g_vars.player_anim_0x40_flag = 0;
			obj_player->data.p.y_velocity = -144;
			obj_player->x_friction = 3;
			obj_player->x_velocity = -160;
			level_update_objects_boss_hit_player();
		}
	}
	if (g_vars.boss_level5.tick_counter > 0) {
		--g_vars.boss_level5.tick_counter;
	}
}

static void level_update_boss_minotaur_set_frame(int num) {
	const int offset = (num * 6 + 20);
	for (int y = 0; y < 7; ++y) {
		for (int x = 0; x < 6; ++x) {
			g_res.leveldat[(y << 8) + 12 + x] = g_res.leveldat[(y << 8) + offset + x];
		}
	}
}

static void level_update_boss_minotaur_add_spr_0x137() { /* boss defeated, bonus */
	g_vars.current_bonus.x_pos = 185;
	g_vars.current_bonus.y_pos = 30;
	g_vars.current_bonus.spr_num = 0x2137;
	int x_vel = 32;
	int y_vel = -160;
	for (int i = 0; i < 4; ++i) {
		level_add_object23_bonus(x_vel, y_vel, 1);
		x_vel = -x_vel;
		if (x_vel >= 0) {
			x_vel -= 16;
			y_vel -= 16;
		}
	}
}

static void level_update_boss_minotaur_add_spr_0x1CA() { /* rock */
	for (int i = 0; i < 32; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[23 + i];
		if (obj->spr_num != 0xFFFF) {
			continue;
		}
		obj->spr_num = 0x1CA;
		obj->x_pos = 200;
		obj->y_pos = 88;
		obj->hit_counter = 0;
		obj->data.t.counter = 132;
		obj->data.t.ref = 0;
		obj->x_velocity = -((random_get_number() & 15) << 3);
		obj->data.t.y_velocity = 0;
		break;
	}
}

static void level_update_boss_minotaur_add_spr_0x1CB() { /* ceiling chandelier */
	for (int i = 0; i < 32; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[23 + i];
		if (obj->spr_num != 0xFFFF) {
			continue;
		}
		obj->spr_num = 0x1CB;
		obj->x_pos = (random_get_number() & 0x7F) - 16;
		obj->y_pos = 0;
		obj->hit_counter = 0;
		obj->data.t.counter = 66;
		obj->data.t.ref = 0;
		obj->x_velocity = 0;
		obj->data.t.y_velocity = 0;
		break;
	}
}

static void level_update_boss_minotaur() {
	static const uint16_t data[] = {
		0xA70F, 5, 0xA74E, 1, 0xA70F, 3, 0xA74E, 2, 0xA70F, 2, 0xA734, 1, 0xFFFF
	};
	if (!g_vars.boss_level9.seq) {
		g_vars.boss_level9.anim = data;
		g_vars.boss_level9.energy = 24;
		g_vars.boss_level9.seq_counter = 0;
		g_vars.boss_level9.hit_counter = 3;
	}
	level_update_objects_boss_energy(g_vars.boss_level9.energy >> 2);
	if (g_vars.boss_level9.energy == 0) {
		level_update_boss_minotaur_set_frame(2);
		return;
	}
	if (g_vars.boss_level9.seq_counter == 0) {
		const uint16_t *p = g_vars.boss_level9.anim;
		if (p[0] == 0xFFFF) {
			p = data;
		}
		assert(p[0] >= 0xA70F);
		g_vars.boss_level9.seq = &boss_minotaur_seq_data[p[0] - 0xA70F];
		g_vars.boss_level9.seq_counter = p[1];
		g_vars.boss_level9.anim = p + 2;
	}
	for (int i = 0; i < 4; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[2 + i];
		if (obj->spr_num == 0xFFFF) {
			continue;
		}
		if (obj->x_pos >= 235 || obj->y_pos >= 80) {
			continue;
		}
		g_vars.boss_level9.seq = &boss_minotaur_seq_data[0x19];
		if (g_vars.boss_level9.energy > 0) {
			--g_vars.boss_level9.energy;
		}
		if (g_vars.boss_level9.energy == 0) {
			level_update_boss_minotaur_add_spr_0x137();
		}
		++g_vars.boss_level9.hit_counter;
		g_vars.boss_level9.hit_counter &= 3;
		if (g_vars.boss_level9.hit_counter == 0) {
			g_vars.boss_level9.seq = &boss_minotaur_seq_data[0x25];
		}
		break;
	}
	const uint8_t *p = g_vars.boss_level9.seq;
	while (1) {
		if (*p == 0xFF) {
			level_update_boss_minotaur_add_spr_0x1CA();
			++p;
		} else if (*p == 0xFE) {
			level_update_boss_minotaur_add_spr_0x1CB();
			++p;
		} else if (*p == 0xFD) {
			play_sound(2);
			++p;
		} else if ((*p & 0x80) == 0) {
			++g_vars.boss_level9.seq;
			level_update_boss_minotaur_set_frame(*p);
			break;
		} else {
			if (g_vars.boss_level9.seq_counter > 0) {
				--g_vars.boss_level9.seq_counter;
			}
			p += (int8_t)*p;
			g_vars.boss_level9.seq = p;
		}
	}
}

void boss_update() {
	if (g_res.level.boss_state != 0xFF) {
		level_update_boss_gorilla();
	}
	if (g_vars.level_num == 5 && (g_res.level.scrolling_mask & ~1) == 0) {
		level_update_boss_tree();
	}
	if (g_vars.level_num == 9) {
		level_update_boss_minotaur();
	}
}

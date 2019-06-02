
/* monsters logic */

#include "game.h"
#include "resource.h"
#include "util.h"

static void monster_reset(struct object_t *obj, struct level_monster_t *m) {
	m->current_tick = 0;
	obj->spr_num = 0xFFFF;
	m->flags &= ~4;
	if ((m->flags & 2) == 0) {
		m->spr_num = 0xFFFF;
	}
}

static void monster_func1_helper(struct object_t *obj, int16_t x_pos, int16_t y_pos) {
	struct level_monster_t *m = obj->data.m.ref;
	if (obj->data.m.state == 0xFF) {
		return;
	}
	if (obj->spr_num & 0x2000) {
		return;
	}
	const int dx = abs(x_pos - g_vars.objects_tbl[1].x_pos);
	if (dx <= TILEMAP_SCREEN_W) {
		const int dy = abs(y_pos - g_vars.objects_tbl[1].y_pos);
		if (dy <= TILEMAP_SCREEN_H + (300 - 176)) {
			return;
		}
	}
	if (obj->data.m.state < 10) {
		obj->spr_num = 0xFFFF;
		m->flags &= ~4;
		m->current_tick = 0;
	} else {
		monster_reset(obj, m);
	}
}

static bool monster_next_tick(struct level_monster_t *m) {
	if (m->current_tick < UCHAR_MAX) {
		++m->current_tick;
	}
	return ((m->current_tick >> 2) < m->respawn_ticks);
}

void monster_change_next_anim(struct object_t *obj) {
	const uint8_t *p = obj->data.m.anim;
	while ((int16_t)READ_LE_UINT16(p) >= 0) {
		p += 2;
	}
	obj->data.m.anim = p + 2;
}

void monster_change_prev_anim(struct object_t *obj) {
	const uint8_t *p = obj->data.m.anim;
	do {
		p -= 2;
	} while ((int16_t)READ_LE_UINT16(p) >= 0);
	obj->data.m.anim = p;
}

static struct orb_t *find_orb() {
	for (int i = 0; i < 20; ++i) {
		struct orb_t *r = &g_vars.orb_tbl[i];
		if (r->x_pos == 0xFFFF) {
			return r;
		}
	}
	return 0;
}

static void monster_add_orb(struct level_monster_t *m, int index, int step) {
	step >>= 2;
	uint8_t radius = step;
	for (int i = 0; i < 3; ++i) {
		struct orb_t *r = find_orb();
		if (r) {
			r->x_pos = m->x_pos;
			r->y_pos = m->y_pos - 24;
			r->radius = radius;
			r->index_tbl = index;
			radius += step;
		}
	}
}

static void monster_rotate_pos(struct object_t *obj, struct level_monster_t *m) {
	obj->x_pos = m->x_pos + ((m->type4.radius * (((int8_t)cos_tbl[m->type4.angle]) >> 2)) >> 4);
	obj->y_pos = m->y_pos + ((m->type4.radius * (((int8_t)sin_tbl[m->type4.angle]) >> 2)) >> 4);
	monster_add_orb(m, m->type4.angle, m->type4.radius);
}

static void monster_update_y_velocity(struct object_t *obj, struct level_monster_t *m) {
	if ((m->flags & 1) != 0 && ((obj->spr_num & 0x2000) != 0 || g_vars.objects_tbl[1].y_pos >= obj->y_pos)) {
		if (obj->data.m.y_velocity < 240) {
			obj->data.m.y_velocity += 15;
		}
	} else {
		monster_reset(obj, m);
	}
}

static void monster_func1_type0(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	const uint8_t state = obj->data.m.state;
	if (state == 0) {
		if (monster_next_tick(m)) {
			return;
		}
		if (obj->y_pos - g_vars.objects_tbl[1].y_pos >= TILEMAP_SCREEN_H) {
			monster_reset(obj, m);
		} else {
			m->flags = 8;
			obj->data.m.state = 1;
		}
	} else if (state == 1) {
		if (obj->data.m.y_velocity != 0) {
			return;
		}
		obj->data.m.state = 2;
		const int x_vel = (obj->x_pos < g_vars.objects_tbl[1].x_pos) ? 32 : -32;
		obj->data.m.x_velocity = x_vel;
		monster_change_next_anim(obj);
	} else if (state == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

static void monster_func1_type2(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	const uint8_t state = obj->data.m.state;
	if (state < 2) {
		monster_add_orb(m, 0, obj->y_pos - m->y_pos);
		if (state == 0) {
			obj->data.m.y_velocity = m->type2.unkE << 4;
			const int dy = obj->y_pos - m->y_pos;
			if (m->type2.y_range >= dy) {
				return;
			}
			obj->data.m.state = 1;
			monster_change_next_anim(obj);
		} else if (state == 1) {
			obj->data.m.y_velocity = -(m->type2.unkE << 4);
			const int dy = obj->y_pos - m->y_pos;
			if (dy >= 0) {
				return;
			}
			obj->data.m.state = 0;
			monster_change_prev_anim(obj);
		}
	} else if (state == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

static void monster_func1_type3(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	const uint8_t state = obj->data.m.state;
	if (state == 0) {
		if (monster_next_tick(m)) {
			return;
		}
		const int dx = abs(m->x_pos - g_vars.objects_tbl[1].x_pos) >> 4;
		if (m->type3.unkD < dx) {
			return;
		}
		m->flags &= ~0x10;
		obj->data.m.state = 1;
		obj->data.m.y_velocity = 32;
		monster_change_next_anim(obj);
	} else if (state == 1) {
		monster_add_orb(m, 0, obj->y_pos - m->y_pos);
		const uint16_t pos = ((obj->y_pos >> 4) << 8) | (obj->x_pos >> 4);
		const uint8_t tile_num = g_res.leveldat[pos];
		if (g_res.level.tile_attributes1[tile_num] == 0) {
			return;
		}
		obj->y_pos &= ~15;
		obj->data.m.y_velocity = 0;
		obj->data.m.state = 2;
		m->flags |= 0x48;
		const int dx = (g_vars.objects_tbl[1].x_pos >= obj->x_pos) ? 48 : -48;
		obj->data.m.x_velocity = dx;
		monster_change_next_anim(obj);
	} else if (state == 2) {
		if (obj->x_pos < 0) {
			obj->data.m.x_velocity = -obj->data.m.x_velocity;
		}
	} else if (state == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

static void monster_func1_type4(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	const uint8_t state = obj->data.m.state;
	if (state == 0) {
		if (monster_next_tick(m)) {
			return;
		}
		const int dy = obj->y_pos - m->y_pos;
		monster_add_orb(m, 0, dy);
		if (m->type4.radius > dy) {
			obj->y_pos += 2;
		} else {
			obj->data.m.state = 1;
		}
	} else if (state == 1) {
		monster_rotate_pos(obj, m);
		m->type4.angle += 4;
		if (m->type4.angle >= m->type4.unkE) {
			m->type4.angle = m->type4.unkE;
			obj->data.m.state = 2;
		}
	} else if (state == 2) {
		monster_rotate_pos(obj, m);
		if (m->type4.angle & 0x80) {
			++m->type4.angle_step;
		} else {
			--m->type4.angle_step;
		}
		m->type4.angle += m->type4.angle_step;
	} else if (state == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

static void monster_func1_type5(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	const uint8_t state = obj->data.m.state;
	if (state == 0) {
		const int x_vel = (obj->x_pos <= g_vars.objects_tbl[1].x_pos) ? 1 : -1;
		obj->data.m.x_velocity = x_vel;
		const int dx = abs(g_vars.objects_tbl[1].x_pos - obj->x_pos) >> 4;
		if (m->type5.x_range < dx) {
			return;
		}
		const int dy = abs(g_vars.objects_tbl[1].y_pos - obj->y_pos) >> 4;
		if (m->type5.y_range < dy) {
			return;
		}
		obj->data.m.state = 10;
		int x = m->type5.unkF << 4;
		obj->data.m.y_velocity = x;
		if (obj->data.m.x_velocity < 0){
			x = -x;
		}
		obj->data.m.x_velocity = x;
	} else if (state == 10) {
		if (abs(obj->y_pos - g_vars.objects_tbl[1].y_pos) <= 8) {
			obj->data.m.state = 11;
			obj->data.m.y_velocity = 10;
		}
	} else if (state == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

static void monster_func1_type6(struct object_t *obj) {
	static const uint8_t data[] = {
		0x40, 0x28, 0x50, 0x26, 0x10, 0x30, 0x20, 0x36, 0x10, 0x3C,
		0x08, 0x32, 0xF8, 0x32, 0xF0, 0x30, 0xE0, 0x28, 0xF0, 0xFF
	};
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	const int x_vel = (obj->x_pos <= g_vars.objects_tbl[1].x_pos) ? 1 : -1;
	obj->data.m.x_velocity = x_vel;
	const uint8_t state = obj->data.m.state;
	if (state == 0) {
		const int dx = abs(g_vars.objects_tbl[1].x_pos - obj->x_pos) >> 4;
		if (m->type6.x_range < dx) {
			return;
		}
		obj->data.m.state = 1;
		m->type6.pos = data;
	} else if (state == 1) {
		const uint8_t *p = m->type6.pos;
		bool flag = false;
		int d = 3;
		int x = g_vars.objects_tbl[1].x_pos + (int8_t)p[0] - obj->x_pos;
		if (x <= 0) {
			x = -x;
			d = -d;
		}
		if (x >= 3) {
			obj->x_pos += d;
			flag = true;
		}
		int y = (int8_t)p[1];
		if (g_vars.player_anim_0x40_flag != 0) {
			y += 5;
		}
		y = g_vars.objects_tbl[1].y_pos - y - obj->y_pos;
		d = 3;
		if (y <= 0) {
			y = -y;
			d = -d;
		}
		if (y >= 3) {
			obj->y_pos += d;
			flag = true;
		}
		if (flag) {
			if (p[2] == 0xFF) {
				m->type6.pos = data;
			} else {
				m->type6.pos = p + 2;
			}
		}
	} else if (state == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

static void monster_func1_type7(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	const uint8_t state = obj->data.m.state;
	if (state == 0) {
		const int x_vel = (obj->x_pos <= g_vars.objects_tbl[1].x_pos) ? 1 : -1;
		obj->data.m.x_velocity = x_vel;
		const int dx = abs(g_vars.objects_tbl[1].x_pos - obj->x_pos) >> 4;
		if (m->type7.unkD < dx) {
			return;
		}
		obj->data.m.state = 10;
		int x = m->type7.unkE << 4;
		obj->data.m.y_velocity = x;
		if (obj->data.m.x_velocity & 0x5000) { /* typo, should be 0x8000, eg. < 0 ? */
			x = -x;
		}
		obj->data.m.x_velocity = x;
		monster_change_next_anim(obj);
	} else if (state == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

static void monster_func1_type8_helper(struct object_t *obj, struct level_monster_t *m) {
	obj->data.m.y_velocity = -(m->type8.y_step << 4);
	int x_vel = m->type8.x_step << 4;
	if (g_vars.objects_tbl[1].x_pos <= obj->x_pos) {
		x_vel = -x_vel;
	}
	obj->data.m.x_velocity = x_vel;
	obj->data.m.state = 10;
	m->flags |= 0x2C;
	monster_change_next_anim(obj);
}

static void monster_func1_type8(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	if (obj->data.m.x_velocity == 0) {
		const int dx = (obj->x_pos <= g_vars.objects_tbl[1].x_pos) ? 1 : -1;
		obj->data.m.x_velocity = dx;
	}
	const uint8_t state = obj->data.m.state;
	if (state == 0 && !monster_next_tick(m)) {
		const int dx = abs(g_vars.objects_tbl[1].x_pos - obj->x_pos);
		if (m->type8.x_range < (dx >> 4)) {
			return;
		}
		const int dy = abs(g_vars.objects_tbl[1].y_pos - obj->y_pos);
		if (m->type8.y_range < (dy >> 4)) {
			return;
		}
		monster_func1_type8_helper(obj, m);
	} else if (state == 10) {
		if (obj->data.m.y_velocity < 0) {
			return;
		}
		obj->data.m.state = 11;
		monster_change_next_anim(obj);
	} else if (state == 11) {
		if (obj->data.m.y_velocity > 0) {
			return;
		}
		obj->data.m.state = 12;
		monster_change_prev_anim(obj);
		monster_change_prev_anim(obj);
		obj->data.m.x_velocity = 0;
		m->current_tick = 0;
	} else if (state == 12 && !monster_next_tick(m)) {
		monster_func1_type8_helper(obj, m);
	} else if (state == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

static void monster_func1_type9(struct object_t *obj) {
	struct level_monster_t *m = obj->data.m.ref;
	if ((obj->spr_num & 0x2000) == 0 && obj->data.m.state != 0xFF) {
		const int dy = abs(obj->y_pos - g_vars.objects_tbl[1].y_pos);
		if (dy >= 190 || (g_vars.objects_tbl[1].x_pos < m->type9.unkD && g_vars.objects_tbl[1].x_pos + 480 < m->type9.unkD) || (g_vars.objects_tbl[1].x_pos >= m->type9.unkD && m->type9.unkF + 480 <= g_vars.objects_tbl[1].x_pos)) {
			obj->spr_num = 0xFFFF;
			m->flags &= ~4;
			return;
		}
	}
	const uint8_t state = obj->data.m.state;
	if (state == 0) {
		obj->data.m.x_velocity = m->type9.x_step;
		const int x = m->type9.x_step + 3;
		if (x <= m->type9.x_dist) {
			m->type9.x_step = x;
		}
		if (m->type9.unkF < obj->x_pos) {
			obj->data.m.state = 1;
		}
	} else if (state == 1) {
		obj->data.m.x_velocity = m->type9.x_step;
		const int x = m->type9.x_step - 3;
		if (x >= -m->type9.x_dist) {
			m->type9.x_step = x;
		}
		if (m->type9.unkD >= obj->x_pos) {
			obj->data.m.state = 0;
		}
	} else if (state == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

static void monster_func1_type10(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	const uint8_t state = obj->data.m.state;
	if (g_vars.level_num == 5 && g_vars.shake_screen_counter != 0 && state != 3 && state != 0xFF) {
		m->current_tick = 1;
	}
	if (state == 0) {
		m->flags |= 0x18;
		if (obj->data.m.y_velocity == 0) {
			obj->data.m.state = 1;
		}
	} else if (state == 1) {
		if (obj->data.m.y_velocity != 0) {
			obj->data.m.state = 0;
		} else {
			m->current_tick = 30;
			obj->data.m.state = 2;
			monster_change_next_anim(obj);
		}
	} else if (state == 2) {
		const int dx = abs(obj->data.m.x_velocity);
		if (dx < 16) {
			if (g_vars.monster.hit_mask == 0) {
				return;
			}
			m->flags = 0xF;
			int x_vel = m->type10.unkD;
			if (obj->x_pos >= g_vars.objects_tbl[1].x_pos) {
				x_vel = -x_vel;
			}
			obj->data.m.x_velocity = x_vel;
		}
		if ((g_vars.level_draw_counter & 3) == 0) {
			--m->current_tick;
			if (m->current_tick == 0) {
				obj->data.m.state = 3;
				obj->data.m.y_velocity = 0;
				obj->data.m.x_velocity >>= 15;
				m->flags = 0x36;
				monster_change_next_anim(obj);
			}
		}
	} else if (state == 3) {
		if (g_vars.monster.hit_mask == 0) {
			return;
		}
		monster_reset(obj, m);
	} else if (state == 0xFF) {
		if ((m->flags & 1) != 0 || (obj->data.m.flags & 0x20) != 0 || g_vars.objects_tbl[1].y_pos >= obj->y_pos) {
			if (obj->data.m.y_velocity < 240) {
				obj->data.m.y_velocity += 15;
			}
		} else {
			monster_reset(obj, m);
		}
	}
}

static void monster_func1_type11(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	const uint8_t state = obj->data.m.state;
	if (state == 0) {
		if (g_vars.monster.hit_mask == 0) {
			return;
		}
		obj->data.m.state = 1;
		m->flags &= ~0x10;
		int x_vel = m->type11.unkD << 4;
		if (g_vars.objects_tbl[1].x_pos <= obj->x_pos) {
			x_vel = -x_vel;
		}
		obj->data.m.x_velocity = x_vel;
		obj->data.m.y_velocity = -(m->type11.unkE << 4);
	} else if (state == 1) {
		if (obj->data.m.y_velocity < (m->type11.unkF << 4)) {
			obj->data.m.y_velocity += 8;
		}
	} else if (state == 0xFF) {
		if ((obj->data.m.flags & 0x20) != 0 || g_vars.objects_tbl[1].y_pos >= obj->y_pos) {
			if (obj->data.m.y_velocity < 240) {
				obj->data.m.y_velocity += 15;
			}
		} else {
			monster_reset(obj, m);
		}
	}
}

static void monster_func1_type12(struct object_t *obj) {
	struct level_monster_t *m = obj->data.m.ref;
	const uint8_t state = obj->data.m.state;
	if (state == 0) {
		int x_vel = m->type12.unkD;
		if (g_vars.objects_tbl[1].x_pos < obj->x_pos) {
			x_vel = -x_vel;
		}
		obj->data.m.x_velocity = x_vel << 4;
		obj->data.m.state = 1;
	} else if (state == 1) {
		if (obj->data.m.flags & 0x20) {
			m->current_tick = 0;
		} else {
			++m->current_tick;
			if (m->current_tick >= 154) {
				obj->data.m.state = 0xFF;
			}
		}
	} else if (state == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

void monster_func1(int type, struct object_t *obj) {
	switch (type) {
	case 0:
		monster_func1_type0(obj);
		break;
	case 1:
		monster_func1_helper(obj, obj->x_pos, obj->y_pos);
		break;
	case 2:
		monster_func1_type2(obj);
		break;
	case 3:
		monster_func1_type3(obj);
		break;
	case 4:
		monster_func1_type4(obj);
		break;
	case 5:
		monster_func1_type5(obj);
		break;
	case 6:
		monster_func1_type6(obj);
		break;
	case 7:
		monster_func1_type7(obj);
		break;
	case 8:
		monster_func1_type8(obj);
		break;
	case 9:
		monster_func1_type9(obj);
		break;
	case 10:
		monster_func1_type10(obj);
		break;
	case 11:
		monster_func1_type11(obj);
		break;
	case 12:
		monster_func1_type12(obj);
		break;
	default:
		print_warning("monster_func1 unhandled monster type %d", type);
		break;
	}
}

static struct object_t *find_object_monster() {
	for (int i = 0; i < MONSTERS_COUNT; ++i) {
		struct object_t *obj = &g_vars.objects_tbl[11 + i];
		if (obj->spr_num == 0xFFFF) {
			return obj;
		}
	}
	return 0;
}

static bool monster_init_object(struct level_monster_t *m) {
	struct object_t *obj = find_object_monster();
	if (obj) {
		obj->data.m.hit_jump_counter = 0;
		g_vars.monster.current_object = obj;
		obj->x_pos = m->x_pos;
		obj->y_pos = m->y_pos;
		obj->spr_num = m->spr_num;
		obj->data.m.ref = m;
		m->flags = 0x17;
		obj->data.m.x_velocity = 0;
		obj->data.m.y_velocity = 0;
		obj->data.m.state = 0;
		obj->data.m.energy = m->energy;
		return true;
	}
	return false;
}

static bool monster_is_visible(int x_pos, int y_pos) {
	const int dx = (x_pos >> 4) - g_vars.tilemap.x;
	if (dx < -2 || dx > (TILEMAP_SCREEN_W + 2)) {
		return false;
	}
	const int dy = (y_pos >> 4) - g_vars.tilemap.y;
	if (dy < -2 || dy > (TILEMAP_SCREEN_H + 2)) {
		return false;
	}
	return true;
}

static bool monster_func2_type0(struct level_monster_t *m) {
	const uint16_t x = m->x_pos;
	const uint16_t y = m->y_pos;
	const int dx = (g_vars.objects_tbl[1].x_pos >> 4) - (x & 255);
	if (dx < 0) {
		return false;
	}
	if ((y & 255) < dx) {
		return false;
	}
	const int dy = (g_vars.objects_tbl[1].y_pos >> 4) - (x >> 8);
	if (dy < 0) {
		return false;
	}
	if ((y >> 8) < dy) {
		return false;
	}
	struct object_t *obj = find_object_monster();
	if (!obj) {
		return false;
	}
	obj->data.m.hit_jump_counter = 0;
	g_vars.monster.current_object = obj;
	g_vars.monster.type0_hdir ^= 1;
	obj->x_pos = g_vars.objects_tbl[1].x_pos + ((g_vars.monster.type0_hdir == 0) ? 192 : -192);
	obj->y_pos = g_vars.objects_tbl[1].y_pos - TILEMAP_SCREEN_H;
	obj->spr_num = m->spr_num;
	obj->data.m.ref = m;
	m->flags = 7;
	obj->data.m.x_velocity = 0;
	obj->data.m.y_velocity = 0;
	obj->data.m.state = 0;
	obj->data.m.energy = m->energy;
	return true;
}

static bool monster_func2_type1(struct level_monster_t *m) {
	if (!monster_is_visible(m->x_pos, m->y_pos)) {
		return false;
	}
	return monster_init_object(m);
}

static bool monster_func2_type2(struct level_monster_t *m) {
	if (!monster_is_visible(m->x_pos, m->y_pos)) {
		return false;
	}
	struct object_t *obj = find_object_monster();
	if (obj) {
		obj->data.m.hit_jump_counter = 0;
		g_vars.monster.current_object = obj;
		obj->x_pos = m->x_pos;
		obj->y_pos = m->y_pos;
		obj->spr_num = m->spr_num;
		obj->data.m.ref = m;
		m->flags = 5;
		obj->data.m.y_velocity = 0;
		obj->data.m.x_velocity = 0;
		obj->data.m.state = 0;
		obj->data.m.energy = m->energy;
		return true;
	}
	return false;
}

static bool monster_func2_type3(struct level_monster_t *m) {
	if (!monster_func2_type1(m)) {
		return false;
	}
	m->flags = 0x37;
	return true;
}

static bool monster_func2_type4(struct level_monster_t *m) {
	if (!monster_func2_type1(m)) {
		return false;
	}
	m->flags = 5;
	m->type4.angle = 0;
	m->type4.angle_step = 0;
	return true;
}

static bool monster_func2_type5_6_7_8(struct level_monster_t *m) {
	if (!monster_func2_type1(m)) {
		return false;
	}
	m->flags = 5;
	return true;
}

static bool monster_func2_type9(struct level_monster_t *m) {
	m->type9.x_step = 0;
	uint8_t flags = m->flags;
	if (!monster_func2_type1(m)) {
		return false;
	}
	flags |= 5;
	if (g_vars.level_num == 6) {
		flags |= 0x80;
	}
	m->flags = flags;
	return true;
}

static bool monster_func2_type10(struct level_monster_t *m) {
	if (g_vars.level_num == 5 && g_vars.shake_screen_counter != 0) {
		return false;
	}
	if (m->current_tick < UCHAR_MAX) {
		++m->current_tick;
	}
	if (m->respawn_ticks > (m->current_tick >> 2)) {
		return false;
	}
	const uint16_t x = m->x_pos;
	const uint16_t y = m->y_pos;
	const int dx = (g_vars.objects_tbl[1].x_pos >> 4) - (x & 255);
	if (dx < 0) {
		return false;
	}
	if ((y & 255) < dx) {
		return false;
	}
	const int dy = (g_vars.objects_tbl[1].y_pos >> 4) - (x >> 8);
	if (dy < 0) {
		return false;
	}
	if ((y >> 8) < dy) {
		return false;
	}
	struct object_t *obj = find_object_monster();
	if (!obj) {
		return false;
	}
	obj->data.m.hit_jump_counter = 0;
	g_vars.monster.current_object = obj;
	static const int16_t dist_tbl[] = { 120, 100, -90, 110, -120, 40, 80, 60 };
	obj->x_pos = g_vars.objects_tbl[1].x_pos + dist_tbl[g_vars.monster.type10_dist & 7];
	++g_vars.monster.type10_dist;
	obj->data.m.x_velocity = 0;
	uint8_t bh = (g_vars.objects_tbl[1].y_pos >> 4) + 4;
	uint8_t bl = (obj->x_pos >> 4);
	uint16_t pos = (bh << 8) | bl;
	for (int i = 0; i < 10 && pos >= 0x300; ++i, pos -= 0x100) {
		if (pos < (g_vars.tilemap.h << 8)) {
			bool init_spr = true;
			for (int j = 0; j < 3; ++j) {
				const uint8_t tile_num = g_res.leveldat[pos - j * 0x100];
				if (g_res.level.tile_attributes1[tile_num] != 0) {
					init_spr = false;
					break;
				}
			}
			if (init_spr) {
				obj->y_pos = (pos >> 8) << 4;
				obj->spr_num = m->spr_num;
				obj->data.m.ref = m;
				m->flags = 0x17;
				obj->data.m.state = 0;
				obj->data.m.y_velocity = 0;
				obj->data.m.energy = m->energy;
				return true;
			}
		}
	}
	return false;
}

static bool monster_func2_type11(struct level_monster_t *m) {
	if (m->current_tick < UCHAR_MAX) {
		++m->current_tick;
	}
	if (m->respawn_ticks > (m->current_tick >> 2) || !monster_func2_type1(m)) {
		return false;
	}
	m->flags = 0x37;
	g_vars.monster.current_object->y_pos -= (random_get_number() & 0x3F);
	return true;
}

static bool monster_func2_type12(struct level_monster_t *m) {
	if (g_vars.objects_tbl[1].y_pos <= m->y_pos) {
		return false;
	}
	const int dx = abs(m->x_pos - g_vars.objects_tbl[1].x_pos);
	if (dx >= TILEMAP_SCREEN_W * 2) {
		return false;
	}
	if (dx <= TILEMAP_SCREEN_W) {
		const int dy = g_vars.objects_tbl[1].y_pos - m->y_pos;
		if (dy >= 360 || dy <= 180) {
			return false;
		}
	}
	if (m->current_tick < UCHAR_MAX) {
		++m->current_tick;
	}
	if (m->respawn_ticks > (m->current_tick >> 2)) {
		return false;
	}
	if (!monster_init_object(m)) {
		return false;
	}
	m->flags = 0x8F;
	m->current_tick = 0;
	return true;
}

bool monster_func2(int type, struct level_monster_t *m) {
	switch (type) {
	case 0:
		return monster_func2_type0(m);
	case 1:
		return monster_func2_type1(m);
	case 2:
		return monster_func2_type2(m);
	case 3:
		return monster_func2_type3(m);
	case 4:
		return monster_func2_type4(m);
	case 5:
	case 6:
	case 7:
	case 8:
		return monster_func2_type5_6_7_8(m);
	case 9:
		return monster_func2_type9(m);
	case 10:
		return monster_func2_type10(m);
	case 11:
		return monster_func2_type11(m);
	case 12:
		return monster_func2_type12(m);
	default:
		print_warning("monster_func2 unhandled monster type %d", type);
		break;
	}
	return false;
}

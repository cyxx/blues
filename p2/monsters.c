
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
	if (dx <= 320) {
		const int dy = abs(y_pos - g_vars.objects_tbl[1].y_pos);
		if (dy <= 300) {
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
	if (m->current_tick < 255) {
		++m->current_tick;
	}
	return ((m->current_tick >> 2) < m->total_ticks);
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

static struct rotation_t *find_rotation() {
	for (int i = 0; i < 20; ++i) {
		struct rotation_t *r = &g_vars.rotation_tbl[i];
		if (r->x_pos == 0xFFFF) {
			return r;
		}
	}
	return 0;
}

static void monster_rotate_tiles(struct level_monster_t *m, int index, int step) {
	step >>= 2;
	uint8_t radius = step;
	for (int i = 0; i < 3; ++i) {
		struct rotation_t *r = find_rotation();
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
	monster_rotate_tiles(m, m->type4.angle, m->type4.radius);
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

static void monster_func1_type2(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	const uint8_t state = obj->data.m.state;
	if (state < 2) {
		monster_rotate_tiles(m, 0, obj->y_pos - m->y_pos);
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

static void monster_func1_type4(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	const uint8_t state = obj->data.m.state;
	if (state == 0) {
		if (monster_next_tick(m)) {
			return;
		}
		const int dy = obj->y_pos - m->y_pos;
		monster_rotate_tiles(m, 0, dy);
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
			++m->type4.unk10;
		} else {
			--m->type4.unk10;
		}
		m->type4.angle = m->type4.unk10;
	} else if (state == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

static void monster_func1_type8(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	if (obj->data.m.x_velocity != 0) {
		const int dx = (obj->x_pos <= g_vars.objects_tbl[1].x_pos) ? 1 : -1;
		obj->data.m.x_velocity = dx;
	}
	const uint8_t state = obj->data.m.state;
	if ((state == 0 && !monster_next_tick(m)) || (state == 12 && !monster_next_tick(m))) {
		const int dx = abs(g_vars.objects_tbl[1].x_pos - obj->x_pos);
		if (m->type8.x_range < (dx >> 4)) {
			return;
		}
		const int dy = abs(g_vars.objects_tbl[1].y_pos - obj->y_pos);
		if (m->type8.y_range < (dy >> 4)) {
			return;
		}
		obj->data.m.y_velocity = -(m->type8.unkE << 4);
		int x_vel = m->type8.unkF << 4;
		if (g_vars.objects_tbl[1].x_pos <= obj->x_pos) {
			x_vel = -x_vel;
		}
		obj->data.m.x_velocity = x_vel;
		obj->data.m.state = 10;
		m->flags = (m->flags & ~0x2C) | 0x2C;
		monster_change_next_anim(obj);
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
		obj->data.m.x_velocity = m->type9.unk11;
		const int x = m->type9.unk11 + 3;
		if (x <= m->type9.unk12) {
			m->type9.unk11 = x;
		}
		if (m->type9.unkF < obj->x_pos) {
			obj->data.m.state = 1;
		}
	} else if (state == 1) {
		obj->data.m.x_velocity = m->type9.unk11;
		const int x = m->type9.unk11 - 3;
		if (x >= -m->type9.unk12) {
			m->type9.unk11 = x;
		}
		if (m->type9.unkD >= obj->x_pos) {
			obj->data.m.state = 1;
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
		if ((m->flags & 1) != 0 || (obj->data.m.unk5 & 0x20) != 0 || g_vars.objects_tbl[1].y_pos >= obj->y_pos) {
			if (obj->data.m.y_velocity < 240) {
				obj->data.m.y_velocity += 15;
			}
		} else {
			monster_reset(obj, m);
		}
	}
}

void monster_func1(int type, struct object_t *obj) {
	switch (type) {
	case 1:
		monster_func1_helper(obj, obj->x_pos, obj->y_pos);
		break;
	case 2:
		monster_func1_type2(obj);
		break;
	case 4:
		monster_func1_type4(obj);
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
		obj->data.m.unk10 = 0;
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
	const int dx = (x_pos >> 4) - g_vars.tilemap_x;
	if (dx < -2 || dx > (TILEMAP_SCREEN_W + 2)) {
		return false;
	}
	const int dy = (y_pos >> 4) - g_vars.tilemap_y;
	if (dy < -2 || dy > (TILEMAP_SCREEN_H + 2)) {
		return false;
	}
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
		obj->data.m.unk10 = 0;
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

static bool monster_func2_type4(struct level_monster_t *m) {
	if (!monster_func2_type1(m)) {
		return false;
	}
	m->flags = 5;
	m->type4.angle = 0;
	m->type4.unk10 = 0;
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
	if (m->current_tick < 255) {
		++m->current_tick;
	}
	if (m->total_ticks > (m->current_tick >> 2)) {
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
	obj->data.m.unk10 = 0;
	g_vars.monster.current_object = obj;
	static const int16_t dist_tbl[] = { 120, 100, -90, 110, -120, 40, 80, 60 };
	obj->x_pos = g_vars.objects_tbl[1].x_pos + dist_tbl[g_vars.monster.type10_dist & 7];
	++g_vars.monster.type10_dist;
	obj->data.m.x_velocity = 0;
	uint8_t bh = (g_vars.objects_tbl[1].y_pos >> 4) + 4;
	uint8_t bl = (obj->x_pos >> 4);
	uint16_t bp = (bh << 8) | bl;
	for (int i = 0; i < 10; ++i) {
		if (bp < (g_vars.tilemap_h << 8)) {
			bool init_spr = true;
			for (int j = 0; j < 3; ++j) {
				const uint8_t tile_num = g_res.leveldat[bp - j * 0x100];
				if (g_res.level.tile_attributes1[tile_num] != 0) {
					init_spr = false;
					break;
				}
			}
			if (init_spr) {
				obj->y_pos = bh << 4;
				obj->spr_num = m->spr_num;
				obj->data.m.ref = m;
				m->flags = 0x17;
				obj->data.m.state = 0;
				obj->data.m.y_velocity = 0;
				obj->data.m.energy = m->energy;
				return true;
			}
		}
		bp -= 0x100;
		if (bp < 0x300) {
			return false;
		}
	}
	return false;
}

static bool monster_func2_type11(struct level_monster_t *m) {
	if (m->current_tick < 255) {
		++m->current_tick;
	}
	if (m->total_ticks > (m->current_tick >> 2) || !monster_func2_type1(m)) {
		return false;
	}
	m->flags = 0x37;
	g_vars.monster.current_object->y_pos -= (random_get_number() & 0x3F);
	return true;
}

bool monster_func2(int type, struct level_monster_t *m) {
	switch (type) {
	case 1:
		return monster_func2_type1(m);
	case 2:
		return monster_func2_type2(m);
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
	default:
		print_warning("monster_func2 unhandled monster type %d", type);
		break;
	}
	return false;
}

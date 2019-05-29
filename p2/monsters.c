
#include "game.h"
#include "resource.h"
#include "util.h"

static void monster_func1_helper(struct object_t *obj, int16_t x_pos, int16_t y_pos) {
	struct level_monster_t *m = obj->data.m.ref;
	if (obj->data.m.unkE == 0xFF) {
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
	if (obj->data.m.unkE < 10) {
		obj->spr_num = 0xFFFF;
		m->flags &= ~4;
		m->current_tick = 0;
	} else {
		m->current_tick = 0;
		obj->spr_num = 0xFFFF;
		m->flags &= ~4;
		if ((m->flags & 2) == 0) {
			m->spr_num = 0xFFFF;
		}
	}
}

static bool monster_next_tick(struct level_monster_t *m) {
	if (m->current_tick < 255) {
		++m->current_tick;
	}
	return ((m->current_tick >> 2) < m->total_ticks);
}

static void monster_change_next_anim(struct object_t *obj) {
	const uint8_t *p = obj->data.m.anim;
	while ((int16_t)READ_LE_UINT16(p) >= 0) {
		p += 2;
	}
	obj->data.m.anim = p + 2;
}

static void monster_change_prev_anim(struct object_t *obj) {
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

static void monster_rotate_pos(struct level_monster_t *m, int index, int step) {
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

static void monster_update_y_velocity(struct object_t *obj, struct level_monster_t *m) {
	if ((m->flags & 1) != 0 && ((obj->spr_num & 0x2000) != 0 || g_vars.objects_tbl[1].y_pos >= obj->y_pos)) {
		if (obj->data.m.y_velocity < 240) {
			obj->data.m.y_velocity += 15;
		}
	} else {
		m->current_tick = 0;
		obj->spr_num = 0xFFFF;
		m->flags &= ~4;
		if ((m->flags & 2) == 0) {
			m->spr_num = 0xFFFF;
		}
	}
}

static void monster_func1_type2(struct object_t *obj) {
	monster_func1_helper(obj, obj->x_pos, obj->y_pos);
	struct level_monster_t *m = obj->data.m.ref;
	uint8_t al = obj->data.m.unkE;
	if (al < 2) {
		monster_rotate_pos(m, 0, obj->y_pos - m->y_pos);
		if (al == 0) {
			obj->data.m.y_velocity = m->type2.unkE << 4;
			const int dy = obj->y_pos - m->y_pos;
			if (m->type2.y_range >= dy) {
				return;
			}
			obj->data.m.unkE = 1;
			monster_change_next_anim(obj);
		} else if (al == 1) {
			obj->data.m.y_velocity = m->type2.unkE << 4;
			const int dy = obj->y_pos - m->y_pos;
			if (dy >= 0) {
				return;
			}
			obj->data.m.unkE = 0;
			monster_change_prev_anim(obj);
		}
	} else if (al == 0xFF) {
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
	uint8_t al = obj->data.m.unkE;
	if ((al == 0 && !monster_next_tick(m)) || (al == 12 && !monster_next_tick(m))) {
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
		obj->data.m.unkE = 10;
		m->flags = (m->flags & ~0x2C) | 0x2C;
		monster_change_next_anim(obj);
	} else if (al == 10) {
		if (obj->data.m.y_velocity < 0) {
			return;
		}
		obj->data.m.unkE = 11;
		monster_change_next_anim(obj);
	} else if (al == 11) {
		if (obj->data.m.y_velocity > 0) {
			return;
		}
		obj->data.m.unkE = 12;
		monster_change_prev_anim(obj);
		monster_change_prev_anim(obj);
		obj->data.m.x_velocity = 0;
		m->current_tick = 0;
	} else if (al == 0xFF) {
		monster_update_y_velocity(obj, m);
	}
}

static void monster_func1_type9(struct object_t *obj) {
	struct level_monster_t *m = obj->data.m.ref;
	if ((obj->spr_num & 0x2000) == 0 && obj->data.m.unkE != 0xFF) {
	} else {
		uint8_t al = obj->data.m.unkE;
		if (al == 0) {
		} else if (al == 1) {
		} else if (al == 0xFF) {
			monster_update_y_velocity(obj, m);
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
	case 8:
		monster_func1_type8(obj);
		break;
	case 9:
		monster_func1_type9(obj);
		break;
	default:
		print_warning("monster_func1 unhandled monster type %d", type);
		break;
	}
}

static struct object_t *find_object_monster() {
	for (int i = 0; i < 12; ++i) {
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
		obj->data.m.unkE = 0;
		obj->data.m.unkF = m->unk5;
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
	const int16_t x_pos = m->x_pos;
	const int16_t y_pos = m->y_pos;
	if (!monster_is_visible(x_pos, y_pos)) {
		return false;
	}
	return monster_init_object(m);
}

static bool monster_func2_type2(struct level_monster_t *m) {
	const int16_t x_pos = m->x_pos;
	const int16_t y_pos = m->y_pos;
	if (!monster_is_visible(x_pos, y_pos)) {
		return false;
	}
	struct object_t *obj = find_object_monster();
	if (obj) {
		obj->data.m.unk10 = 0;
		g_vars.monster.current_object = obj;
		obj->x_pos = x_pos;
		obj->y_pos = y_pos;
		obj->spr_num = m->spr_num;
		obj->data.m.ref = m;
		m->flags = 5;
		obj->data.m.y_velocity = 0;
		obj->data.m.x_velocity = 0;
		obj->data.m.unkE = 0;
		obj->data.m.unkF = m->unk5;
		return true;
	}
	return false;
}

static bool monster_func2_type4(struct level_monster_t *m) {
	if (!monster_func2_type1(m)) {
		return false;
	}
	m->flags = 5;
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
	return true;
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

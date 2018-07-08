
/* tiles update and object triggers */

#include "game.h"
#include "resource.h"
#include "util.h"

uint16_t triggers_get_tile_type(int x, int y) {
	const uint8_t *p = lookup_sql(x, y);
	const int num = p[0];
	return g_res.triggers[num].tile_type;
}

uint16_t triggers_get_next_tile_flags(int x, int y) {
	const uint8_t *p = lookup_sql(x, y);
	int num = p[0];
	num = g_res.triggers[num].unk16;
	return g_res.triggers[num].tile_flags;
}

uint16_t triggers_get_tile_data(struct object_t *obj) {
	const uint8_t *p = lookup_sql(obj->xpos16, obj->ypos16 + 1);
	const int num = p[0];
	return g_res.triggers[num].unk10;
}

uint16_t triggers_get_next_tile_num(int x, int y) {
	const uint8_t *p = lookup_sql(x,  y);
	const int num = p[0];
	return g_res.triggers[num].unk16;
}

static void trigger_func_op0(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->yfriction = 0;
	} else if (obj->yvelocity < 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + 15;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->yfriction = 0;
	}
	obj->unk2F = 0;
	obj->unk3D = 0;
}

static void trigger_func_op1(struct object_t *obj) {
	if (obj->data5F == g_vars.level + 1) {
		obj->carry_crate_flag = 1;
		do_level_enter_door(obj);
	}
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->yfriction = 0;
	} else if (obj->yvelocity < 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + 15;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->yfriction = 0;
	}
	obj->unk2F = 0;
}

static void trigger_func_op2(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->yfriction = 0;
		if (!g_vars.screen_unk1) {
			obj->unk1C = 5;
		}
	} else if (obj->yvelocity < 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + 15;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->yfriction = 0;
	}
	obj->unk2F = 0;
	if (g_vars.screen_unk1) {
		obj->special_anim = 18;
		obj->anim_num = 1;
	}
}

static void trigger_func_op3(struct object_t *obj) {
	obj->yvelocity = 0;
	obj->ypos = (obj->ypos & ~15) - 1;
	obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
	obj->ypos16 = obj->ypos >> 4;
	obj->sprite_type = 1;
	obj->yfriction = 0;
	if (obj->data5F != g_vars.level + 1) {
		if (obj->yfriction == 0) {
			obj->yfriction = 15;
		}
	} else {
		obj->xpos16 = 101;
		obj->ypos16 = 70;
		obj->xpos = obj->xpos16 << 4;
		obj->ypos = obj->ypos16 << 4;
		obj->screen_xpos = obj->xpos - g_vars.screen_tilemap_xorigin;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		g_vars.screen_unk1 = 1;
	}
}

static void trigger_func_op4(struct object_t *obj) {
	if (obj->unk54 == 0) {
		obj->unk54 = 0;
		obj->unk55 = 0;
		obj->unk56 = 8;
		obj->sprite_type = 6;
	} else {
		obj->special_anim = 22;
		obj->unk54 = 0;
		obj->unk55 = 0;
		obj->unk56 = 0;
		obj->yfriction = 8;
		obj->sprite_type = 0;
	}
}

static void trigger_func_op5(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->yfriction = 0;
	}
	if (obj->unk55 == 0) {
		obj->unk54 = 0;
		obj->unk55 = 2;
		obj->unk56 = 9;
		obj->sprite_type = 6;
	} else if (obj->unk56 != 9) {
		obj->special_anim = 22;
		obj->unk54 = 0;
		obj->unk55 = 0;
		obj->unk56 = 0;
		obj->sprite_type = 0;
	}
}

static void trigger_func_op6(struct object_t *obj) {
	if (obj->unk56 == 0) {
		obj->unk54 = 2;
		obj->unk55 = 0;
		obj->unk56 = 7;
	} else if (obj->unk56 != 7) {
		obj->special_anim = 22;
		obj->unk54 = 0;
		obj->unk55 = 0;
		obj->unk56 = 0;
		obj->sprite_type = 0;
	}
}

static void trigger_func_op7(struct object_t *obj) {
	if (obj->unk56 == 2 || obj->unk56 == 7) {
		obj->unk54 = 2;
	}
	obj->unk56 = 1;
}

static void trigger_func_op8(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->yfriction = 0;
	}
	switch (obj->unk56 - 1) {
	case 0:
		obj->unk55 = 1;
		break;
	case 2:
		obj->unk54 = 2;
		break;
	case 3:
		obj->unk54 = 2;
		break;
	case 4:
		obj->unk55 = 1;
		break;
	case 5:
		obj->unk55 = 1;
		break;
	case 7:
		obj->unk55 = 1;
		break;
	}
}

static void trigger_func_op9(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->yfriction = 0;
	}
	obj->unk56 = 3;
	obj->unk54 = 0;
}

static void trigger_func_op10(struct object_t *obj) {
	obj->unk56 = 3;
	obj->unk54 = 0;
}

static void trigger_func_op11(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->yfriction = 0;
	}
	if (obj->unk56 == 1) {
		obj->unk55 = 2;
	} else if (obj->unk56 == 3) {
		obj->unk54 = 2;
	}
	obj->unk56 = 4;
}

static void trigger_func_op12(struct object_t *obj) {
	if (obj->unk56 == 1) {
		obj->unk55 = 1;
		obj->unk54 = 2;
	} else if (obj->unk56 == 3 || obj->unk56 == 6) {
		obj->unk54 = 1;
		obj->unk55 = 0;
	}
	obj->unk56 = 5;
	obj->sprite_type = 6;
}

static void trigger_func_op13(struct object_t *obj) {
	if (obj->unk56 == 1) {
		obj->unk55 = 2;
		obj->unk54 = 2;
	} else if (obj->unk56 == 3) {
		obj->unk54 = 1;
	}
	obj->unk56 = 6;
	obj->sprite_type = 6;
}

static void trigger_func_op14(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
	}
	obj->unk2F = 0;
}

static void trigger_func_op15(struct object_t *obj) {
	if (obj->special_anim != 18) {
		if (obj->floor_ypos16 < obj->ypos16 && obj->unk2B == 0) {
			obj->yvelocity = 0;
			obj->ypos = (obj->ypos & ~15) - 1;
			obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
			obj->ypos16 = obj->ypos >> 4;
		}
		obj->sprite_type = 2;
		obj->unk2F = 0;
	}
}

static void trigger_func_op16(struct object_t *obj) {
	if (obj->special_anim != 18) {
		if (obj->floor_ypos16 < obj->ypos16 && obj->unk2B == 0) {
			obj->yvelocity = 0;
			obj->ypos = (obj->ypos & ~15) - 1;
			obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
			obj->ypos16 = obj->ypos >> 4;
		}
		obj->sprite_type = 3;
		obj->unk2F = 0;
	}
}

static void trigger_func_op17(struct object_t *obj) {
	if (obj->yvelocity < -2) {
		obj->yvelocity -= 3;
		obj->yacc = -3;
	}
	obj->xpos = (obj->xpos & ~15) + 7;
	obj->screen_xpos = obj->xpos - g_vars.screen_tilemap_xorigin;
	obj->xpos16 = obj->xpos >> 4;
}

static void trigger_func_op18(struct object_t *obj) {
	if (obj->yvelocity < -2) {
		obj->yvelocity -= 3;
		obj->yacc = -3;
	}
	obj->xpos = (obj->xpos & ~15) + 17;
	obj->screen_xpos = obj->xpos - g_vars.screen_tilemap_xorigin;
	obj->xpos16 = obj->xpos >> 4;
}

static void trigger_func_op19(struct object_t *obj) {
	if (obj->yvelocity < -2) {
		obj->yvelocity -= 3;
		obj->yacc = -3;
	}
	obj->xpos = (obj->xpos & ~15) - 1;
	obj->screen_xpos = obj->xpos - g_vars.screen_tilemap_xorigin;
	obj->xpos16 = obj->xpos >> 4;
}

static void trigger_func_op20(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->yfriction = 0;
	} else if (obj->yvelocity < 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + 15;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 0;
	}
	obj->unk2F = 0;
}

static void trigger_func_op21(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->yfriction = 0;
	} else if (obj->yvelocity < 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + 15;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->yfriction = 0;
	}
	obj->unk2F = 0;
	obj->sprite_type = 4;
	obj->unk2D = 1;
}

static void trigger_func_op22(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
	}
	obj->unk2F = 0;
	obj->sprite_type = 4;
	obj->unk2D = 1;
}

static void trigger_func_op23(struct object_t *obj) {
	obj->sprite_type = 4;
	obj->unk2D = 1;
	obj->unk2F = 0;
	obj->yvelocity = 0;
}

static void trigger_func_op24(struct object_t *obj) {
	obj->yfriction = (obj->yvelocity & 255) + 6;
	if (obj->floor_ypos16 < obj->ypos16 && obj->unk2B == 0) {
		play_sound(SOUND_14);
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
	}
	obj->unk3D = 0;
}

// speedwalk (right)
static void trigger_func_op27(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->unk1C = 4;
		obj->sprite_type = 1;
	} else if (obj->yvelocity < 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + 15;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
	}
	obj->unk2F = 0;
}

// speedwalk (left)
static void trigger_func_op28(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->unk1C = -4;
		obj->sprite_type = 1;
	} else if (obj->yvelocity < 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + 15;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
	}
	obj->unk2F = 0;
}

static void trigger_func_op29(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->unk2D = 1;
	} else if (obj->yvelocity < 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + 15;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
	}
	obj->unk2F = 0;
}

static void trigger_func_op30(struct object_t *obj) {
	if (obj->floor_ypos16 < obj->ypos16 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->unk2D = 2;
	}
	obj->unk2F = 0;
}

static void trigger_func_op31(struct object_t *obj) {
	if (obj->yvelocity > 0 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + triggers_get_dy(obj);
		obj->ypos16 = obj->ypos >> 4;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->sprite_type = 1;
		obj->unk1C = 1;
		obj->xvelocity = 1;
	}
}

static void trigger_func_op32(struct object_t *obj) {
	if (obj->yvelocity > 0 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + triggers_get_dy(obj);
		obj->ypos16 = obj->ypos >> 4;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->sprite_type = 1;
		obj->unk2F = -3;
		obj->unk2D = 0;
		obj->facing_left = 1;
	}
}

static void trigger_func_op33(struct object_t *obj) {
	if (obj->yvelocity > 0 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 2;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->unk2F = -3;
		obj->unk2D = 0;
		obj->facing_left = 1;
	}
}

static void trigger_func_op34(struct object_t *obj) {
	if (obj->yvelocity > 0 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + triggers_get_dy(obj);
		obj->ypos16 = obj->ypos >> 4;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->sprite_type = 1;
		obj->unk2F = 3;
		obj->unk2D = 0;
		obj->facing_left = 0;
	}
}

static void trigger_func_op35(struct object_t *obj) {
	if (obj->yvelocity > 0 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 2;
		obj->ypos16 = obj->ypos >> 4;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->sprite_type = 1;
		obj->unk2F = 3;
		obj->unk2D = 0;
		obj->facing_left = 0;
	}
}

static void trigger_func_op36(struct object_t *obj) {
	if (obj->yvelocity > 0 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + triggers_get_dy(obj);
		obj->ypos16 = obj->ypos >> 4;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->sprite_type = 4;
		obj->unk2D = 2;
	}
}

static void trigger_func_op37(struct object_t *obj) {
	if (obj->yvelocity > 0 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 2;
		obj->ypos16 = obj->ypos >> 4;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->sprite_type = 4;
		obj->unk2D = 2;
	}
}

static void trigger_func_op38(struct object_t *obj) {
	if (obj->yvelocity > 0 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + triggers_get_dy(obj);
		obj->ypos16 = obj->ypos >> 4;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->sprite_type = 4;
		obj->unk2D = 2;
	}
}

static void trigger_func_op39(struct object_t *obj) {
	if (obj->yvelocity > 0 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 2;
		obj->ypos16 = obj->ypos >> 4;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->sprite_type = 4;
		obj->unk2D = 2;
	}
}

static void trigger_func_op40(struct object_t *obj) {
	static int counter = 0;
	g_vars.music_num = 0;
	if (obj->yvelocity > 0 && obj->unk2B == 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) - 1;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
		obj->sprite_type = 1;
		obj->yfriction = 0;
	} else if (obj->yvelocity < 0) {
		obj->yvelocity = 0;
		obj->ypos = (obj->ypos & ~15) + 15;
		obj->screen_ypos = obj->ypos - g_vars.screen_tilemap_yorigin;
		obj->ypos16 = obj->ypos >> 4;
	}
	obj->unk2F = 0;
	obj->special_anim = 21;
	if (counter == 0) {
		play_music(g_vars.music_num);
	}
	if (counter == 400) {
		if (!g_vars.two_players_flag) {
			g_vars.level_completed_flag = 1;
		} else if (g_vars.objects[OBJECT_NUM_PLAYER1].flag_end_level != 0 && g_vars.objects[OBJECT_NUM_PLAYER2].flag_end_level != 0) {
			g_vars.level_completed_flag = 1;
		}
	}
	++counter;
}

void level_call_trigger_func(struct object_t *obj, int y) {
	const uint8_t *p = lookup_sql(obj->xpos16, obj->ypos16 + y);
	int num = p[0];
	num = g_res.triggers[num].unk16;
	switch (g_res.triggers[num].op_func) {
	case 0:
		trigger_func_op0(obj);
		break;
	case 1:
		trigger_func_op1(obj);
		break;
	case 2:
		trigger_func_op2(obj);
		break;
	case 3:
		trigger_func_op3(obj);
		break;
	case 4:
		trigger_func_op4(obj);
		break;
	case 5:
		trigger_func_op5(obj);
		break;
	case 6:
		trigger_func_op6(obj);
		break;
	case 7:
		trigger_func_op7(obj);
		break;
	case 8:
		trigger_func_op8(obj);
		break;
	case 9:
		trigger_func_op9(obj);
		break;
	case 10:
		trigger_func_op10(obj);
		break;
	case 11:
		trigger_func_op11(obj);
		break;
	case 12:
		trigger_func_op12(obj);
		break;
	case 13:
		trigger_func_op13(obj);
		break;
	case 14:
		trigger_func_op14(obj);
		break;
	case 15:
		trigger_func_op15(obj);
		break;
	case 16:
		trigger_func_op16(obj);
		break;
	case 17:
		trigger_func_op17(obj);
		break;
	case 18:
		trigger_func_op18(obj);
		break;
	case 19:
		trigger_func_op19(obj);
		break;
	case 20:
		trigger_func_op20(obj);
		break;
	case 21:
		trigger_func_op21(obj);
		break;
	case 22:
		trigger_func_op22(obj);
		break;
	case 23:
		trigger_func_op23(obj);
		break;
	case 24:
		trigger_func_op24(obj);
		break;
	case 25:
		break;
	case 27:
		trigger_func_op27(obj);
		break;
	case 28:
		trigger_func_op28(obj);
		break;
	case 29:
		trigger_func_op29(obj);
		break;
	case 30:
		trigger_func_op30(obj);
		break;
	case 31:
		trigger_func_op31(obj);
		break;
	case 32:
		trigger_func_op32(obj);
		break;
	case 33:
		trigger_func_op33(obj);
		break;
	case 34:
		trigger_func_op34(obj);
		break;
	case 35:
		trigger_func_op35(obj);
		break;
	case 36:
		trigger_func_op36(obj);
		break;
	case 37:
		trigger_func_op37(obj);
		break;
	case 38:
		trigger_func_op38(obj);
		break;
	case 39:
		trigger_func_op39(obj);
		break;
	case 40:
		trigger_func_op40(obj);
		break;
	default:
		print_warning("level_call_trigger_func: op_func %d unimplemented", g_res.triggers[num].op_func);
		break;
	}
}

void triggers_update_tiles1(struct object_t *obj) {
	struct object_t *obj35 = &g_vars.objects[35];
	struct object_t *obj37 = &g_vars.objects[37];
	if (obj->yvelocity >= 0 && obj->special_anim != 18) {
		obj->special_anim = 0;
	}
	int _di = 0;
	const uint8_t *p = lookup_sql(obj->xpos16, obj->ypos16);
	int num = p[0];
	num = g_res.triggers[num].unk16;
	struct trigger_t *t = &g_res.triggers[num];
	if (!t->op_table3 && obj->unk2E != 0) {
		p = lookup_sql(obj->xpos16, obj->ypos16 - 1);
		num = p[0];
		t = &g_res.triggers[num];
		_di = 1;
	}
	if (!t->op_table3) {
		return;
	}
	p = t->op_table3;
	if (p[1] == 11) {
		if (obj->unk60 == 0) {
			obj->unk60 = p[9];
		}
		if (p[8] != 0) {
			if (obj->data5F == g_vars.level + 1) { // music instrument must have been found to complete the level
				if (obj->flag_end_level == 0) {
					play_sound(SOUND_13);
					obj->flag_end_level = 1;
					if (_di == 0) {
						do_level_update_tile(obj->xpos16, obj->ypos16, p[2]);
					} else {
						do_level_update_tile(obj->xpos16, obj->ypos16 - 1, p[2]);
					}
					obj->special_anim = p[6];
					if (obj->special_anim == 22) {
						play_sound(SOUND_11);
					}
					if (obj->type != 0 && g_vars.two_players_flag) {
						obj37 = &g_vars.objects[36];
					}
					if (obj37->type != 100) {
						do_level_drop_grabbed_object(obj);
					}
					g_vars.triggers_counter = 0;
				}

			} else if (g_vars.triggers_counter == 0) {
				g_vars.triggers_counter = 1;

			}
		} else {
			if (_di == 0) {
				do_level_update_tile(obj->xpos16, obj->ypos16, p[2]);
			} else {
				do_level_update_tile(obj->xpos16, obj->ypos16 - 1, p[2]);
			}
			if (p[3] != 0) {
				play_sound(SOUND_6);
			}
			obj->vinyls_count += p[3];
			obj->vinyls_count -= p[4];
			if (obj->vinyls_count < 0) {
				obj->vinyls_count = 0;
			}
			if (p[5] != 0) {
				play_sound(SOUND_12);
				if (!g_vars.two_players_flag && obj->data51 < 5) {
					++obj->data51;
				}
				if (g_vars.two_players_flag && obj->data51 < 3) {
					++obj->data51;
				}
				do_level_update_panel_lifes(obj);
			}
			obj->special_anim = p[6];
			if (obj->special_anim == 21) {
				play_sound(SOUND_8);
			}
			if (obj->data5F == 0 && p[7] != 0) { // found music instrument
				if (!g_vars.two_players_flag) {
					obj->data5F = p[7];
				} else {
					g_vars.objects[OBJECT_NUM_PLAYER1].data5F = p[7];
					g_vars.objects[OBJECT_NUM_PLAYER2].data5F = p[7];
				}
				screen_draw_frame(g_res.spr_frames[140 + g_vars.level], 12, 16, 80 + g_vars.level * 32, -12);
				g_vars.screen_draw_offset ^= 0x2000;
				screen_draw_frame(g_res.spr_frames[140 + g_vars.level], 12, 16, 80 + g_vars.level * 32, -12);
				g_vars.screen_draw_offset ^= 0x2000;
				g_vars.found_music_instrument_flag = 1;
				g_vars.triggers_counter = 0;
				play_sound(SOUND_12);
			}
		}
	} else if (p[1] == 12) {
		if (obj->type != 0 && g_vars.two_players_flag) {
			obj35 = &g_vars.objects[34];
		}
		obj35->type = 2;
		obj35->visible_flag = 1;
		obj35->facing_left = 0;
		obj35->grab_type = p[9];
		if (obj->facing_left != 0) {
			obj35->xpos = obj->xpos;
			obj35->screen_xpos = obj->screen_xpos;
		} else {
			obj35->xpos = obj->xpos;
			obj35->screen_xpos = obj->screen_xpos;
		}
		obj35->ypos = obj->ypos - 18;
		obj35->screen_ypos = obj->screen_ypos - 18;
		obj35->xpos16 = obj->xpos16;
		obj35->ypos16 = obj->ypos16;
		obj35->anim_num = 1;
		obj35->animframes_ptr = animframes_059d + obj35->type * 116 / 4;
// seg003:12A3
		if (_di == 0) {
			do_level_update_tile(obj->xpos16, obj->ypos16, p[2]);
		} else {
			do_level_update_tile(obj->xpos16, obj->ypos16 - 1, p[2]);
		}
		if (p[3] != 0) {
			play_sound(SOUND_7);
			obj->vinyls_count += p[3];
		} else if (p[4] != 0) {
			play_sound(SOUND_8);
			obj->vinyls_count -= p[4];
		}
		if (obj->vinyls_count < 0) {
			obj->vinyls_count = 0;
		}
		if (p[5] != 0) {
			play_sound(SOUND_12);
			if (!g_vars.two_players_flag && obj->data51 < 5) {
				++obj->data51;
			}
			do_level_update_panel_lifes(obj);
		}
		obj->special_anim = p[6];
		if (p[10] != 0) {
			play_sound(SOUND_12);
			++obj->lifes_count;
			if (!g_vars.two_players_flag) {
				screen_draw_number(obj->lifes_count - 1, 64, 163, 2);
				g_vars.screen_draw_offset ^= 0x2000;
				screen_draw_number(obj->lifes_count - 1, 64, 163, 2);
				g_vars.screen_draw_offset ^= 0x2000;
			} else if (obj->type == 0) {
				screen_draw_number(obj->lifes_count - 1, 48, 163, 2);
				g_vars.screen_draw_offset ^= 0x2000;
				screen_draw_number(obj->lifes_count - 1, 48, 163, 2);
				g_vars.screen_draw_offset ^= 0x2000;
			} else {
				screen_draw_number(obj->lifes_count - 1, 216, 163, 2);
				g_vars.screen_draw_offset ^= 0x2000;
				screen_draw_number(obj->lifes_count - 1, 216, 163, 2);
				g_vars.screen_draw_offset ^= 0x2000;
			}
		}
	} else {
		obj->trigger3_num = p[0];
	}
}

int16_t triggers_get_dy(struct object_t *obj) {
	const uint8_t *p = lookup_sql(obj->xpos16, obj->ypos16);
	const int num = p[0];
	return g_res.triggers[num].op_table1[obj->xpos & 15] - 1;
}

void triggers_update_tiles2(struct object_t *obj) {
	int offset = 2;
	int _si = offset;
	const uint8_t *p = obj->trigger3;
	if (p[1] < 10) {
		const int num = obj->trigger3_num - 1;
		_si += (p[1] << 2) * num;
		while (offset < (p[1] << 2)) {
			do_level_update_tile(p[_si], p[_si + 1], p[_si + 2]);
			offset += 4;
			_si += 4;
		}
	} else if (!(g_options.cheats & CHEATS_NO_HIT)) {
		obj->yfriction = p[_si];
		obj->yvelocity = p[_si + 1];
		obj->special_anim = p[_si + 2];
		if (obj->special_anim == 22) {
			play_sound(SOUND_11);
		}
		obj->anim_num = 1;
		obj->unk2F = 0;
		obj->xvelocity = p[3] - 100;
		obj->xmaxvelocity = ABS(obj->xvelocity);
		do_level_player_hit(obj);
	}
	obj->trigger3_num = 0;
}


/* objects update and logic  */

#include "game.h"
#include "util.h"

static void object_func_op0(struct object_t *obj) {
	if (triggers_get_tile_type(obj->xpos16 + 2 - obj->facing_left * 4, obj->ypos16) == 1) {
		obj->moving_direction ^= 1;
	}
	obj->direction_lr = obj->moving_direction + 1;
	obj->xmaxvelocity = 40;
	obj->special_anim = 1;
}

static void object_func_op1(struct object_t *obj) {
	if (triggers_get_tile_type(obj->xpos16 + 2 - obj->facing_left * 4, obj->ypos16) == 1) {
		obj->moving_direction ^= 1;
		obj->xvelocity = 0;
	}
	obj->direction_lr = obj->moving_direction + 1;
	obj->xmaxvelocity = 40;
	if (obj->player_ydist <= 10 && obj->player_xdist > -26 && obj->player_xdist < 26) {
		if (g_vars.objects[OBJECT_NUM_PLAYER1].special_anim != 18 && g_vars.objects[OBJECT_NUM_PLAYER2].special_anim != 18) {
			if (obj->player_ydist > -20) {
				obj->special_anim = 2;
				obj->direction_lr = 0;
			} else {
				obj->special_anim = 1;
			}
			if (obj->player_ydist < -50) {
				if (obj->player_xdist < 0) {
					obj->moving_direction = 1;
					obj->facing_left = 1;
				} else {
					obj->moving_direction = 0;
					obj->facing_left = 0;
				}
			}
		}
	} else if (obj->player_ydist < 0 && obj->player_ydist > -50 && obj->player_xdist > -60 && obj->player_xdist < 60) {
		obj->special_anim = 1;
	}
}

// police
static void object_func_op2(struct object_t *obj) {
	if (triggers_get_tile_type(obj->xpos16 + 2 - obj->facing_left * 4, obj->ypos16) == 1) {
		obj->moving_direction ^= 1;
	}
	obj->direction_lr = obj->moving_direction + 1;
	obj->xmaxvelocity = 40;
	if (obj->anim_num == 12 && obj->special_anim == 2) {
		obj->special_anim = 3;
		obj->anim_num = 1;
	} else if (obj->anim_num == 12 && obj->special_anim == 3) {
		obj->special_anim = 0;
	}
	if (obj->player_ydist <= 0 && obj->player_xdist > -(TILEMAP_SCREEN_W / 4) && obj->player_xdist < (TILEMAP_SCREEN_W / 4) && obj->direction_ud > 196) {
		if (obj->special_anim == 0 || obj->anim_num == 12) {
			obj->anim_num = 1;
			obj->unk42 = 0;
			do_level_update_object38(obj);
		}
		obj->facing_left = (obj->player_xdist < 0) ? 1 : 0;
	}
	if (obj->special_anim != 0) {
		obj->direction_lr = 0;
	}
}

static void object_func_op3(struct object_t *obj) {
	if (obj->player_ydist <= 20 && obj->player_ydist > -(TILEMAP_SCREEN_W / 2) && obj->player_xdist < (TILEMAP_SCREEN_W / 2) && g_vars.objects[38].type == 100) {
		if (obj->direction_ud > 226) {
			if (obj->special_anim == 0 || obj->anim_num == 16) {
				do_level_update_object38(obj);
			}
			obj->facing_left = (obj->player_xdist < 0) ? 1 : 0;
		}
	} else {
		if (obj->anim_num == 16) {
			obj->special_anim = 0;
		}
	}
	if (obj->special_anim != 0) {
		obj->direction_lr = 0;
	}
}

// elevator
static void object_func_op4(struct object_t *obj) {
	obj->unk2B = 0;
	obj->unk1E = 0;
	obj->yacc = 0;
	if (obj->elevator_direction == 1) {
		if (obj->moving_direction < 40) {
			++obj->moving_direction;
			obj->yvelocity = 0;
			obj->special_anim = 1;
		} else {
			if (triggers_get_tile_type(obj->xpos16, obj->ypos16 + 1) == 10) {
				obj->elevator_direction = -1;
				obj->moving_direction = 1;
			}
			obj->yvelocity = 2;
			obj->special_anim = 2;
		}
	}
	if (obj->elevator_direction == -1) {
		if (obj->moving_direction < 40) {
			++obj->moving_direction;
			obj->yvelocity = 0;
			obj->special_anim = 1;
		} else {
			if (triggers_get_tile_type(obj->xpos16, obj->ypos16 - 4) == 10) {
				obj->elevator_direction = 1;
				obj->moving_direction = 1;
			}
			obj->yvelocity = -2;
			obj->special_anim = 2;
		}
	}
}

static void object_func_op5(struct object_t *obj) {
	if (triggers_get_tile_type(obj->xpos16 + 4 - obj->facing_left * 8, obj->ypos16) == 12) {
		obj->moving_direction ^= 1;
	}
	obj->direction_lr = obj->moving_direction + 1;
	switch (obj->data5F) {
	case 0:
		obj->direction_lr |= 2;
		obj->yvelocity = obj->data51 >> 3;
		break;
	case 1:
		obj->direction_lr |= 1;
		obj->yvelocity = (20 - obj->data51) >> 3;
		break;
	case 2:
		obj->direction_lr |= 1;
		obj->yvelocity = -(obj->data51 >> 3);
		break;
	case 3:
		obj->direction_lr |= 2;
		obj->yvelocity = (obj->data51 - 20) >> 3;
		break;
	}
	if (obj->data51 > 20) {
		obj->data51 = 0;
		obj->data5F = (obj->data5F + 1) & 3;
	}
	obj->yacc = obj->yvelocity;
	obj->special_anim = 1;
	obj->xmaxvelocity = 32;
}

static void object_func_op8(struct object_t *obj) {
	if (triggers_get_tile_type(obj->xpos16 + 2 - obj->facing_left * 4, obj->ypos16) == 1) {
		obj->moving_direction ^= 1;
	}
	obj->direction_lr = obj->moving_direction + 1;
	obj->xmaxvelocity = 48;
	obj->yacc = 0;
	obj->yvelocity = 0;
	if (obj->player_ydist <= 100 && obj->player_xdist > -90 && obj->player_xdist < 90 && obj->direction_ud > 196 && obj->special_anim == 0) {
		obj->anim_num = 1;
		obj->unk42 = 1;
		do_level_update_object38(obj);
	}
}

static void object_func_op10(struct object_t *obj) {
	if (triggers_get_tile_type(obj->xpos16 + 2 - obj->facing_left * 4, obj->ypos16) == 1) {
		obj->moving_direction ^= 1;
	}
	obj->direction_lr = obj->moving_direction + 1;
	obj->xmaxvelocity = 32;
	if (obj->player_ydist <= 10 && obj->player_xdist > -(TILEMAP_SCREEN_W / 2) && obj->player_xdist < (TILEMAP_SCREEN_W / 2) && g_vars.objects[38].type == 100) {
		if (obj->direction_ud > 226) {
			if (obj->special_anim == 0 || obj->anim_num == 16) {
				obj->anim_num = 1;
				obj->unk42 = 0;
				do_level_update_object38(obj);
			}
			obj->facing_left = (obj->player_xdist < 0) ? 1 : 0;
		}
	} else if (obj->anim_num == 16) {
		obj->special_anim = 0;
	}
	if (obj->special_anim != 0) {
		obj->direction_lr = 0;
	}
}

static void object_func_op11(struct object_t *obj) {
	extern uint8_t level_tiledata_1dbf[];
	extern uint8_t level_tiledata_1dc7[];
	// extern uint8_t level_tiledata_1dc0[];
	// extern uint8_t level_tiledata_1dc8[];
	obj->unk2B = 0;
	obj->unk1E = 0;
	obj->yacc = 0;
	if (obj->elevator_direction == 1) {
		if (obj->moving_direction < 40) {
			++obj->moving_direction;
			obj->yvelocity = 0;
			obj->special_anim = 1;
			level_tiledata_1dbf[0] = 1;
			level_tiledata_1dc7[0] = 1;
			// level_tiledata_1dc0[0] = 1;
			// level_tiledata_1dc8[0] = 1;
		} else {
			// level_tiledata_1dc0[0] = 6;
			// level_tiledata_1dc8[0] = 6;
			if (triggers_get_tile_type(obj->xpos16, obj->ypos16 + 2) == 10) {
				obj->elevator_direction = -1;
				obj->moving_direction = 1;
			}
			obj->yvelocity = 4;
			obj->special_anim = 2;
			if (((obj->ypos - 8) & 15) == 0) {
				do_level_update_tile(obj->xpos16, obj->ypos16 - 1, triggers_get_next_tile_num(obj->xpos16, obj->ypos16 - 2));
				do_level_update_tile(obj->xpos16 - 1, obj->ypos16 - 1, triggers_get_next_tile_num(obj->xpos16 - 1, obj->ypos16 - 2));
			}
		}
	}
	if (obj->elevator_direction == -1) {
		if (obj->moving_direction < 40) {
			++obj->moving_direction;
			obj->yvelocity = 0;
			obj->special_anim = 1;
			level_tiledata_1dbf[0] = 1;
			level_tiledata_1dc7[0] = 1;
			// level_tiledata_1dc0[0] = 1;
			// level_tiledata_1dc8[0] = 1;
		} else {
			// level_tiledata_1dc0[0] = 6;
			// level_tiledata_1dc8[0] = 6;
			if (triggers_get_tile_type(obj->xpos16, obj->ypos16 - 4) == 10) {
				obj->elevator_direction = 1;
				obj->moving_direction = 1;
			}
			obj->yvelocity = -2;
			obj->special_anim = 2;
			if (((obj->ypos - 8) & 15) == 0) {
				do_level_update_tile(obj->xpos16, obj->ypos16 - 1, triggers_get_next_tile_num(obj->xpos16 + 1, obj->ypos16 - 1));
				do_level_update_tile(obj->xpos16 - 1, obj->ypos16 - 1, triggers_get_next_tile_num(obj->xpos16 + 2, obj->ypos16 - 1));
			}
		}
	}
}

static void object_func_op12(struct object_t *obj) {
	if (triggers_get_tile_type(obj->xpos16 + 2 - obj->facing_left * 4, obj->ypos16) == 1) {
		obj->moving_direction ^= 1;
	}
	obj->direction_lr = obj->moving_direction + 1;
	obj->xmaxvelocity = 64;
	if (obj->sprite_type != 0) {
		obj->yfriction = 8;
	}
	obj->special_anim = 1;
}

static void object_func_op13(struct object_t *obj) {
	obj->direction_lr = 0;
	obj->xmaxvelocity = 0;
	obj->special_anim = 1;
}

static void object_func_op14(struct object_t *obj) {
	obj->special_anim = 2;
	// sub_1AD3B(obj->xpos, level_ypos_egou[obj->unk5D] - _screen_tilemap_yorigin, obj->xpos, obj->ypos - 5, 3);
	if (obj->elevator_direction == 1) {
		if (obj->moving_direction < 25) {
			++obj->moving_direction;
			obj->yvelocity = 0;
		} else if (triggers_get_tile_type(obj->xpos16, obj->ypos16 + 1) != 0) {
			obj->elevator_direction = -1;
			obj->moving_direction = 1;
			obj->yacc = 0;
			obj->yvelocity = 0;
		} else {
			obj->moving_direction = 1;
			obj->yacc = 2;
			obj->yvelocity = 2;
		}
	}
	if (obj->elevator_direction == -1) {
		if (obj->moving_direction < 10) {
			++obj->moving_direction;
			obj->yvelocity = 0;
		} else if (obj->ypos <= level_ypos_egou[obj->unk5D]) {
			obj->elevator_direction = 1;
			obj->moving_direction = 1;
			obj->yacc = 0;
			obj->yvelocity = 0;
		} else {
			obj->moving_direction = 1;
			obj->yacc = -2;
			obj->yvelocity = -2;
		}
	}
}

static void object_func_op15(struct object_t *obj) {
	if (triggers_get_tile_type(obj->xpos16 + 4 - obj->facing_left * 8, obj->ypos16) == 12) {
		obj->moving_direction ^= 1;
	}
	obj->direction_lr = obj->moving_direction + 1;
	obj->yacc = obj->yvelocity;
	obj->special_anim = 1;
	obj->xmaxvelocity = 32;
}

static void object_func_op16(struct object_t *obj) {
	if (triggers_get_tile_type(obj->xpos16 + 2 - obj->facing_left * 4, obj->ypos16) == 1) {
		obj->moving_direction ^= 1;
	}
	obj->direction_lr = obj->moving_direction + 1;
	obj->xmaxvelocity = 64;
	obj->special_anim = 1;
	if (obj->tile0_flags != 0 && obj->sprite_type != 0) {
		obj->yfriction = 6;
	}
}

static void object_func_op17(struct object_t *obj) {
	obj->unk2B = 0;
	obj->unk1E = 0;
	obj->yacc = 0;
	if (obj->elevator_direction == 1) {
		if (obj->moving_direction < 40) {
			++obj->moving_direction;
			obj->yvelocity = 0;
			obj->special_anim = 1;
		} else {
			if (triggers_get_tile_type(obj->xpos16, obj->ypos16 + 1) == 1) {
				obj->elevator_direction = -1;
				obj->moving_direction = 1;
			}
			obj->yvelocity = 2;
			obj->special_anim = 2;
			if (((obj->ypos - 12) & 15) == 0) {
				do_level_update_tile(obj->xpos16, obj->ypos16, triggers_get_next_tile_num(obj->xpos16, obj->ypos16 - 2));
				do_level_update_tile(obj->xpos16 - 1, obj->ypos16, triggers_get_next_tile_num(obj->xpos16 - 1, obj->ypos16 - 2));
			}
		}
	}
	if (obj->elevator_direction == -1) {
		if (obj->moving_direction < 40) {
			++obj->moving_direction;
			obj->yvelocity = 0;
			obj->special_anim = 1;
		} else {
			if (triggers_get_tile_type(obj->xpos16, obj->ypos16 - 3) == 1) {
				obj->elevator_direction = 1;
				obj->moving_direction = 1;
			}
			obj->yvelocity = -2;
			obj->special_anim = 2;
			if ((obj->ypos & 15) == 0) {
				do_level_update_tile(obj->xpos16, obj->ypos16 - 1, triggers_get_next_tile_num(obj->xpos16, obj->ypos16));
				do_level_update_tile(obj->xpos16 - 1, obj->ypos16 - 1, triggers_get_next_tile_num(obj->xpos16 - 1, obj->ypos16));
			}
		}
	}
}

static void object_func_op18(struct object_t *obj) {
	if (triggers_get_tile_type(obj->xpos16 + 2 - obj->facing_left * 4, obj->ypos16) == 1) {
		obj->moving_direction ^= 1;
	}
	obj->direction_lr = obj->moving_direction + 1;
	obj->xmaxvelocity = 40;
	obj->special_anim = 1;
	if (obj->sprite_type != 0) {
		obj->yfriction = 2;
	}
}

static void object_func_op19(struct object_t *obj) {
	obj->unk2B = 0;
	obj->unk1E = 0;
	obj->yacc = 0;
	if (obj->elevator_direction == 1) {
		if (obj->moving_direction < 40) {
			++obj->moving_direction;
			obj->yvelocity = 0;
			obj->special_anim = 1;
		} else {
			if (triggers_get_tile_type(obj->xpos16, obj->ypos16 + 1) != 0) {
				obj->elevator_direction = -1;
				obj->moving_direction = 1;
			}
			obj->yvelocity = 2;
			obj->special_anim = 2;
		}
	}
	if (obj->elevator_direction == -1) {
		if (obj->moving_direction < 40) {
			++obj->moving_direction;
			obj->yvelocity = 0;
			obj->special_anim = 1;
		} else {
			if (triggers_get_tile_type(obj->xpos16, obj->ypos16 - 5) != 0) {
				obj->elevator_direction = 1;
				obj->moving_direction = 1;
			}
			obj->yvelocity = -2;
			obj->special_anim = 2;
		}
	}
}

void level_call_object_func(struct object_t *obj) {
	if (obj->collide_flag == 0) {
		switch (obj->op - 1) {
		case 0:
			object_func_op0(obj);
			break;
		case 1:
		case 9:
			object_func_op1(obj);
			break;
		case 2:
			object_func_op2(obj);
			break;
		case 3:
			object_func_op3(obj);
			break;
		case 4:
			object_func_op4(obj);
			break;
		case 5:
			object_func_op5(obj);
			break;
		case 8:
			object_func_op8(obj);
			break;
		case 10:
			object_func_op10(obj);
			break;
		case 11:
			object_func_op11(obj);
			break;
		case 12:
			object_func_op12(obj);
			break;
		case 13:
			object_func_op13(obj);
			break;
		case 14:
			object_func_op14(obj);
			break;
		case 15:
			object_func_op15(obj);
			break;
		case 16:
			object_func_op16(obj);
			break;
		case 17:
			object_func_op17(obj);
			break;
		case 18:
			object_func_op18(obj);
			break;
		case 19:
			object_func_op19(obj);
			break;
		default:
			// print_warning("level_call_object_func: unimplemented opcode %d", obj->op - 1);
			obj->special_anim = 1;
			break;
		}
	}
}

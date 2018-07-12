
#ifndef GAME_H__
#define GAME_H__

#include "intern.h"

#define GAME_SCREEN_W 320
#define GAME_SCREEN_H 200

#define TILEMAP_OFFSET_Y  14
#define TILEMAP_SCREEN_W 320
#define TILEMAP_SCREEN_H 160

#define PLAYER_JAKE   0
#define PLAYER_ELWOOD 1

#define MAX_DOORS   30
#define MAX_LEVELS   6
#define MAX_OBJECTS 41

#define SOUND_0 0
#define SOUND_2 2
#define SOUND_3 3
#define SOUND_4 4
#define SOUND_5 5
#define SOUND_6 6
#define SOUND_7 7
#define SOUND_8 8
#define SOUND_11 11
#define SOUND_12 12
#define SOUND_13 13
#define SOUND_14 14
#define SOUND_15 15
#define SOUND_16 16

#define CHEATS_NO_HIT (1 << 0)

struct options_t {
	uint32_t cheats;
	int start_level;
	int start_xpos16;
	int start_ypos16;
	bool amiga_copper_bars;
	bool amiga_colors;
	bool amiga_sprites;
	bool amiga_lbms;
	bool amiga_data;
};

extern struct options_t g_options;

struct door_t {
	uint16_t src_x16, src_y16;
	uint16_t dst_x16, dst_y16;
};

#define OBJECT_DIRECTION_RIGHT 1
#define OBJECT_DIRECTION_LEFT  2

#define OBJECT_DIRECTION_UP 1

#define OBJECT_NUM_PLAYER1 39
#define OBJECT_NUM_PLAYER2 40

#define OBJECT_ANIM_DEAD 18

#define OBJECT_GRAB_CRATE    0
#define OBJECT_GRAB_BALLOON  2
#define OBJECT_GRAB_UMBRELLA 8

struct object_t {
	uint8_t type;
	uint8_t anim_frame;
	uint8_t anim_num;
	const uint8_t **animframes_ptr;
	uint8_t facing_left;
	int16_t xvelocity;
	int16_t yvelocity;
	int16_t xmaxvelocity;
	int16_t ymaxvelocity;
	int16_t screen_xpos;
	int16_t xpos;
	int16_t screen_ypos;
	int16_t ypos;
	int16_t xacc;
	int16_t yacc;
	int16_t unk1C; // xvelocity / 8
	int16_t unk1E; // yvelocity / 8
	uint8_t direction_lr;
	int8_t direction_ud;
	int16_t xpos16; // tilemap_xpos
	int16_t ypos16; // tilemap_ypos
	// int16_t unk26;
	int16_t floor_ypos16;
	uint8_t yfriction;
	uint8_t unk2B;
	uint8_t sprite_type;
	uint8_t unk2D; // xfriction
	int16_t unk2E;
	int8_t unk2F;
	uint8_t op;
	uint8_t grab_state; // 1:carry_crate
	uint8_t grab_type;
	uint8_t special_anim; // 2:flying
	// uint8_t unk35;
	int16_t player_xdist;
	int16_t player_ydist;
	uint8_t sprite3_counter;
	uint8_t visible_flag;
	uint8_t moving_direction; // 0:right 1:left
	uint8_t unk3D; // 1:umbrella 2:balloon
	uint8_t carry_crate_flag;
	// int16_t unk3F;
	// uint8_t unk41;
	int16_t unk42;
	uint8_t tile0_flags;
	uint8_t tile1_flags;
	uint8_t tile2_flags;
	int16_t tile012_xpos;
	int8_t elevator_direction; // -1,1
	const uint8_t *trigger3;
	uint8_t trigger3_num;
	// int16_t unk4E;
	int16_t unk50;
	uint8_t data51; // health for obj39/40, horizontal direction for other objects
	uint8_t unk53;
	uint8_t unk54;
	uint8_t unk55;
	uint8_t unk56;
	int16_t vinyls_count;
	uint8_t collide_flag;
	int16_t unk5A; // always zero
	uint8_t unk5C;
	uint8_t unk5D;
	uint8_t blinking_counter;
	uint8_t data5F; // music instrument number for obj39/40, counter for other objects
	uint8_t unk60;
	uint8_t lifes_count;
	uint8_t flag_end_level; // level flag, set if music instrument was found
	uint8_t unk63; // restart_level_flag
};

struct vars_t {
	int level;
	int player;
	int start_level;
	int16_t level_xpos[MAX_OBJECTS];
	int16_t level_ypos[MAX_OBJECTS];
	int screen_w, screen_h;
	int screen_tilemap_w, screen_tilemap_h;
	int screen_tilemap_w16, screen_tilemap_h16;
	int screen_tilemap_xorigin, screen_tilemap_yorigin;
	int screen_tilemap_xoffset, screen_tilemap_yoffset;
	int screen_tilemap_size_w, screen_tilemap_size_h;
	int screen_scrolling_dirmask, screen_unk1;
	uint16_t screen_draw_offset;
	uint16_t screen_tile_lut[256];
	int level_completed_flag;
	int play_level_flag;
	int game_over_flag;
	int found_music_instrument_flag;
	int player2_dead_flag;
	int player1_dead_flag;
	int player2_scrolling_flag;
	int play_demo_flag;
	int quit_level_flag;
	int music_num;
	uint8_t inp_keyboard[256];
	uint8_t inp_code;
	int inp_key_space;
	int inp_key_tab;
	int inp_key_up;
	int inp_key_down;
	int inp_key_right;
	int inp_key_left;
	int inp_key_up_prev;
	int inp_key_down_prev;
	int inp_key_action;
	struct door_t doors[MAX_DOORS];
	int cheat_code_len;
	int cheat_flag;
	struct object_t objects[MAX_OBJECTS];
	int two_players_flag;
	int vinyls_count;
	uint16_t level_loop_counter;
	int triggers_counter;
	int update_objects_counter;
};

extern struct vars_t g_vars;

/* staticres.c */
extern const uint8_t animframe_00dd[];
extern const uint8_t animframe_0135[];
extern const uint8_t animframe_01d5[];
extern const uint8_t animframe_022d[];
extern const int16_t level_xpos_magasin[];
extern const int16_t level_ypos_magasin[];
extern const int16_t level_xpos_concert[];
extern const int16_t level_ypos_concert[];
extern const int16_t level_xpos_ville[];
extern const int16_t level_ypos_ville[];
extern const int16_t level_xpos_egou[];
extern const int16_t level_ypos_egou[];
extern const int16_t level_xpos_prison[];
extern const int16_t level_ypos_prison[];
extern const int16_t level_xpos_ent[];
extern const int16_t level_ypos_ent[];
extern const int16_t level_dim[];
extern const uint8_t level_door[];
extern const uint16_t level_tilemap_start_xpos[];
extern const uint16_t level_tilemap_start_ypos[];
extern const uint8_t level_objtypes[];
extern uint8_t level_data[];
extern const uint8_t *animframes_059d[];
extern uint16_t level_obj_w[MAX_OBJECTS];
extern uint16_t level_obj_h[MAX_OBJECTS];
extern const uint16_t level_obj_type[MAX_OBJECTS];
extern uint8_t *level_tilesdata_1e8c[];

/* game.c */
extern void	update_input();
extern void	game_main();
extern void	do_title_screen();
extern void	do_select_player();

/* level.c */
extern void	load_level_data(int num);
extern void	do_level_update_tile(int x, int y, int num);
extern void	do_level_update_panel_lifes(struct object_t *);
extern void	do_level_enter_door(struct object_t *);
extern void	do_level_player_hit(struct object_t *);
extern void	do_level_drop_grabbed_object(struct object_t *);
extern void	do_level_update_projectile(struct object_t *obj);
extern void	do_level();

/* opcodes.c */
extern void	level_call_object_func(struct object_t *);

/* screen.c */
extern void	screen_init();
extern void	screen_clear_sprites(); // _sprites_count = 0
extern void	screen_add_sprite(int x, int y, int frame);
extern void	screen_clear_last_sprite();
extern void	screen_redraw_sprites();
extern void	fade_in_palette();
extern void	fade_out_palette();
extern void	screen_adjust_palette_color(int color, int b, int c);
extern void	screen_vsync();
extern void	screen_draw_frame(const uint8_t *frame, int a, int b, int c, int d);
extern void	screen_flip();
extern void	screen_unk4();
extern void	screen_unk5();
extern void	screen_unk6();
extern void	screen_copy_tilemap2(int a, int b, int c, int d);
extern void	screen_copy_tilemap(int a);
extern void	screen_do_transition1(int a);
extern void	screen_clear(int a);
extern void	screen_draw_tile(int tile, int dst, int type);
extern void	screen_do_transition2();
extern void	screen_draw_number(int num, int x, int y, int color);
extern void	screen_add_game_sprite1(int x, int y, int frame);
extern void	screen_add_game_sprite2(int x, int y, int frame);
extern void	screen_add_game_sprite3(int x, int y, int frame, int blinking_counter);
extern void	screen_add_game_sprite4(int x, int y, int frame, int blinking_counter);
extern void	screen_load_graphics();

/* sound.c */
extern void	sound_init(int rate);
extern void	sound_fini();
extern void	play_sound(int num);
extern void	play_music(int num);

/* triggers.c */
extern uint16_t	triggers_get_tile_type(int x, int y);
extern uint16_t	triggers_get_next_tile_flags(int x, int y);
extern uint16_t	triggers_get_tile_data(struct object_t *obj);
extern uint16_t	triggers_get_next_tile_num(int x, int y);
extern void	level_call_trigger_func(struct object_t *obj, int y);
extern void	triggers_update_tiles1(struct object_t *obj);
extern int16_t	triggers_get_dy(struct object_t *obj);
extern void	triggers_update_tiles2(struct object_t *obj);

#endif /* GAME_H__ */


#ifndef GAME_H__
#define GAME_H__

#include "intern.h"

extern struct options_t g_options;

#define PANEL_H 24

#define TILEMAP_SCREEN_W  GAME_SCREEN_W
#define TILEMAP_SCREEN_H (GAME_SCREEN_H - PANEL_H)
#define TILEMAP_SCROLL_W  64

#define CHEATS_NO_HIT           (1 << 0)
#define CHEATS_UNLIMITED_LIFES  (1 << 1)
#define CHEATS_UNLIMITED_ENERGY (1 << 2)

struct club_anim_t {
	const uint8_t *anim; /* uint16_t[4] : player spr, club spr, x, y */
	uint8_t a;
	uint8_t power; /* damage */
	uint8_t c;
};

struct player_t {
	int16_t hdir; /* left:-1 right:1 */
	uint8_t current_anim_num;
	const uint8_t *anim;
	int16_t y_velocity;
	uint8_t special_anim_num; /* idle, exhausted */
};

struct club_projectile_t {
	const uint8_t *anim;
	int16_t y_velocity;
};

struct monster_t {
	uint8_t flags;
	void *ref;
	int16_t x_velocity;
	int16_t y_velocity;
	const uint8_t *anim;
	uint8_t state;
	int8_t energy;
	uint8_t hit_jump_counter;
};

struct thing_t {
	void *ref;
	int16_t counter;
	int16_t y_velocity;
};

struct object_t {
	int16_t x_pos, y_pos;
	uint16_t spr_num;
	int16_t x_velocity;
	uint8_t x_friction;
	union {
		struct player_t p; /* objects[1] */
		struct club_projectile_t c; /* objects[2..5] */
		struct monster_t m; /* objects[11..22] */
		struct thing_t t; /* objects[23..74] */
	} data;
	uint8_t hit_counter;
};

#define MONSTERS_COUNT 12
#define OBJECTS_COUNT 116
/*
 offset count
      0     1 : club
      1     1 : player
      2     4 : axe
      6     5 : club hitting decor frames
     11    12 : monsters
     23    32 : secret bonuses
     55    20 : items
     75    16 : scores
     91     7 : decor
     98     5 : boss level 5 (tree)
    103     5 : boss projectiles
    108     8 : boss energy bars
*/

struct boss_level5_leaf_t {
	int16_t x_pos;
	int16_t y_pos;
	uint16_t spr_num;
	int16_t y_delta;
	int16_t x_delta;
	int8_t dir;
};

struct orb_t {
	uint16_t x_pos;
	uint16_t y_pos;
	uint8_t index_tbl; /* cos_/sin_tbl */
	uint8_t radius;
};

struct fly_t {
	uint16_t x_pos;
	uint16_t y_pos;
	int8_t x_delta;
	int8_t y_delta;
	uint8_t unk6;
	uint8_t unk7;
};

struct vars_t {
	uint32_t starttime;
	uint32_t timestamp;
	struct {
		uint8_t a, b, c;
		uint16_t d, e;
	} random;
	struct {
		bool keystate[128];
		uint8_t key_left, key_right, key_down, key_up, key_space;
		uint8_t key_vdir, key_hdir;
	} input;

	uint8_t level_num;
	uint32_t score;
	uint16_t score_extra_life;

	uint16_t level_complete_secrets_count;
	uint16_t level_complete_bonuses_count;
	uint16_t level_current_secrets_count;
	uint16_t level_current_bonuses_count;

	uint8_t level_completed_flag;
	uint8_t restart_level_flag;

	uint16_t level_draw_counter;

	uint8_t shake_screen_counter;
	uint16_t shake_screen_voffset;

	uint8_t player_lifes;
	int8_t player_energy;
	uint8_t player_death_flag;
	uint8_t player_flying_flag;
	uint8_t player_flying_counter;
	uint8_t player_flying_anim_index;
	uint8_t player_bonus_letters_mask;
	uint8_t player_utensils_mask;
	uint8_t player_gravity_flag; /* 0, 1 or 2 */
	uint8_t player_unk_counter1;
	uint8_t player_moving_counter;
	uint8_t player_anim_0x40_flag;
	uint8_t player_anim2_counter;
	int16_t player_prev_y_pos;
	uint8_t player_bonus_letters_blinking_counter;
	uint8_t player_nojump_counter;
	uint8_t player_jumping_counter;
	uint8_t player_action_counter;
	int16_t player_hit_monster_counter;
	uint8_t player_jump_monster_flag;
	uint8_t player_club_type;
	uint8_t player_club_power;
	uint8_t player_club_powerup_duration;
	uint8_t player_club_anim_duration;
	bool player_using_club_flag;
	uint16_t player_update_counter;
	uint8_t player_current_anim_type;
	uint8_t player_tile_flags;

	uint8_t level_items_count_tbl[140]; /* bonuses and items collected in the level */
	uint8_t level_items_total_count;
	uint8_t level_bonuses_count_tbl[80];
	uint8_t bonus_energy_counter;

	int16_t current_platform_dx, current_platform_dy;
	uint16_t decor_tile0_offset; /* decor tile below the player */

	struct fly_t fly_tbl[20];
	struct orb_t orb_tbl[20]; /* spider webs */
	struct object_t *current_hit_object;
	struct object_t objects_tbl[OBJECTS_COUNT];

	uint8_t level_xscroll_center_flag, level_yscroll_center_flag;
	uint8_t level_force_x_scroll_flag;
	bool tilemap_adjust_player_pos_flag;
	uint8_t tilemap_noscroll_flag;
	int16_t tilemap_yscroll_diff;
	uint16_t tilemap_start_x_pos, tilemap_start_y_pos; /* tilemap restart position */
	uint8_t tile_attr2_flags; /* current tilemap tile types (eg. front) */

	uint8_t level_animated_tiles_counter; /* animated tiles update counter */
	uint8_t *level_animated_tiles_current_tbl; /* pointer to current tile_tbl */
	uint8_t tile_tbl1[256]; /* animated tile state 1 */
	uint8_t tile_tbl2[256]; /* animated tile state 2 */
	uint8_t tile_tbl3[256]; /* animated tile state 3 */
	uint8_t animated_tile_flag_tbl[256]; /* 1 if tile is animated */

	uint8_t columns_tiles_buf[256];

	struct {
		int16_t x, y;
		int16_t prev_x, prev_y;
		int8_t scroll_dx, scroll_dy;
		uint8_t redraw_flag2; /* tilemap needs redraw */
		uint8_t redraw_flag1; /* force redraw even if tilemap origin did not change */
		uint8_t h;
		uint16_t size; /* tilemap size h*256 */
	} tilemap;
	struct {
		struct object_t *current_object;
		uint8_t type10_dist;
		uint8_t hit_mask;
		int16_t collide_y_dist;
		uint8_t type0_hdir;
	} monster;
	struct {
		uint16_t draw_counter;
		uint8_t change_counter;
		int16_t x_velocity, y_velocity;
		bool hdir; /* facing to the right */
		int16_t x_dist; /* horizontal distance from player */
		int16_t state_counter;
		uint8_t anim_num;
		const uint8_t *prev_anim;
		const uint8_t *next_anim;
		const uint8_t *current_anim;
		struct {
			int16_t x_pos, y_pos;
			uint16_t spr_num;
		} parts[5];
		struct object_t *obj1;
		struct object_t *obj2;
		struct object_t *obj3;
	} boss; /* gorilla */
	struct {
		uint8_t unk1;
		uint8_t energy;
		uint8_t state; /* 3: boss dead */
		uint8_t spr103_pos;
		uint8_t spr106_pos;
		uint8_t unk6;
		uint8_t tick_counter;
		uint8_t idle_counter;
		struct boss_level5_leaf_t leaf_tbl[5];
	} boss_level5; /* tree */
	struct {
		int16_t energy;
		uint8_t seq_counter;
		uint8_t hit_counter;
		const uint8_t *seq;
		const uint16_t *anim;
	} boss_level9; /* minotaur */
	struct {
		int16_t x_pos, y_pos;
		uint16_t spr_num;
	} current_bonus; /* bonus added */
	struct {
		uint16_t value;
		uint16_t counter;
		uint16_t random_tbl[256];
	} snow;
	struct {
		uint8_t state; /* 1: lights off, 0: lights on */
		uint8_t palette_flag1; /* palette day time */
		uint8_t palette_flag2; /* palette night time */
		uint8_t palette_counter;
	} light;
	struct {
		uint32_t score;
		uint8_t lifes;
		uint8_t energy;
		uint8_t bonus_letters_mask;
	} panel;
};

extern struct vars_t g_vars;

/* staticres.c */
extern const uint8_t *palettes_tbl[16];
extern const uint8_t credits_palette_data[16 * 3];
extern const uint8_t light_palette_data[16 * 3];
extern const uint8_t spr_offs_tbl[922];
extern const uint8_t spr_size_tbl[922];
extern const uint16_t score_tbl[17];
extern const uint8_t score_spr_lut[110];
extern const uint8_t *object_anim_tbl[];
extern const struct club_anim_t club_anim_tbl[4];
extern const uint8_t player_anim_lut[32];
extern const uint8_t player_anim_data[100];
extern const uint8_t vscroll_offsets_data[132];
extern const uint8_t cos_tbl[256];
extern const uint8_t sin_tbl[256];
extern const uint16_t monster_spr_tbl[48];
extern const uint8_t monster_anim_tbl[1100];
extern const uint8_t boss_minotaur_seq_data[86];
extern const uint16_t boss_gorilla_data[19 * 10];
extern const uint16_t boss_gorilla_spr_tbl[46 * 3]; /* uint16_t: spr1_num, uint16_t: spr2_num, uint8_t: dx, uint8_t: dy */

/* game.c */
extern void	update_input();
extern void	input_check_ctrl_alt_w();
extern uint32_t	timer_get_counter();
extern void	random_reset();
extern uint8_t	random_get_number();
extern uint16_t	random_get_number2();
extern uint16_t	random_get_number3(uint16_t x);
extern void	game_main();

/* level.c */
extern void	do_level();
extern uint8_t	level_get_tile(uint16_t offset);
extern void	level_player_die();
extern bool	level_objects_collide(const struct object_t *, const struct object_t *);
extern void	level_add_object23_bonus(int x_vel, int y_vel, int count);
extern void	level_add_bonuses_4x();

/* monsters.c */
extern void	monster_change_next_anim(struct object_t *obj);
extern void	monster_change_prev_anim(struct object_t *obj);

/* screen.c */
extern void	video_draw_string(int offset, int hspace, const char *s);
extern void	video_clear();
extern void	video_copy_img(const uint8_t *src);
extern void	video_draw_panel(const uint8_t *src);
extern void	video_draw_panel_number(int offset, int num);
extern void	video_draw_number(int offset, int num);
extern void	video_draw_character_spr(int offset, uint8_t chr);
extern void	video_draw_string2(int offset, const char *str);
extern void	video_draw_tile(const uint8_t *src, int x, int y);
extern void	video_convert_tiles(uint8_t *data, int len);
extern void	video_load_front_tiles();
extern void	video_transition_close();
extern void	video_transition_open();
extern void	video_load_sprites();
extern void	video_draw_sprite(int num, int x, int y, int flag);
extern void	video_put_pixel(int x, int y, uint8_t color);

/* sound.c */
extern void	sound_init();
extern void	sound_fini();
extern void	play_sound(int num);
extern void	play_music(int num);

#endif /* GAME_H__ */


#ifndef GAME_H__
#define GAME_H__

#include "intern.h"

#define CHEATS_OBJECT_NO_HIT    (1 << 0)
#define CHEATS_ONE_HIT_VINYL    (1 << 1)
#define CHEATS_DECOR_NO_HIT     (1 << 2)
#define CHEATS_UNLIMITED_VINYLS (1 << 3)
#define CHEATS_UNLIMITED_TIME   (1 << 4)

extern struct options_t g_options;

#define TILEMAP_SCREEN_W  GAME_SCREEN_W
#define TILEMAP_SCREEN_H (GAME_SCREEN_H - PANEL_H)

#define PANEL_H 24

#define SOUND_0 0
#define SOUND_1 1 /* monster knocked out */
#define SOUND_2 2 /* bouncing mushroom */
#define SOUND_3 3 /* decor hit */
#define SOUND_4 4 /* player hit */
#define SOUND_5 5 /* grab vinyl */
#define SOUND_8 8 /* no vinyl */
#define SOUND_9 9 /* throw vinyl */
#define SOUND_10 10 /* "rock,rock,rock and roll" */
#define SOUND_11 11
#define SOUND_14 14 /* player lost life */
#define SOUND_15 15 /* running out time */

struct object_t {
	int16_t x_pos;
	int16_t y_pos;
	uint16_t spr_num;
	union {
		uint8_t b[2];
		int16_t w;
		const uint8_t *p;
	} data[11];
};

enum {
	PLAYER_FLAGS_STAIRS = 2,
	PLAYER_FLAGS_FACING_LEFT = 4,
	PLAYER_FLAGS_THROW_VINYL = 8,
	PLAYER_FLAGS_POWERUP = 0x20,
	PLAYER_FLAGS_JAKE = 0x80
};

#define player_obj_num(obj)        (obj)->data[0].w
#define player_prev_spr_num(obj)   (obj)->data[1].w
#define player_anim_data(obj)      (obj)->data[2].p
#define player_idle_counter(obj)   (obj)->data[3].b[0]
#define player_power(obj)          (obj)->data[3].b[1]
#define player_x_delta(obj)        (obj)->data[4].w
#define player_y_delta(obj)        (obj)->data[5].w
#define player_flags(obj)          (obj)->data[6].b[0]
#define player_jump_counter(obj)   (obj)->data[6].b[1]
#define player_bounce_counter(obj) (obj)->data[7].b[0]
#define player_tilemap_offset(obj) (obj)->data[8].w
#define player_hit_counter(obj)    (obj)->data[9].b[0]
#define player_throw_counter(obj)  (obj)->data[9].b[1]
#define player_flags2(obj)         (obj)->data[10].b[0]

#define object_blinking_counter(obj) (obj)->data[6].b[1]

/* star */
#define object2_spr_count(obj)   (obj)->data[0].b[0]
#define object2_spr_tick(obj)    (obj)->data[0].b[1]

/* vinyl */
#define object22_xvelocity(obj)  (obj)->data[0].w
#define object22_damage(obj)     (obj)->data[1].w
#define object22_player_num(obj) (obj)->data[2].w

/* crate */
#define object64_counter(obj)    (obj)->data[1].w
#define object64_yvelocity(obj)  (obj)->data[2].w

/* monster */
#define object82_state(obj)      (obj)->data[0].b[0]
#define object82_type(obj)       (obj)->data[0].b[1]
#define object82_anim_data(obj)  (obj)->data[1].p
#define object82_hflip(obj)      (obj)->data[3].b[0]
#define object82_counter(obj)    (obj)->data[3].b[1]
#define object82_x_delta(obj)    (obj)->data[4].w
#define object82_y_delta(obj)    (obj)->data[5].w
#define object82_energy(obj)     (obj)->data[8].w
#define object82_type0_init_data(obj)  (obj)->data[4].p
#define object82_type0_player_num(obj) (obj)->data[5].w
#define object82_type0_x_delta(obj)    (obj)->data[6].b[0] // signed
#define object82_type0_y_delta(obj)    (obj)->data[7].b[0] // signed
#define object82_type1_hdir(obj)       (obj)->data[6].b[0] // signed

#define object102_x_delta(obj)   (obj)->data[0].w
#define object102_y_delta(obj)   (obj)->data[1].w

struct player_t {
	struct object_t obj;
	int16_t unk_counter;
	int16_t change_hdir_counter;
	uint8_t lifes_count;
	int16_t vinyls_count;
	int8_t energy;
	uint8_t dir_mask;
	uint8_t ticks_counter;
};

#define TRIGGERS_COUNT  36
#define LEVELS_COUNT    30
#define OBJECTS_COUNT  165
/*
 offset count
     2     20 : animated tiles/vinyls
    22     10 : vinyls
    32    8*4 : (rotating) platforms
    64      8 : crates
    72     10 : bonuses spr_num:190
    82     20 : monsters
   102     10
   112      9 : vertical bars
   121      8 : dragon monster (level 26)
*/

struct vars_t {
	int level;
	int player;
	bool input_keystate[128];
	uint32_t timestamp;
	uint8_t input_key_left, input_key_right, input_key_down, input_key_up, input_key_space, input_key_jump;
	uint16_t buffer[128 * 2]; /* level objects state 0xFFFF, 0xFF20 or g_vars.objects_table index */
	int16_t dragon_coords[1 + 128];
	struct player_t players_table[2];
	int16_t player_xscroll, player_map_offset;
	struct object_t objects_table[OBJECTS_COUNT];
	int level_start_1p_x_pos, level_start_1p_y_pos;
	int level_start_2p_player1_x_pos, level_start_2p_player1_y_pos, level_start_2p_player2_x_pos, level_start_2p_player2_y_pos;
	int level_loop_counter;
	int throw_vinyl_counter;
	uint8_t level_num;
	uint16_t level_time, level_time2;
	uint8_t level_time_counter;
	uint8_t triggers_table[19 + TRIGGERS_COUNT * 16];
	int tilemap_x, tilemap_y, tilemap_w, tilemap_h;
	int tilemap_end_x, tilemap_end_y;
	int tilemap_scroll_dx, tilemap_scroll_dy;
	uint8_t *tilemap_data;
	uint16_t level_pos_num;
	uint8_t tilemap_type, tilemap_flags;
	uint8_t tilemap_lut_type[0x100]; /* type:0,1,2 */
	uint8_t tilemap_lut_init[6 * 0x100];
	uint8_t tilemap_lut_init2[0x100];
	const uint8_t *tilemap_current_lut;
	uint8_t tile_palette_table[0x100];
	const uint8_t *level_tiles_lut;
	uint8_t palette_buffer[256 * 3];
	bool change_next_level_flag;
	bool reset_palette_flag;
};

extern struct vars_t g_vars;

/* staticres.c */
extern const uint16_t sound_offsets[];
extern const uint16_t sprite_offsets[];
extern const uint16_t sprite_sizes[];
extern const uint8_t sprite_palettes[];
extern const uint8_t level_data1p[];
extern const uint8_t level_data2p[];
extern const uint8_t level_data3[];
extern const uint8_t vscroll_offsets_table[];
extern const uint16_t rotation_table[];
extern const uint8_t bonus_spr_table[];
extern const uint8_t tiles_5dc9_lut[];
extern const uint8_t tiles_yoffset_table[];
extern const uint8_t monster_spr_anim_data0[];
extern const uint8_t monster_spr_anim_data1[];
extern const uint8_t monster_spr_anim_data2[];
extern const uint8_t monster_spr_anim_data3[];
extern const uint8_t monster_spr_anim_data6[];
extern const uint8_t monster_spr_anim_data8[];
extern const uint8_t monster_spr_anim_data9[];
extern const uint8_t common_palette_data[];
extern const uint8_t levels_palette_data[];
extern const uint8_t tile_lut_data0[];
extern const uint8_t tile_lut_data1[];
extern const uint8_t tile_lut_data2[];
extern const uint8_t tile_lut_data3[];
extern const uint16_t tile_funcs0[];
extern const uint16_t tile_funcs1[];
extern const uint16_t tile_funcs2[];
extern const uint16_t tile_funcs3[];
extern const uint8_t level_data_tiles_lut[];
extern const uint8_t player_anim_dy[];
extern const uint8_t player_anim_lut[];
extern const uint8_t *player_anim_table[];

/* game.c */
extern void	update_input();
extern void	game_main();
extern void	do_game_over_screen();
extern void	do_game_win_screen();
extern void	do_difficulty_screen();
extern void	do_level_password_screen();
extern void	do_level_number_screen();

/* level.c */
extern void	do_level();

/* screen.c */
extern void	video_draw_dot_pattern(int offset);
extern void	video_draw_sprite(int num, int x, int y, int flag);
extern void	video_draw_string(const char *s, int offset, int hspace);
extern void	video_copy_vga(int size);
extern void	video_copy_backbuffer(int h);
extern void	ja_decode_spr(const uint8_t *src, int w, int h, uint8_t *dst, int dst_pitch, uint8_t pal_mask);
extern void	ja_decode_chr(const uint8_t *buffer, const int size, uint8_t *dst, int dst_pitch);
extern void	ja_decode_tile(const uint8_t *buffer, uint8_t pal_mask, uint8_t *dst, int dst_pitch, int x, int y);
extern void	ja_decode_motif(int num, uint8_t color);
extern void	video_load_sprites();

/* sound.c */
extern void	sound_init();
extern void	sound_fini();
extern void	play_sound(int num);
extern void	play_music(int num);

#endif /* GAME_H__ */

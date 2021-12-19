
#ifndef RESOURCE_H__
#define RESOURCE_H__

#include "intern.h"

struct level_gate_t {
	uint16_t enter_pos;
	uint16_t tilemap_pos;
	uint16_t dst_pos;
	uint8_t scroll_flag;
};

struct level_column_t {
	uint16_t tilemap_pos; /* current column tilemap pos */
	uint8_t w;
	uint8_t h;
	uint16_t trigger_pos; /* compared with player pos */
	uint16_t tiles_offset_buf;
	uint8_t y_target; /* final y pos */
	uint8_t unk9;
};

struct level_bonus_t {
	uint8_t tile_num0; /* new tile */
	uint8_t tile_num1; /* original tile */
	uint8_t count;
	uint16_t pos;
};

struct level_item_t {
	int16_t x_pos, y_pos;
	uint16_t spr_num;
	int8_t y_delta;
};

struct level_platform_t {
	uint16_t x_pos;
	uint16_t y_pos;
	uint16_t spr_num;
	uint8_t flags;
	union {
		struct {
			uint8_t y_velocity;
			uint8_t unk8;
			uint8_t unk9;
			uint8_t state;
			uint16_t y_delta;
			uint8_t counter;
		} type8;
		struct {
			int8_t max_velocity;
			uint8_t unk9;
			uint16_t unkA;
			uint16_t counter;
			int8_t velocity;
		} other;
	};
};

struct level_monster_t {
	uint8_t len;
	uint8_t type;
	uint16_t spr_num;
	uint8_t flags;
	uint8_t energy;
	uint8_t respawn_ticks;
	uint8_t current_tick;
	uint8_t score;
	uint16_t x_pos;
	uint16_t y_pos;
	union {
		struct {
			uint8_t y_range;
			int8_t unkE;
		} type2;
		struct {
			uint8_t unkD;
		} type3;
		struct {
			uint8_t radius;
			uint8_t unkE;
			uint8_t angle;
			uint8_t angle_step;
		} type4;
		struct {
			uint8_t x_range;
			uint8_t y_range;
			uint8_t unkF;
		} type5;
		struct {
			uint8_t x_range;
			const uint8_t *pos;
		} type6;
		struct {
			uint8_t unkD;
			uint8_t unkE;
		} type7;
		struct {
			uint8_t x_range;
			int8_t y_step;
			int8_t x_step;
			uint8_t y_range;
		} type8;
		struct {
			int16_t unkD;
			int16_t unkF;
			int8_t x_step;
			uint8_t x_dist;
		} type9;
		struct {
			uint8_t unkD;
		} type10;
		struct {
			uint8_t unkD;
			uint8_t unkE;
			uint8_t unkF;
		} type11;
		struct {
			uint8_t unkD;
		} type12;
	};
};

#define MAX_LEVEL_GATES      20
#define MAX_LEVEL_COLUMNS    15
#define MAX_LEVEL_BONUSES    80
#define MAX_LEVEL_ITEMS      70
#define MAX_LEVEL_PLATFORMS  16
#define MAX_LEVEL_MONSTERS  150

struct level_t {
	uint8_t tile_attributes0[256];
	uint8_t tile_attributes1[256];
	uint8_t tile_attributes2[256]; /* 0x80: animated, 0x40: front, 0x20: animate if player is on top */
	uint8_t scrolling_top;
	uint16_t start_x_pos;
	uint16_t start_y_pos;
	uint16_t tilemap_w;
	uint16_t scrolling_mask; /* 4: screen scroll down 1 line, 2: no horizontal scrolling, 1: wider vertical scrolling */
	uint16_t front_tiles_lut[256];
	struct level_gate_t gates_tbl[MAX_LEVEL_GATES];
	struct level_column_t columns_tbl[MAX_LEVEL_COLUMNS];
	struct level_monster_t monsters_tbl[MAX_LEVEL_MONSTERS];
	uint8_t monsters_count;
	uint16_t items_spr_num_offset;
	uint16_t monsters_spr_num_offset;
	struct level_bonus_t bonuses_tbl[MAX_LEVEL_BONUSES];
	uint8_t tile_attributes3[256];
	struct level_item_t items_tbl[MAX_LEVEL_ITEMS];
	struct level_platform_t platforms_tbl[MAX_LEVEL_PLATFORMS];
	uint16_t boss_xmin;
	uint16_t boss_xmax;
	uint8_t boss_speed; /* 0..4 */
	int16_t boss_energy;
	uint8_t boss_state; /* !=255: has boss */
	uint16_t boss_x_pos;
	uint16_t boss_y_pos;
};

struct resource_t {
	bool dos_demo;
	uint8_t *maps;
	uint8_t *motif;
	uint8_t *allfonts;
	uint8_t *sprites;
	uint8_t *frontdat;
	int frontlen;
	uint8_t *uniondat;
	int unionlen;
	uint8_t *keyb;
	int keyblen;
	uint8_t *leveldat;
	int levellen;
	uint8_t *vga;
	uint8_t *background;
	struct level_t level, restart;
	uint8_t *samples;
	uint16_t spr_monsters_offset;
	uint16_t spr_monsters_count;
};

extern struct resource_t g_res;
extern int g_uncompressed_size;

extern void	res_init(const char *datapath, int vga_size);
extern void	res_fini();
extern void	load_leveldat(const uint8_t *data, struct level_t *level);
extern uint8_t *load_file(const char *filename);

#endif

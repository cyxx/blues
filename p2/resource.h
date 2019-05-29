
#ifndef RESOURCE_H__
#define RESOURCE_H__

#include "intern.h"

struct level_gate_t {
	uint16_t enter_pos; // (y << 8) | x
	uint16_t tilemap_pos;
	uint16_t dst_pos;
	uint8_t scroll_flag;
}; // sizeof == 7

struct level_platform_t {
	uint16_t tilemap_pos;
	uint8_t w;
	uint8_t h;
	uint16_t unk4;
	uint16_t unk6;
	uint8_t unk8; // y_offs
	uint8_t unk9;
}; // sizeof == 10

struct level_bonus_t {
	uint8_t tile_num0; /* new tile */
	uint8_t tile_num1; /* original tile */
	uint8_t count;
	uint16_t pos; // (y << 8) | x
}; // sizeof == 5

struct level_item_t {
	int16_t x_pos, y_pos;
	uint16_t spr_num;
	int8_t y_delta;
}; // sizeof == 7

struct level_trigger_t {
	uint16_t x_pos;
	uint16_t y_pos;
	uint16_t spr_num;
	uint8_t flags;
	union {
		struct {
			int8_t unk7;
			uint8_t unk8;
			uint8_t unk9;
			uint8_t state; // 0xA
			uint16_t y_delta; // 0xB
			uint8_t counter; // 0xD
		} type8;
		struct {
			int16_t unk7;
			uint8_t unk9;
			int16_t unkA;
			int16_t unkC;
		} other;
	};
	uint8_t unkE;
}; // sizeof == 15

struct level_monster_t {
	uint8_t len;
	uint8_t type;
	uint16_t spr_num; // 0x2
	uint8_t flags; // 0x4
	uint8_t unk5;
	uint8_t total_ticks;
	uint8_t current_tick;
	uint8_t unk8;
	uint16_t x_pos; // 0x9
	uint16_t y_pos; // 0xB
	union {
		struct {
			uint8_t y_range; // 0xD
			int8_t unkE; // 0xE, cbw
		} type2;
		struct {
			uint8_t x_range; // 0xD
			int8_t unkE; // 0xE, cbw
			int8_t unkF; // 0xF, cbw
			uint8_t y_range; // 0x10
		} type8;
	};
};

#define MAX_LEVEL_GATES     20
#define MAX_LEVEL_PLATFORMS 15
#define MAX_LEVEL_BONUSES   80
#define MAX_LEVEL_ITEMS     70
#define MAX_LEVEL_TRIGGERS  16
#define MAX_LEVEL_MONSTERS 150

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
	struct level_platform_t platforms_tbl[MAX_LEVEL_PLATFORMS];
	struct level_monster_t monsters_tbl[MAX_LEVEL_MONSTERS];
	uint8_t monsters_count;
	uint16_t items_spr_num_offset;
	uint16_t monsters_spr_num_offset;
	struct level_bonus_t bonuses_tbl[MAX_LEVEL_BONUSES];
	uint8_t tile_attributes3[256];
	struct level_item_t items_tbl[MAX_LEVEL_ITEMS];
	struct level_trigger_t triggers_tbl[MAX_LEVEL_TRIGGERS];
	uint16_t monsters_xmin;
	uint16_t monsters_xmax;
	uint8_t monsters_unk0;
	uint16_t monsters_unk1;
	uint8_t monsters_state;
	uint16_t end_x_pos;
	uint16_t end_y_pos;
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
	uint8_t *leveldat;
	int levellen;
	uint8_t *vga;
	uint8_t *background;
	struct level_t level, restart;
	uint8_t *samples;
};

extern struct resource_t g_res;
extern int g_uncompressed_size;

extern void	res_init(const char *datapath, int vga_size);
extern void	res_fini();
extern void	load_leveldat(const uint8_t *data, struct level_t *level);
extern uint8_t *load_file(const char *filename);

#endif

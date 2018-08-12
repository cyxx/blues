
#ifndef RESOURCE_H__
#define RESOURCE_H__

#include "intern.h"

#define TILE_NUM_CRATE    5
#define TILE_NUM_BALLOON  8
#define TILE_NUM_UMBRELLA 9

struct trigger_t {
	int16_t tile_type;
	int16_t tile_flags;
	uint8_t op_func;
	const uint8_t *op_table1; // dy for (x&15)
	const uint8_t *op_table2; // tile_animation
	int16_t unk10;
	const uint8_t *op_table3; // tile_trigger
	uint8_t unk16; // next_tile_num
	uint8_t tile_index; // op_table2
	uint8_t foreground_tile_num;
};

#define MAX_AVT           50
#define MAX_SPR_FRAMES   200
#define MAX_TRIGGERS     256
#define SPRITE_SIZE    35566
#define SOUND_SIZE     29376
#define SPRITES_COUNT    146

struct resource_data_t {
	uint8_t *sql;
	uint8_t *spr_sqv;
	uint8_t *avt_sqv;
	uint8_t *tmp;
	int avt_count;
	const uint8_t *avt[MAX_AVT];
	int spr_count;
	const uint8_t *spr_frames[MAX_SPR_FRAMES];
	uint8_t palette[16 * 3];
	struct trigger_t triggers[MAX_TRIGGERS];
	uint8_t *vga;
	int vga_size;
	uint8_t *tiles;
	uint8_t *snd;
	bool dos_demo;
	bool amiga_data;
	uint8_t cga_lut_avt[16 * 2];
	uint8_t cga_lut_sqv[16 * 2];
};

extern struct resource_data_t g_res;

extern void	res_init(int vga_size);
extern void	res_fini();
extern int	read_file(const char *filename, uint8_t *dst, int size);
extern int	read_compressed_file(const char *filename, uint8_t *dst);
extern void	load_avt(const char *filename, uint8_t *dst, int offset, int dither_pattern);
extern void	load_bin(const char *filename);
extern void	load_blk(const char *filename);
extern void	load_ck(const char *filename, uint16_t offset, int dither_pattern);
extern void	load_img(const char *filename, int screen_w, int dither_pattern);
extern void	load_m(const char *filename);
extern void	load_spr(const char *filename, uint8_t *dst, int offset);
extern void	load_sqv(const char *filename, uint8_t *dst, int offset, int dither_pattern);
extern void	load_sql(const char *filename);
extern uint8_t *	lookup_sql(int x, int y);

#endif /* RESOURCE_H__ */

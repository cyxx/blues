
#ifndef RESOURCE_H__
#define RESOURCE_H__

#include "intern.h"

struct resource_t {
	uint8_t *font;
	uint8_t *board;
	uint8_t *sprites;
	uint8_t *samples;
	uint8_t *tmp;
	uint8_t *background;
	uint8_t *vga;
};

extern struct resource_t g_res;

extern void	res_init(const char *datapath, int vga_size);
extern void	res_fini();
extern int	load_file(const char *filename);

#endif

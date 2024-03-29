
#ifndef INTERN_H__
#define INTERN_H__

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#define SWAP(x, y) do { __typeof__(x) tmp = x; x = y; y = tmp; } while(0)

#undef MIN
static inline int MIN(int a, int b) {
	return (a < b) ? a : b;
}

#undef MAX
static inline int MAX(int a, int b) {
	return (a > b) ? a : b;
}

static inline uint32_t READ_BE_UINT32(const uint8_t *p) {
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static inline uint16_t READ_BE_UINT16(const uint8_t *p) {
	return (p[0] << 8) | p[1];
}

static inline uint16_t READ_LE_UINT16(const uint8_t *p) {
	return p[0] | (p[1] << 8);
}

static inline void WRITE_LE_UINT16(uint8_t *p, uint16_t value) {
	p[0] = value & 0xFF;
	p[1] = value >> 8;
}

#define GAME_SCREEN_W g_options.screen_w
#define GAME_SCREEN_H g_options.screen_h

struct options_t {
	uint32_t cheats;
	int start_level;
	int start_xpos16;
	int start_ypos16;
	int screen_w;
	int screen_h;
	bool dos_scrolling;
	bool jump_button;
	// 'bb' only options
	bool amiga_copper_bars;
	bool amiga_colors;
	bool amiga_status_bar;
	bool cga_colors;
	// 'p2' only options
	bool hybrid_color;
};

struct game_t {
	const char *name;
	void (*res_init)(const char *data_path, int vga_size);
	void (*res_fini)();
	void (*snd_init)();
	void (*snd_fini)();
	void (*run)();
};

#ifdef _WIN32
#define EXPORT_SYMBOL __attribute__((dllexport))
#else
#define EXPORT_SYMBOL __attribute__((visibility("default")))
#endif

#endif

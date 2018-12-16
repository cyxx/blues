
#include <sys/param.h>
#include "resource.h"
#include "unpack.h"
#include "util.h"

static const int TMP_SIZE = 72728;

static const int BACKGROUND_SIZE = 320 * 200;

static const char *_datapath;

struct resource_t g_res;

void res_init(const char *path, int vga_size) {
	_datapath = path;

	char filepath[MAXPATHLEN];

	static const int FONT_SIZE = 7264;
	g_res.font = (uint8_t *)malloc(FONT_SIZE);
	if (!g_res.font) {
		print_error("Failed to allocate font buffer, %d bytes", FONT_SIZE);
	}
	snprintf(filepath, sizeof(filepath), "%s/font256.eat", _datapath);
	unpack(filepath, g_res.font, FONT_SIZE);

	static const int BOARD_SIZE = 12800;
	g_res.board = (uint8_t *)malloc(BOARD_SIZE);
	if (!g_res.board) {
		print_error("Failed to allocate board buffer, %d bytes", BOARD_SIZE);
	}
	snprintf(filepath, sizeof(filepath), "%s/board.eat", _datapath);
	unpack(filepath, g_res.board, BOARD_SIZE);

	static const int SPRITES_SIZE = 108044;
	g_res.sprites = (uint8_t *)malloc(SPRITES_SIZE);
	if (!g_res.sprites) {
		print_error("Failed to allocate sprites buffer, %d bytes", SPRITES_SIZE);
	}
	snprintf(filepath, sizeof(filepath), "%s/sprites.eat", _datapath);
	unpack(filepath, g_res.sprites, SPRITES_SIZE);

	static const int SAMPLES_SIZE = 49398;
	g_res.samples = (uint8_t *)malloc(SAMPLES_SIZE);
	if (!g_res.samples) {
		print_error("Failed to allocate samples buffer, %d bytes", SAMPLES_SIZE);
	}
	snprintf(filepath, sizeof(filepath), "%s/samples.eat", _datapath);
	unpack(filepath, g_res.samples, SAMPLES_SIZE);

	g_res.tmp = (uint8_t *)malloc(TMP_SIZE);
	if (!g_res.tmp) {
		print_error("Failed to allocate tmp buffer, %d bytes", TMP_SIZE);
	}

	g_res.background = (uint8_t *)malloc(BACKGROUND_SIZE);
	if (!g_res.background) {
		print_error("Failed to allocate tmp buffer, %d bytes", BACKGROUND_SIZE);
	}

	g_res.vga = (uint8_t *)malloc(vga_size);
	if (!g_res.vga) {
		print_error("Failed to allocate vga buffer, %d bytes", vga_size);
	}
}

void res_fini() {
	free(g_res.font);
	g_res.font = 0;
	free(g_res.board);
	g_res.board = 0;
	free(g_res.sprites);
	g_res.sprites = 0;
	free(g_res.tmp);
	g_res.tmp = 0;
	free(g_res.background);
	g_res.background = 0;
	free(g_res.vga);
	g_res.vga = 0;
}

int load_file(const char *filename) {
	char filepath[MAXPATHLEN];
	snprintf(filepath, sizeof(filepath), "%s/%s", _datapath, filename);
	print_debug(DBG_RESOURCE, "load_file '%s'", filepath);
	return unpack(filepath, g_res.tmp, TMP_SIZE);
}

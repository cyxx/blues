
#include "game.h"
#include "resource.h"
#include "mixer.h"
#include "util.h"

#define MAX_SOUNDS 11

static const uint16_t sound_sizes_tbl[] = {
	0x188E, 0x1C80, 0x235E, 0x19E6, 0x0AB2, 0x0912, 0x0000, 0x35D2,
	0x06C4, 0x1C86, 0x0E2E
};

static const char *trk_names_tbl[] = {
	"PRES.TRK",
	"CODE.TRK",
	"CARTE.TRK",
	"PRESENTA.TRK",
	"GLACE.TRK",
	0,
	0,
	0,
	0,
	"MINES.TRK",
	"MYSTERY.TRK",
	0,
	0,
	"MONSTER.TRK",
	"FINAL.TRK",
	"BRAVO.TRK",
	"KOOL.TRK",
	"BOULA.TRK"
};

static int _music_num;

static uint16_t sound_offsets_tbl[MAX_SOUNDS];

void sound_init() {
	uint16_t offset = 0;
	for (int i = 0; i < MAX_SOUNDS; ++i) {
		sound_offsets_tbl[i] = offset;
		offset += sound_sizes_tbl[i];
	}
	_music_num = -1;
}

void sound_fini() {
}

void play_sound(int num) {
	assert(num < MAX_SOUNDS);
	print_debug(DBG_SOUND, "play_sound %d", num);
	if (!g_res.samples) { /* no SAMPLE. file with demo */
		return;
	}
	const int sample_offset = sound_offsets_tbl[num];
	const int sample_size = sound_sizes_tbl[num];
	print_debug(DBG_SOUND, "sample num %d offset 0x%x size %d", num, sample_offset, sample_size);
	if (sample_size == 0) {
		return;
	}
	g_mix.play_sound(g_res.samples + sample_offset, sample_size, 8000, 0);
}

void play_music(int num) {
	if (g_res.dos_demo) { /* no .TRK files with demo */
		return;
	}
	if (_music_num == num) {
		return;
	}
	const char *filename = trk_names_tbl[num];
	if (filename) {
		print_debug(DBG_SOUND, "play_music '%s'", filename);
		uint8_t *data = load_file(filename);
		if (data) {
			g_mix.play_music(data, g_uncompressed_size);
			free(data);
			_music_num = num;
		}
	}
}

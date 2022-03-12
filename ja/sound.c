
#include "game.h"
#include "resource.h"
#include "mixer.h"
#include "util.h"

#define MAX_SOUNDS 16

static const char *_modules[] = {
	"almost.mod",
	"every.mod",
	"bartende.mod",
	"petergun.mod",
	"shoot.mod"
};

static struct {
	uint16_t offset;
	uint16_t size;
} _samples[MAX_SOUNDS];

static const int8_t *_seq;

static void repeat_sound_cb(const uint8_t **data, uint32_t *size) {
	const int next = _seq ? *_seq++ : -1;
	if (next < 0) {
		*data = 0;
		*size = 0;
	} else {
		*data = g_res.samples + _samples[next].offset;
		*size = _samples[next].size;
	}
}

void sound_init() {
	uint16_t offset = 0;
	for (int i = 0; i < MAX_SOUNDS; ++i) {
		const int num = i;
		_samples[i].size = sound_offsets[num];
		_samples[i].offset = offset;
		offset += _samples[i].size;
	}
}

void sound_fini() {
}

void play_sound(int num) {
	assert(num < MAX_SOUNDS);
	const int sample_offset = _samples[num].offset;
	const int sample_size = _samples[num].size;
	print_debug(DBG_SOUND, "sample num %d offset 0x%x size %d", num, sample_offset, sample_size);
	if (sample_size == 0) {
		return;
	}
	if (num == 10) {
		static const int8_t seq[] = { 10, 10, 10, 6, -1 };
		_seq = seq + 1;
	} else {
		_seq = 0;
	}
	g_mix.play_sound(g_res.samples + sample_offset, sample_size, 8000, _seq ? repeat_sound_cb : 0);
}

void play_music(int num) {
	const int size = load_file(_modules[num]);
	if (size != 0) {
		g_mix.play_music(g_res.tmp, size);
	}
}

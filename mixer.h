
#ifndef MIXER_H__
#define MIXER_H__

#include "intern.h"

typedef void (*mix_repeat_sound_cb)(const uint8_t **data, uint32_t *size);

struct mixer_t {
	void (*init)();
	void (*fini)();
	void (*play_sound)(const uint8_t *data, uint32_t size, int freq, mix_repeat_sound_cb repeat_cb);
	void (*play_music)(const uint8_t *data, uint32_t size);
};

extern struct mixer_t g_mix;

#endif

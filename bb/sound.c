
#include "fileio.h"
#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

#include <libmodplug/modplug.h>

#define PAULA_FREQ 3546897

static const struct {
	uint16_t offset; // words
	uint16_t size;
} _sounds_amiga[] = {
	{ 0x0000, 0x0256 },
	{ 0x0000, 0x0256 },
	{ 0x0000, 0x0256 },
	{ 0x0000, 0x0256 },
	{ 0x012b, 0x051a },
	{ 0x03b8, 0x0532 },
	{ 0x0651, 0x03e8 },
	{ 0x0b51, 0x0c1c },
	{ 0x115f, 0x0e40 },
	{ 0x0000, 0x0256 },
	{ 0x0000, 0x0256 },
	{ 0x187f, 0x04c4 },
	{ 0x1ae1, 0x0c14 },
	{ 0x20eb, 0x1e1a },
	{ 0x2ff8, 0x0610 },
	{ 0x3300, 0x0420 },
	{ 0x3510, 0x032c },
	{ 0x36a6, 0x0574 }
};

// Amiga, ExoticA
static const char *_modules[] = {
	"almost", "mod.almost",
	"gunn",   "mod.bluesgunnbest",
	"every",  "mod.every",
	"shot",   "mod.shot"
};

struct mixerchannel_t {
	uint8_t *data;
	uint32_t pos;
	uint32_t step;
	uint32_t size;
};

static const int _rate = SYS_AUDIO_FREQ;
static struct mixerchannel_t _channel;
static ModPlugFile *_mpf;

static void mix(void *param, uint8_t *buf, int len) {
	memset(buf, 0, len);
	if (_mpf) {
		const int count = ModPlug_Read(_mpf, buf, len);
		if (count == 0) {
			ModPlug_SeekOrder(_mpf, 0);
		}
	}
	if (_channel.data) {
		for (int i = 0; i < len; i += sizeof(int16_t)) {
			const int pos = _channel.pos >> 16;
			if (pos >= _channel.size) {
				_channel.data = 0;
				break;
			}
			const int sample = *(int16_t *)(buf + i) + ((int8_t)_channel.data[pos]) * 256;
			*(int16_t *)(buf + i) = (sample < -32768 ? -32768 : (sample > 32767 ? 32767 : sample));
			_channel.pos += _channel.step;
		}
	}
}

void sound_init() {
	ModPlug_Settings mp_settings;
	memset(&mp_settings, 0, sizeof(mp_settings));
	ModPlug_GetSettings(&mp_settings);
	mp_settings.mFlags = MODPLUG_ENABLE_OVERSAMPLING | MODPLUG_ENABLE_NOISE_REDUCTION;
	mp_settings.mChannels = 1;
	mp_settings.mBits = 16;
	mp_settings.mFrequency = _rate;
	mp_settings.mResamplingMode = MODPLUG_RESAMPLE_FIR;
	mp_settings.mLoopCount = -1;
	ModPlug_SetSettings(&mp_settings);
	g_sys.start_audio(mix, 0);
}

void sound_fini() {
	g_sys.stop_audio();
}

void play_sound(int num) {
	if (g_res.snd) {
		g_sys.lock_audio();
		_channel.data = g_res.snd + _sounds_amiga[num].offset * 2;
		_channel.pos = 0;
		_channel.step = ((PAULA_FREQ / 0x358) << 16) / _rate;
		_channel.size = _sounds_amiga[num].size;
		g_sys.unlock_audio();
	}
}

void play_music(int num) {
	g_sys.lock_audio();
	if (_mpf) {
		ModPlug_Unload(_mpf);
		_mpf = 0;
	}
	const char *filename = _modules[num * 2];
	if (fio_exists(filename)) { // Amiga
		int slot = fio_open(filename, 1);
		int size = fio_size(slot);
		uint8_t *buf = (uint8_t *)malloc(size);
		if (buf) {
			fio_read(slot, buf, size);

			// append samples to the end of the buffer
			static const int SONG_INFO_LEN = 20;
			static const int NUM_SAMPLES = 8;
			struct {
				char *name;
				int size;
			} samples[8];

			int samples_size = 0;
			int offset = SONG_INFO_LEN;
			for (int i = 0; i < NUM_SAMPLES; ++i, offset += 30) {
				samples[i].name = (char *)&buf[offset];
				samples[i].size = READ_BE_UINT16(&buf[offset + 22]) * 2;
				string_lower(samples[i].name);
				samples_size += samples[i].size;
			}

			buf = (uint8_t *)realloc(buf, size + samples_size);
			if (buf) {
				memset(buf + size, 0, samples_size);
				for (int i = 0; i < NUM_SAMPLES; ++i) {
					if (samples[i].size != 0) {
						if (samples[i].name[0]) {
							const int sample_slot = fio_open(samples[i].name, 0);
							if (sample_slot < 0) {
								print_warning("Unable to open instrument '%s'", samples[i].name);
							} else {
								fio_read(sample_slot, buf + size, samples[i].size);
								fio_close(sample_slot);
							}
						}
						size += samples[i].size;
					}
				}
				_mpf = ModPlug_Load(buf, size);
				free(buf);
			}
		}
		fio_close(slot);
	} else { // ExoticA
		filename = _modules[num * 2 + 1];
		if (fio_exists(filename)) {
			int slot = fio_open(filename, 1);
			int size = fio_size(slot);
			uint8_t *buf = (uint8_t *)malloc(size);
			if (buf) {
				fio_read(slot, buf, size);
				_mpf = ModPlug_Load(buf, size);
				free(buf);
			}
			fio_close(slot);
		}
	}
	g_sys.unlock_audio();
}

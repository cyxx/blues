
#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

#include <libmodplug/modplug.h>

#define MAX_SOUNDS 11

static const bool _volume = false;

static const uint16_t sound_sizes_tbl[] = {
	0x188E, 0x1C80, 0x235E, 0x19E6, 0x0AB2, 0x0912, 0x0000, 0x35D2,
	0x06C4, 0x1C86, 0x0E2E
};

static const uint8_t sound_volume_tbl[] = {
	0x3F,0x37,0x32,0x2F,0x2C,0x2A,0x28,0x27,0x26,0x24,0x23,0x22,0x21,0x21,0x20,0x1F,
	0x1E,0x1E,0x1D,0x1C,0x1C,0x1B,0x1B,0x1A,0x1A,0x19,0x19,0x19,0x18,0x18,0x17,0x17,
	0x17,0x16,0x16,0x16,0x15,0x15,0x15,0x15,0x14,0x14,0x14,0x14,0x13,0x13,0x13,0x13,
	0x12,0x12,0x12,0x12,0x11,0x11,0x11,0x11,0x11,0x10,0x10,0x10,0x10,0x10,0x0F,0x0F,
	0x0F,0x0F,0x0F,0x0F,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,
	0x0D,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,
	0x0B,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x09,0x09,0x09,0x09,0x09,0x09,
	0x09,0x09,0x09,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x07,0x07
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

struct mixerchannel_t {
	uint8_t *data;
	uint32_t pos;
	uint32_t step;
	uint32_t size;
};

static const int _rate = SYS_AUDIO_FREQ;
static struct mixerchannel_t _channel;
static ModPlugFile *_mpf;

static uint16_t sound_offsets_tbl[MAX_SOUNDS];

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
			const int8_t pcm = _volume ? sound_volume_tbl[(_channel.data[pos] ^ 0x80) >> 1] : _channel.data[pos];
			const int sample = *(int16_t *)(buf + i) + pcm * 256;
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
	uint16_t offset = 0;
	for (int i = 0; i < MAX_SOUNDS; ++i) {
		sound_offsets_tbl[i] = offset;
		offset += sound_sizes_tbl[i];
	}
	g_sys.start_audio(mix, 0);
}

void sound_fini() {
	g_sys.stop_audio();
}

void play_sound(int num) {
	assert(num < MAX_SOUNDS);
	print_debug(DBG_MIXER, "play_sound %d", num);
	if (!g_res.samples) { /* no SAMPLE. file with demo */
		return;
	}
	const int sample_offset = sound_offsets_tbl[num];
	const int sample_size = sound_sizes_tbl[num];
	print_debug(DBG_MIXER, "sample num %d offset 0x%x size %d", num, sample_offset, sample_size);
	if (sample_size == 0) {
		return;
	}
	g_sys.lock_audio();
	_channel.data = g_res.samples + sample_offset;
	_channel.pos = 0;
	_channel.step = (8000 << 16) / _rate;
	_channel.size = sample_size;
	g_sys.unlock_audio();
}

void play_music(int num) {
	if (g_res.dos_demo) { /* no .TRK files with demo */
		return;
	}
	const char *filename = trk_names_tbl[num];
	if (filename) {
		print_debug(DBG_MIXER, "play_music '%s'", filename);
		g_sys.lock_audio();
		if (_mpf) {
			ModPlug_Unload(_mpf);
			_mpf = 0;
		}
		uint8_t *data = load_file(filename);
		if (data) {
			_mpf = ModPlug_Load(data, g_uncompressed_size);
			if (_mpf) {
				print_debug(DBG_MIXER, "Loaded module '%s'", ModPlug_GetName(_mpf));
			}
			free(data);
		}
		g_sys.unlock_audio();
	}
}

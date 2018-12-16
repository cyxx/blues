
#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

#include <libmodplug/modplug.h>

#define MAX_SOUNDS 16

static const char *_modules[] = {
	"almost.mod",
	"every.mod",
	"bartende.mod",
	"petergun.mod",
	"shoot.mod"
};

struct mixerchannel_t {
	uint8_t *data;
	uint32_t pos;
	uint32_t step;
	uint32_t size;
	const int8_t *seq;
};

static struct {
	uint16_t offset;
	uint16_t size;
} _samples[MAX_SOUNDS];

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
			int pos = _channel.pos >> 16;
			if (pos >= _channel.size) {
				const int next = _channel.seq ? *_channel.seq++ : -1;
				if (next < 0) {
					_channel.data = 0;
					break;
				}
				_channel.data = g_res.samples + _samples[next].offset;
				_channel.pos = pos = 0;
				_channel.size = _samples[next].size;
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
	uint16_t offset = 0;
	for (int i = 0; i < MAX_SOUNDS; ++i) {
		const int num = i;
		_samples[i].size = sound_offsets[num];
		_samples[i].offset = offset;
		offset += _samples[i].size;
	}
}

void sound_fini() {
	g_sys.stop_audio();
}

void play_sound(int num) {
	assert(num < MAX_SOUNDS);
	const int sample_offset = _samples[num].offset;
	const int sample_size = _samples[num].size;
	print_debug(DBG_MIXER, "sample num %d offset 0x%x size %d", num, sample_offset, sample_size);
	if (sample_size == 0) {
		return;
	}
	g_sys.lock_audio();
	_channel.data = g_res.samples + sample_offset;
	_channel.pos = 0;
	_channel.step = (8000 << 16) / _rate;
	_channel.size = sample_size;
	if (num == 10)  {
		static const int8_t seq[] = { 10, 10, 10, 6, -1 };
		_channel.seq = seq + 1;
	} else {
		_channel.seq = 0;
	}
	g_sys.unlock_audio();
}

void play_music(int num) {
	g_sys.lock_audio();
	if (_mpf) {
		ModPlug_Unload(_mpf);
		_mpf = 0;
	}
	const int size = load_file(_modules[num]);
	_mpf = ModPlug_Load(g_res.tmp, size);
	if (_mpf) {
		print_debug(DBG_MIXER, "Loaded module '%s'", ModPlug_GetName(_mpf));
	}
	g_sys.unlock_audio();
}

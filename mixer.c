
#include "mixer.h"
#include "sys.h"
#include "util.h"

#include <libmodplug/modplug.h>

struct mixerchannel_t {
	const uint8_t *data;
	uint32_t pos;
	uint32_t step;
	uint32_t size;
	mix_repeat_sound_cb repeat_cb;
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
			int pos = _channel.pos >> 16;
			if (pos >= _channel.size) {
				_channel.data = 0;
				if (_channel.repeat_cb) {
					_channel.repeat_cb(&_channel.data, &_channel.size);
				}
				if (!_channel.data) {
					break;
				}
				_channel.pos = pos = 0;
			}
			const int sample = *(int16_t *)(buf + i) + ((int8_t)_channel.data[pos]) * 256;
			*(int16_t *)(buf + i) = (sample < -32768 ? -32768 : (sample > 32767 ? 32767 : sample));
			_channel.pos += _channel.step;
		}
	}
}

static void mixer_init() {
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

static void mixer_fini() {
	g_sys.stop_audio();
}

static void play_sound(const uint8_t *data, uint32_t size, int freq, mix_repeat_sound_cb repeat_cb) {
	g_sys.lock_audio();
	_channel.data = data;
	_channel.pos = 0;
	_channel.step = (freq << 16) / _rate;
	_channel.size = size;
	_channel.repeat_cb = repeat_cb;
	g_sys.unlock_audio();
}

static void play_music(const uint8_t *data, uint32_t size) {
	g_sys.lock_audio();
	if (_mpf) {
		ModPlug_Unload(_mpf);
		_mpf = 0;
	}
	_mpf = ModPlug_Load(data, size);
	if (_mpf) {
		print_debug(DBG_MIXER, "Loaded module '%s'", ModPlug_GetName(_mpf));
	}
	g_sys.unlock_audio();
}

struct mixer_t g_mix = {
	.init = mixer_init,
	.fini = mixer_fini,
	.play_sound = play_sound,
	.play_music = play_music,
};

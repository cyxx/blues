
#include <SDL.h>
#include "sys.h"
#include "util.h"

#define COPPER_BARS_H 80

static const int SCALE = 2;
static const int FADE_STEPS = 16;

static int _screen_w;
static int _screen_h;
static SDL_Window *_window;
static SDL_Renderer *_renderer;
static SDL_Texture *_texture;
static SDL_PixelFormat *_fmt;
static uint32_t _screen_palette[32];
static uint32_t *_screen_buffer;
static struct input_t *_input = &g_sys.input;
static int _copper_color;
static uint32_t _copper_palette[COPPER_BARS_H];

static int sdl2_init() {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_ShowCursor(SDL_DISABLE);
	_screen_w = _screen_h = 0;
	_window = 0;
	_renderer = 0;
	_texture = 0;
	_fmt = 0;
	memset(_screen_palette, 0, sizeof(_screen_palette));
	_screen_buffer = 0;
	_copper_color = -1;
	return 0;
}

static void sdl2_fini() {
	if (_renderer) {
		SDL_DestroyRenderer(_renderer);
		_renderer = 0;
	}
	if (_window) {
		SDL_DestroyWindow(_window);
		_window = 0;
	}
	if (_texture) {
		SDL_DestroyTexture(_texture);
		_texture = 0;
	}
	if (_fmt) {
		SDL_FreeFormat(_fmt);
		_fmt = 0;
	}
	free(_screen_buffer);
	SDL_Quit();
}

static void sdl2_set_screen_size(int w, int h, const char *caption) {
	assert(_screen_w == 0 && _screen_h == 0); // abort if called more than once
	_screen_w = w;
	_screen_h = h;
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); /* nearest pixel sampling */
	const int window_w = w * SCALE;
	const int window_h = h * SCALE; // * 4 / 3;
	_window = SDL_CreateWindow(caption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, 0);
	_renderer = SDL_CreateRenderer(_window, -1, 0);
	print_debug(DBG_SYSTEM, "set_screen_size %d,%d", _screen_w, _screen_h);
	_screen_buffer = (uint32_t *)calloc(_screen_w * _screen_h, sizeof(uint32_t));
	if (!_screen_buffer) {
		print_error("Failed to allocate screen buffer");
	}
	static const uint32_t pfmt = SDL_PIXELFORMAT_RGB888;
	_texture = SDL_CreateTexture(_renderer, pfmt, SDL_TEXTUREACCESS_STREAMING, _screen_w, _screen_h);
	_fmt = SDL_AllocFormat(pfmt);
}

static uint32_t convert_amiga_color(uint16_t color) {
	uint8_t r = (color >> 8) & 15;
	r |= r << 4;
	uint8_t g = (color >> 4) & 15;
	g |= g << 4;
	uint8_t b = color & 15;
	b |= b << 4;
	return SDL_MapRGB(_fmt, r, g, b);
}

static void sdl2_set_palette_amiga(const uint16_t *colors) {
	for (int i = 0; i < 32; ++i) {
		_screen_palette[i] = convert_amiga_color(colors[i]);
	}
}

static void sdl2_set_copper_bars(const uint16_t *data) {
	if (!data) {
		_copper_color = -1;
	} else {
		_copper_color = (data[0] - 0x180) / 2;
		const uint16_t *src = data + 1;
		uint32_t *dst = _copper_palette;
		for (int i = 0; i < 16; ++i) {
			const int j = i + 1;
			*dst++ = convert_amiga_color(src[j]);
			*dst++ = convert_amiga_color(src[i]);
			*dst++ = convert_amiga_color(src[j]);
			*dst++ = convert_amiga_color(src[i]);
			*dst++ = convert_amiga_color(src[j]);
		}
        }
}

static void sdl2_set_screen_palette(const uint8_t *colors, int count) {
	for (int i = 0; i < count; ++i) {
		_screen_palette[i] = SDL_MapRGB(_fmt, colors[0], colors[1], colors[2]);
		colors += 3;
	}
}

static void fade_palette_helper(int in) {
	SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
	SDL_Rect r;
	r.x = r.y = 0;
	SDL_GetRendererOutputSize(_renderer, &r.w, &r.h);
	for (int i = 1; i <= FADE_STEPS; ++i) {
		int alpha = 256 * i / FADE_STEPS;
		if (in) {
			alpha = 256 - alpha;
		}
		SDL_SetRenderDrawColor(_renderer, 0, 0, 0, alpha);
		SDL_RenderClear(_renderer);
		SDL_RenderCopy(_renderer, _texture, 0, 0);
		SDL_RenderFillRect(_renderer, &r);
		SDL_RenderPresent(_renderer);
		SDL_Delay(30);
	}
}

static void sdl2_fade_in_palette() {
	fade_palette_helper(1);
}

static void sdl2_fade_out_palette() {
	fade_palette_helper(0);
}

static void sdl2_update_screen(const uint8_t *p, int present) {
	if (_copper_color != -1) {
		for (int j = 0; j < _screen_h; ++j) {
			for (int i = 0; i < _screen_w; ++i) {
				if (*p == _copper_color && j / 2 < COPPER_BARS_H) {
					_screen_buffer[j * _screen_w + i] = _copper_palette[j / 2];
				} else {
					_screen_buffer[j * _screen_w + i] = _screen_palette[*p];
				}
				++p;
			}
		}
	} else {
		for (int i = 0; i < _screen_w * _screen_h; ++i) {
			_screen_buffer[i] = _screen_palette[p[i]];
		}
	}
	SDL_UpdateTexture(_texture, 0, _screen_buffer, _screen_w * sizeof(uint32_t));
	if (present) {
		SDL_RenderClear(_renderer);
		SDL_RenderCopy(_renderer, _texture, 0, 0);
		SDL_RenderPresent(_renderer);
	}
}

static void handle_keyevent(int keysym, int keydown) {
	switch (keysym) {
	case SDLK_LEFT:
		if (keydown) {
			_input->direction |= INPUT_DIRECTION_LEFT;
		} else {
			_input->direction &= ~INPUT_DIRECTION_LEFT;
		}
		break;
	case SDLK_RIGHT:
		if (keydown) {
			_input->direction |= INPUT_DIRECTION_RIGHT;
		} else {
			_input->direction &= ~INPUT_DIRECTION_RIGHT;
		}
		break;
	case SDLK_UP:
		if (keydown) {
			_input->direction |= INPUT_DIRECTION_UP;
		} else {
			_input->direction &= ~INPUT_DIRECTION_UP;
		}
		break;
	case SDLK_DOWN:
		if (keydown) {
			_input->direction |= INPUT_DIRECTION_DOWN;
		} else {
			_input->direction &= ~INPUT_DIRECTION_DOWN;
		}
		break;
	case SDLK_RETURN:
	case SDLK_SPACE:
		_input->space = keydown;
		break;
	}
}

static int handle_event(const SDL_Event *ev) {
	switch (ev->type) {
	case SDL_QUIT:
		_input->quit = 1;
		break;
	case SDL_KEYUP:
		handle_keyevent(ev->key.keysym.sym, 0);
		break;
	case SDL_KEYDOWN:
		handle_keyevent(ev->key.keysym.sym, 1);
		break;
	default:
		return -1;
	}
	return 0;
}

static void sdl2_process_events() {
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		handle_event(&ev);
		if (_input->quit) {
			break;
		}
	}
}

static void sdl2_sleep(int duration) {
	SDL_Delay(duration);
}

static uint32_t sdl2_get_timestamp() {
	return SDL_GetTicks();
}

static void sdl2_start_audio(sys_audio_cb callback, void *param) {
	SDL_AudioSpec desired;
	memset(&desired, 0, sizeof(desired));
	desired.freq = SYS_AUDIO_FREQ;
	desired.format = AUDIO_S16;
	desired.channels = 1;
	desired.samples = 2048;
	desired.callback = callback;
	desired.userdata = param;
	if (SDL_OpenAudio(&desired, 0) == 0) {
		SDL_PauseAudio(0);
	}
}

static void sdl2_stop_audio() {
	SDL_CloseAudio();
}

static void sdl2_lock_audio() {
	SDL_LockAudio();
}

static void sdl2_unlock_audio() {
	SDL_UnlockAudio();
}

struct sys_t g_sys = {
	.init	= sdl2_init,
	.fini	= sdl2_fini,
	.set_screen_size	= sdl2_set_screen_size,
	.set_screen_palette	= sdl2_set_screen_palette,
	.set_palette_amiga	= sdl2_set_palette_amiga,
	.set_copper_bars	= sdl2_set_copper_bars,
	.fade_in_palette	= sdl2_fade_in_palette,
	.fade_out_palette	= sdl2_fade_out_palette,
	.update_screen	= sdl2_update_screen,
	.process_events	= sdl2_process_events,
	.sleep	= sdl2_sleep,
	.get_timestamp = sdl2_get_timestamp,
	.start_audio = sdl2_start_audio,
	.stop_audio = sdl2_stop_audio,
	.lock_audio = sdl2_lock_audio,
	.unlock_audio = sdl2_unlock_audio,
};

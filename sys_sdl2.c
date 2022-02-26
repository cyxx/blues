
#include <SDL.h>
#include "sys.h"
#include "util.h"

#define COPPER_BARS_H 80
#define MAX_SPRITES 256
#define MAX_SPRITESHEETS 3

static const int FADE_STEPS = 16;

struct spritesheet_t {
	int count;
	SDL_Rect *r;
	SDL_Surface *surface;
	SDL_Texture *texture;
};

static struct spritesheet_t _spritesheets[MAX_SPRITESHEETS];

struct sprite_t {
	int sheet;
	int num;
	int x, y;
	bool xflip;
};

static struct sprite_t _sprites[MAX_SPRITES];
static int _sprites_count;
static SDL_Rect _sprites_cliprect;

static int _screen_w, _screen_h;
static int _shake_dx, _shake_dy;
static SDL_Window *_window;
static SDL_Renderer *_renderer;
static SDL_Texture *_texture;
static SDL_PixelFormat *_fmt;
static SDL_Palette *_palette;
static uint32_t _screen_palette[256];
static uint32_t *_screen_buffer;
static int _copper_color_key;
static uint32_t _copper_palette[COPPER_BARS_H];

static SDL_GameController *_controller;
static SDL_Joystick *_joystick;

static int sdl2_init() {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
	SDL_ShowCursor(SDL_DISABLE);
	_screen_w = _screen_h = 0;
	memset(_screen_palette, 0, sizeof(_screen_palette));
	_palette = SDL_AllocPalette(256);
	_screen_buffer = 0;
	_copper_color_key = -1;
	SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
	_controller = 0;
	const int count = SDL_NumJoysticks();
	if (count > 0) {
		for (int i = 0; i < count; ++i) {
			if (SDL_IsGameController(i)) {
				_controller = SDL_GameControllerOpen(i);
				if (_controller) {
					fprintf(stdout, "Using controller '%s'\n", SDL_GameControllerName(_controller));
					break;
				}
			}
		}
		if (!_controller) {
			_joystick = SDL_JoystickOpen(0);
			if (_joystick) {
				fprintf(stdout, "Using joystick '%s'\n", SDL_JoystickName(_joystick));
			}
		}
	}
	return 0;
}

static void sdl2_fini() {
	if (_fmt) {
		SDL_FreeFormat(_fmt);
		_fmt = 0;
	}
	if (_palette) {
		SDL_FreePalette(_palette);
		_palette = 0;
	}
	if (_texture) {
		SDL_DestroyTexture(_texture);
		_texture = 0;
	}
	if (_renderer) {
		SDL_DestroyRenderer(_renderer);
		_renderer = 0;
	}
	if (_window) {
		SDL_DestroyWindow(_window);
		_window = 0;
	}
	free(_screen_buffer);
	if (_controller) {
		SDL_GameControllerClose(_controller);
		_controller = 0;
	}
	if (_joystick) {
		SDL_JoystickClose(_joystick);
		_joystick = 0;
	}
	SDL_Quit();
}

static void sdl2_set_screen_size(int w, int h, const char *caption, int scale, const char *filter, bool fullscreen) {
	assert(_screen_w == 0 && _screen_h == 0); // abort if called more than once
	_screen_w = w;
	_screen_h = h;
	if (!filter || strcmp(filter, "nearest") == 0) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	} else if (strcmp(filter, "linear") == 0) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	} else {
		print_warning("Unhandled filter '%s'", filter);
	}
	const int window_w = w * scale;
	const int window_h = h * scale;
	const int flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE;
	_window = SDL_CreateWindow(caption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, flags);
	_renderer = SDL_CreateRenderer(_window, -1, 0);
	SDL_RenderSetLogicalSize(_renderer, w, h);
	print_debug(DBG_SYSTEM, "set_screen_size %d,%d", _screen_w, _screen_h);
	_screen_buffer = (uint32_t *)calloc(_screen_w * _screen_h, sizeof(uint32_t));
	if (!_screen_buffer) {
		print_error("Failed to allocate screen buffer");
	}
	static const uint32_t pfmt = SDL_PIXELFORMAT_RGB888;
	_texture = SDL_CreateTexture(_renderer, pfmt, SDL_TEXTUREACCESS_STREAMING, _screen_w, _screen_h);
	_fmt = SDL_AllocFormat(pfmt);
	_sprites_cliprect.x = 0;
	_sprites_cliprect.y = 0;
	_sprites_cliprect.w = w;
	_sprites_cliprect.h = h;
}

static uint32_t convert_amiga_color(uint16_t color) {
	uint8_t r = (color >> 8) & 15;
	r |= r << 4;
	uint8_t g = (color >> 4) & 15;
	g |= g << 4;
	uint8_t b =  color       & 15;
	b |= b << 4;
	return SDL_MapRGB(_fmt, r, g, b);
}

static void set_amiga_color(uint16_t color, SDL_Color *p) {
	const uint8_t r = (color >> 8) & 15;
	p->r = (r << 4) | r;
	const uint8_t g = (color >> 4) & 15;
	p->g = (g << 4) | g;
	const uint8_t b =  color       & 15;
	p->b = (b << 4) | b;
}

static void sdl2_set_palette_amiga(const uint16_t *colors, int offset) {
	SDL_Color *palette_colors = &_palette->colors[offset];
	for (int i = 0; i < 16; ++i) {
		_screen_palette[offset + i] = convert_amiga_color(colors[i]);
		set_amiga_color(colors[i], &palette_colors[i]);
	}
}

static void sdl2_set_copper_bars(const uint16_t *data) {
	if (!data) {
		_copper_color_key = -1;
	} else {
		_copper_color_key = (data[0] - 0x180) / 2;
		const uint16_t *src = data + 1;
		uint32_t *dst = _copper_palette;
		for (int i = 0; i < COPPER_BARS_H / 5; ++i) {
			const int j = i + 1;
			*dst++ = convert_amiga_color(src[j]);
			*dst++ = convert_amiga_color(src[i]);
			*dst++ = convert_amiga_color(src[j]);
			*dst++ = convert_amiga_color(src[i]);
			*dst++ = convert_amiga_color(src[j]);
		}
        }
}

static void sdl2_set_screen_palette(const uint8_t *colors, int offset, int count, int depth) {
	SDL_Color *palette_colors = &_palette->colors[offset];
	const int shift = 8 - depth;
	for (int i = 0; i < count; ++i) {
		int r = colors[0];
		int g = colors[1];
		int b = colors[2];
		if (depth != 8) {
			r = (r << shift) | (r >> (depth - shift));
			g = (g << shift) | (g >> (depth - shift));
			b = (b << shift) | (b >> (depth - shift));
		}
		_screen_palette[offset + i] = SDL_MapRGB(_fmt, r, g, b);
		palette_colors[i].r = r;
		palette_colors[i].g = g;
		palette_colors[i].b = b;
		colors += 3;
	}
	for (int i = 0; i < ARRAYSIZE(_spritesheets); ++i) {
		struct spritesheet_t *sheet = &_spritesheets[i];
		if (sheet->surface) {
			SDL_DestroyTexture(sheet->texture);
			sheet->texture = SDL_CreateTextureFromSurface(_renderer, sheet->surface);
		}
	}
}

static void sdl2_set_palette_color(int i, const uint8_t *colors) {
	int r = colors[0];
	r = (r << 2) | (r >> 4);
	int g = colors[1];
	g = (g << 2) | (g >> 4);
	int b = colors[2];
	b = (b << 2) | (b >> 4);
	_screen_palette[i] = SDL_MapRGB(_fmt, r, g, b);
}

static void fade_palette_helper(int in) {
	SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
	SDL_Rect r;
	r.x = r.y = 0;
	SDL_GetRendererOutputSize(_renderer, &r.w, &r.h);
	for (int i = 0; i <= FADE_STEPS; ++i) {
		int alpha = 255 * i / FADE_STEPS;
		if (in) {
			alpha = 255 - alpha;
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
	if (!g_sys.input.quit) {
		fade_palette_helper(1);
	}
}

static void sdl2_fade_out_palette() {
	if (!g_sys.input.quit) {
		fade_palette_helper(0);
	}
}

static void sdl2_transition_screen(enum sys_transition_e type, bool open) {
	const int step_w = _screen_w / FADE_STEPS;
	const int step_h = _screen_h / FADE_STEPS;
	SDL_Rect r;
	r.x = 0;
	r.w = 0;
	r.y = 0;
	r.h = (type == TRANSITION_CURTAIN) ? _screen_h : 0;
	do {
		r.x = (_screen_w - r.w) / 2;
		if (r.x < 0) {
			r.x = 0;
		}
		r.w += step_w;
		if (r.x + r.w > _screen_w) {
			r.w = _screen_w - r.x;
		}
		if (type == TRANSITION_SQUARE) {
			r.y = (_screen_h - r.h) / 2;
			if (r.y < 0) {
				r.y = 0;
			}
			r.h += step_h;
			if (r.y + r.h > _screen_h) {
				r.h = _screen_h - r.y;
			}
		}
		SDL_RenderClear(_renderer);
		SDL_RenderCopy(_renderer, _texture, &r, &r);
		SDL_RenderPresent(_renderer);
		SDL_Delay(30);
	} while (r.x > 0 && (type == TRANSITION_CURTAIN || r.y > 0));
}

static void sdl2_update_screen(const uint8_t *p, int present) {
	if (_copper_color_key != -1) {
		for (int j = 0; j < _screen_h; ++j) {
			if (j / 2 < COPPER_BARS_H) {
				const uint32_t line_color = _copper_palette[j / 2];
				for (int i = 0; i < _screen_w; ++i) {
					_screen_buffer[j * _screen_w + i] = (p[i] == _copper_color_key) ? line_color : _screen_palette[p[i]];
				}
			} else {
				for (int i = 0; i < _screen_w; ++i) {
					_screen_buffer[j * _screen_w + i] = _screen_palette[p[i]];
				}
			}
			p += _screen_w;
		}
	} else {
		for (int i = 0; i < _screen_w * _screen_h; ++i) {
			_screen_buffer[i] = _screen_palette[p[i]];
		}
	}
	SDL_UpdateTexture(_texture, 0, _screen_buffer, _screen_w * sizeof(uint32_t));
	if (present) {
		SDL_Rect r;
		r.x = _shake_dx;
		r.y = _shake_dy;
		r.w = _screen_w;
		r.h = _screen_h;
		SDL_RenderClear(_renderer);
		SDL_RenderCopy(_renderer, _texture, 0, &r);

		// sprites
		SDL_RenderSetClipRect(_renderer, &_sprites_cliprect);
		for (int i = 0; i < _sprites_count; ++i) {
			const struct sprite_t *spr = &_sprites[i];
			struct spritesheet_t *sheet = &_spritesheets[spr->sheet];
			if (spr->num >= sheet->count) {
				continue;
			}
			SDL_Rect r;
			r.x = spr->x + _shake_dx;
			r.y = spr->y + _shake_dy;
			r.w = sheet->r[spr->num].w;
			r.h = sheet->r[spr->num].h;
			if (!spr->xflip) {
				SDL_RenderCopy(_renderer, sheet->texture, &sheet->r[spr->num], &r);
			} else {
				SDL_RenderCopyEx(_renderer, sheet->texture, &sheet->r[spr->num], &r, 0., 0, SDL_FLIP_HORIZONTAL);
			}
		}
		SDL_RenderSetClipRect(_renderer, 0);

		SDL_RenderPresent(_renderer);
	}
}

static void sdl2_shake_screen(int dx, int dy) {
	_shake_dx = dx;
	_shake_dy = dy;
}

static void handle_keyevent(int keysym, bool keydown, struct input_t *input) {
	switch (keysym) {
	case SDLK_LEFT:
		if (keydown) {
			input->direction |= INPUT_DIRECTION_LEFT;
		} else {
			input->direction &= ~INPUT_DIRECTION_LEFT;
		}
		break;
	case SDLK_RIGHT:
		if (keydown) {
			input->direction |= INPUT_DIRECTION_RIGHT;
		} else {
			input->direction &= ~INPUT_DIRECTION_RIGHT;
		}
		break;
	case SDLK_UP:
		if (keydown) {
			input->direction |= INPUT_DIRECTION_UP;
		} else {
			input->direction &= ~INPUT_DIRECTION_UP;
		}
		break;
	case SDLK_DOWN:
		if (keydown) {
			input->direction |= INPUT_DIRECTION_DOWN;
		} else {
			input->direction &= ~INPUT_DIRECTION_DOWN;
		}
		break;
	case SDLK_RETURN:
	case SDLK_SPACE:
		input->space = keydown;
		break;
	case SDLK_ESCAPE:
		if (keydown) {
			g_sys.input.quit = true;
		}
		break;
	case SDLK_1:
		input->digit1 = keydown;
		break;
	case SDLK_2:
		input->digit2 = keydown;
		break;
	case SDLK_3:
		input->digit3 = keydown;
		break;
	}
}

static void handle_controlleraxis(int axis, int value, struct input_t *input) {
	static const int THRESHOLD = 3200;
	switch (axis) {
	case SDL_CONTROLLER_AXIS_LEFTX:
	case SDL_CONTROLLER_AXIS_RIGHTX:
		if (value < -THRESHOLD) {
			input->direction |= INPUT_DIRECTION_LEFT;
		} else {
			input->direction &= ~INPUT_DIRECTION_LEFT;
		}
		if (value > THRESHOLD) {
			input->direction |= INPUT_DIRECTION_RIGHT;
		} else {
			input->direction &= ~INPUT_DIRECTION_RIGHT;
		}
		break;
	case SDL_CONTROLLER_AXIS_LEFTY:
	case SDL_CONTROLLER_AXIS_RIGHTY:
		if (value < -THRESHOLD) {
			input->direction |= INPUT_DIRECTION_UP;
		} else {
			input->direction &= ~INPUT_DIRECTION_UP;
		}
		if (value > THRESHOLD) {
			input->direction |= INPUT_DIRECTION_DOWN;
		} else {
			input->direction &= ~INPUT_DIRECTION_DOWN;
		}
		break;
	}
}

static void handle_controllerbutton(int button, bool pressed, struct input_t *input) {
	switch (button) {
	case SDL_CONTROLLER_BUTTON_A:
	case SDL_CONTROLLER_BUTTON_B:
	case SDL_CONTROLLER_BUTTON_X:
	case SDL_CONTROLLER_BUTTON_Y:
		input->space = pressed;
		break;
	case SDL_CONTROLLER_BUTTON_BACK:
		break;
	case SDL_CONTROLLER_BUTTON_START:
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		if (pressed) {
			input->direction |= INPUT_DIRECTION_UP;
		} else {
			input->direction &= ~INPUT_DIRECTION_UP;
		}
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		if (pressed) {
			input->direction |= INPUT_DIRECTION_DOWN;
		} else {
			input->direction &= ~INPUT_DIRECTION_DOWN;
		}
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		if (pressed) {
			input->direction |= INPUT_DIRECTION_LEFT;
		} else {
			input->direction &= ~INPUT_DIRECTION_LEFT;
		}
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		if (pressed) {
			input->direction |= INPUT_DIRECTION_RIGHT;
		} else {
			input->direction &= ~INPUT_DIRECTION_RIGHT;
		}
		break;
	}
}

static void handle_joystickhatmotion(int value, struct input_t *input) {
	input->direction = 0;
	if (value & SDL_HAT_UP) {
		input->direction |= INPUT_DIRECTION_UP;
	}
	if (value & SDL_HAT_DOWN) {
		input->direction |= INPUT_DIRECTION_DOWN;
	}
	if (value & SDL_HAT_LEFT) {
		input->direction |= INPUT_DIRECTION_LEFT;
	}
	if (value & SDL_HAT_RIGHT) {
		input->direction |= INPUT_DIRECTION_RIGHT;
	}
}

static void handle_joystickaxismotion(int axis, int value, struct input_t *input) {
	static const int THRESHOLD = 3200;
	switch (axis) {
	case 0:
		input->direction &= ~(INPUT_DIRECTION_RIGHT | INPUT_DIRECTION_LEFT);
		if (value > THRESHOLD) {
			input->direction |= INPUT_DIRECTION_RIGHT;
		} else if (value < -THRESHOLD) {
			input->direction |= INPUT_DIRECTION_LEFT;
		}
		break;
	case 1:
		input->direction &= ~(INPUT_DIRECTION_UP | INPUT_DIRECTION_DOWN);
		if (value > THRESHOLD) {
			input->direction |= INPUT_DIRECTION_DOWN;
		} else if (value < -THRESHOLD) {
			input->direction |= INPUT_DIRECTION_UP;
		}
		break;
	}
}

static void handle_joystickbutton(int button, int pressed, struct input_t *input) {
	if (button == 0) {
		input->space = pressed;
	}
}

static int handle_event(const SDL_Event *ev, bool *paused) {
	switch (ev->type) {
	case SDL_QUIT:
		g_sys.input.quit = true;
		break;
	case SDL_WINDOWEVENT:
		switch (ev->window.event) {
		case SDL_WINDOWEVENT_FOCUS_GAINED:
		case SDL_WINDOWEVENT_FOCUS_LOST:
			*paused = (ev->window.event == SDL_WINDOWEVENT_FOCUS_LOST);
			SDL_PauseAudio(*paused);
			break;
		}
		break;
	case SDL_KEYUP:
		handle_keyevent(ev->key.keysym.sym, 0, &g_sys.input);
		break;
	case SDL_KEYDOWN:
		handle_keyevent(ev->key.keysym.sym, 1, &g_sys.input);
		break;
	case SDL_CONTROLLERDEVICEADDED:
		if (!_controller) {
			_controller = SDL_GameControllerOpen(ev->cdevice.which);
			if (_controller) {
				fprintf(stdout, "Using controller '%s'\n", SDL_GameControllerName(_controller));
			}
		}
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
		if (_controller == SDL_GameControllerFromInstanceID(ev->cdevice.which)) {
			fprintf(stdout, "Removed controller '%s'\n", SDL_GameControllerName(_controller));
			SDL_GameControllerClose(_controller);
			_controller = 0;
		}
		break;
	case SDL_CONTROLLERBUTTONUP:
		if (_controller) {
			handle_controllerbutton(ev->cbutton.button, 0, &g_sys.input);
		}
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		if (_controller) {
			handle_controllerbutton(ev->cbutton.button, 1, &g_sys.input);
		}
		break;
	case SDL_CONTROLLERAXISMOTION:
		if (_controller) {
			handle_controlleraxis(ev->caxis.axis, ev->caxis.value, &g_sys.input);
		}
		break;
	case SDL_JOYHATMOTION:
		if (_joystick) {
			handle_joystickhatmotion(ev->jhat.value, &g_sys.input);
		}
		break;
	case SDL_JOYAXISMOTION:
		if (_joystick) {
			handle_joystickaxismotion(ev->jaxis.axis, ev->jaxis.value, &g_sys.input);
		}
		break;
	case SDL_JOYBUTTONUP:
		if (_joystick) {
			handle_joystickbutton(ev->jbutton.button, 0, &g_sys.input);
		}
		break;
	case SDL_JOYBUTTONDOWN:
		if (_joystick) {
			handle_joystickbutton(ev->jbutton.button, 1, &g_sys.input);
		}
		break;
	default:
		return -1;
	}
	return 0;
}

static void sdl2_process_events() {
	bool paused = false;
	while (1) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			handle_event(&ev, &paused);
			if (g_sys.input.quit) {
				break;
			}
		}
		if (!paused) {
			break;
		}
		SDL_Delay(100);
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

static void render_load_sprites(int spr_type, int count, const struct sys_rect_t *r, const uint8_t *data, int w, int h, uint8_t color_key, bool update_pal) {
	assert(spr_type < ARRAYSIZE(_spritesheets));
	struct spritesheet_t *sheet = &_spritesheets[spr_type];
	sheet->count = count;
	sheet->r = (SDL_Rect *)malloc(count * sizeof(SDL_Rect));
	for (int i = 0; i < count; ++i) {
		SDL_Rect *rect = &sheet->r[i];
		rect->x = r[i].x;
		rect->y = r[i].y;
		rect->w = r[i].w;
		rect->h = r[i].h;
	}
	SDL_Surface *surface = SDL_CreateRGBSurface(0, w, h, 8, 0x0, 0x0, 0x0, 0x0);
	SDL_SetSurfacePalette(surface, _palette);
	SDL_SetColorKey(surface, 1, color_key);
	SDL_LockSurface(surface);
	for (int y = 0; y < h; ++y) {
		memcpy(((uint8_t *)surface->pixels) + y * surface->pitch, data + y * w, w);
	}
	SDL_UnlockSurface(surface);
	sheet->texture = SDL_CreateTextureFromSurface(_renderer, surface);
	if (update_pal) { /* update texture on palette change */
		sheet->surface = surface;
	} else  {
		SDL_FreeSurface(surface);
	}
}

static void render_unload_sprites(int spr_type) {
	struct spritesheet_t *sheet = &_spritesheets[spr_type];
	free(sheet->r);
	if (sheet->surface) {
		SDL_FreeSurface(sheet->surface);
	}
	if (sheet->texture) {
		SDL_DestroyTexture(sheet->texture);
	}
	memset(sheet, 0, sizeof(struct spritesheet_t));
}

static void render_add_sprite(int spr_type, int frame, int x, int y, int xflip) {
	assert(_sprites_count < ARRAYSIZE(_sprites));
	struct sprite_t *spr = &_sprites[_sprites_count];
	spr->sheet = spr_type;
	spr->num = frame;
	spr->x = x;
	spr->y = y;
	spr->xflip = xflip;
	++_sprites_count;
}

static void render_clear_sprites() {
	_sprites_count = 0;
}

static void render_set_sprites_clipping_rect(int x, int y, int w, int h) {
	_sprites_cliprect.x = x;
	_sprites_cliprect.y = y;
	_sprites_cliprect.w = w;
	_sprites_cliprect.h = h;
}

struct sys_t g_sys = {
	.init = sdl2_init,
	.fini = sdl2_fini,
	.set_screen_size = sdl2_set_screen_size,
	.set_screen_palette = sdl2_set_screen_palette,
	.set_palette_amiga = sdl2_set_palette_amiga,
	.set_copper_bars = sdl2_set_copper_bars,
	.set_palette_color = sdl2_set_palette_color,
	.fade_in_palette = sdl2_fade_in_palette,
	.fade_out_palette = sdl2_fade_out_palette,
	.update_screen = sdl2_update_screen,
	.shake_screen = sdl2_shake_screen,
	.transition_screen = sdl2_transition_screen,
	.process_events = sdl2_process_events,
	.sleep = sdl2_sleep,
	.get_timestamp = sdl2_get_timestamp,
	.start_audio = sdl2_start_audio,
	.stop_audio = sdl2_stop_audio,
	.lock_audio = sdl2_lock_audio,
	.unlock_audio = sdl2_unlock_audio,
	.render_load_sprites = render_load_sprites,
	.render_unload_sprites = render_unload_sprites,
	.render_add_sprite = render_add_sprite,
	.render_clear_sprites = render_clear_sprites,
	.render_set_sprites_clipping_rect = render_set_sprites_clipping_rect
};

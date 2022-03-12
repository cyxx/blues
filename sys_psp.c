
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <pspsdk.h>
#include <malloc.h>
#include "sys.h"

PSP_MODULE_INFO("Blues Brothers", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

static uint32_t _buttons;

static sys_audio_cb _audio_proc;
static void *_audio_param;
static int _audio_channel;
static SceUID _audio_mutex;

static uint32_t _fb_offset;

// static const int AUDIO_FREQ = 44100;
static const int AUDIO_SAMPLES_COUNT = 2048;

static const int SCREEN_W = 480;
static const int SCREEN_H = 272;
static const int SCREEN_PITCH = 512;

static uint32_t __attribute__((aligned(16))) _dlist[262144];
static uint32_t __attribute__((aligned(16))) _clut[256];

#define MAX_SPRITES 256

struct vertex_t {
	int16_t u, v;
	int16_t x, y, z;
};

struct spritetexture_t {
	int w, h;
	uint8_t *bitmap;
	int count;
	struct sys_rect_t *r;
};

static struct spritetexture_t _spritetextures[RENDER_SPR_COUNT];

struct sprite_t {
	struct vertex_t v[2];
	int tex;
};

static struct sprite_t _sprites[MAX_SPRITES];
static int _sprites_count;

static void print_log(FILE *fp, const char *s) {
	static bool first_open = false;
	if (!first_open) {
		fp = fopen("stdout.txt", "w");
		first_open = true;
	} else {
		fp = fopen("stdout.txt", "a");
	}
	fprintf(fp, "%s\n", s);
	fclose(fp);
}

static int exit_callback(int arg1, int arg2, void *common) {
	g_sys.input.quit = true;
	return 0;
}

static int callback_thread(SceSize args, void *argp) {
	const int cb = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cb);
	sceKernelSleepThreadCB();
	return 0;
}

static void psp_init() {
	_buttons = 0;

	memset(&_audio_proc, 0, sizeof(_audio_proc));
	_audio_channel = -1;
	_audio_mutex = 0;

	const int th = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
	if (th >= 0) {
		sceKernelStartThread(th, 0, 0);
	}

	sceKernelDcacheWritebackAll();

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

static void psp_fini() {
	sceGuTerm();
	sceKernelExitGame();
}

static void psp_set_screen_size(int w, int h, const char *caption, int scale, const char *filter, bool fullscreen) {
	sceGuInit();
	sceGuStart(GU_DIRECT, _dlist);

	const int fbSize = SCREEN_PITCH * SCREEN_H * sizeof(uint32_t); // rgba
	uint32_t vramOffset = 0;
	sceGuDrawBuffer(GU_PSM_8888, (void *)vramOffset, SCREEN_PITCH); vramOffset += fbSize;
	sceGuDispBuffer(SCREEN_W, SCREEN_H, (void *)vramOffset, SCREEN_PITCH); vramOffset += fbSize;

	sceGuOffset(2048 - (SCREEN_W / 2), 2048 - (SCREEN_H / 2));
	sceGuViewport(2048, 2048, SCREEN_W, SCREEN_H);
	sceGuScissor(0, 0, SCREEN_W, SCREEN_H);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuDisable(GU_DEPTH_TEST);

	sceGuClearColor(0);
	sceGuClear(GU_COLOR_BUFFER_BIT);

	sceGuFinish();
	sceGuSync(GU_SYNC_WHAT_DONE, GU_SYNC_FINISH);

	sceDisplayWaitVblankStart();
	_fb_offset = (uint32_t)sceGuSwapBuffers();
	sceGuDisplay(GU_TRUE);

	for (int i = 0; i < 256; ++i) {
		_clut[i] = GU_RGBA(0, 0, 0, 255);
	}
	sceKernelDcacheWritebackRange(_clut, sizeof(_clut));
}

static void psp_set_screen_palette(const uint8_t *colors, int offset, int count, int depth) {
	const int shift = 8 - depth;
	for (int i = 0; i < count; ++i) {
		int r = *colors++;
		int g = *colors++;
		int b = *colors++;
		if (shift != 0) {
			r = (r << shift) | (r >> (depth - shift));
			g = (g << shift) | (g >> (depth - shift));
			b = (b << shift) | (b >> (depth - shift));
		}
		_clut[offset + i] = GU_RGBA(r, g, b, 255);
	}
	sceKernelDcacheWritebackRange(_clut, sizeof(_clut));
}

static uint32_t convert_amiga_color(uint16_t color) {
	uint8_t r = (color >> 8) & 15;
	r |= r << 4;
	uint8_t g = (color >> 4) & 15;
	g |= g << 4;
	uint8_t b =  color       & 15;
	b |= b << 4;
	return GU_RGBA(r, g, b, 255);
}

static void psp_set_palette_amiga(const uint16_t *colors, int offset) {
	for (int i = 0; i < 16; ++i) {
		_clut[offset + i] = convert_amiga_color(colors[i]);
	}
	sceKernelDcacheWritebackRange(_clut, sizeof(_clut));
}

static void psp_set_copper_bars(const uint16_t *data) {
}

static void psp_set_palette_color(int i, const uint8_t *colors) {
	_clut[i] = GU_RGBA(colors[0], colors[1], colors[2], 255);
	sceKernelDcacheWritebackRange(_clut, sizeof(_clut));
}

static void psp_fade_in_palette() {
	sceDisplayWaitVblankStart();
	_fb_offset = (uint32_t)sceGuSwapBuffers();
}

static void psp_fade_out_palette() {
	uint32_t *buffer = (uint32_t *)(((uint8_t *)sceGeEdramGetAddr()) + _fb_offset);
	for (int y = 0; y < SCREEN_H; ++y) {
		memset(buffer, 0, SCREEN_W * sizeof(uint32_t));
		buffer += SCREEN_PITCH;
	}
}

static void psp_transition_screen(enum sys_transition_e type, bool open) {
}

static void psp_copy_bitmap(const uint8_t *p, int w, int h) {
	const int dx = (SCREEN_W - w) / 2;
	const int dy = (SCREEN_H - h) / 2;
	uint32_t *buffer = (uint32_t *)(((uint8_t *)sceGeEdramGetAddr()) + _fb_offset);
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			buffer[(dy + y) * SCREEN_PITCH + (dx + x)] = _clut[*p++];
		}
	}
}

static void psp_update_screen() {
	sceGuStart(GU_DIRECT, _dlist);

	sceGuClutMode(GU_PSM_8888, 0, 0xFF, 0);
	sceGuClutLoad(256 / 8, _clut);
	sceGuTexMode(GU_PSM_T8, 0, 0, 0);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);

	sceGuEnable(GU_COLOR_TEST);
	sceGuColorFunc(GU_NOTEQUAL, _clut[0], 0xFFFFFF);

	int prev_tex = -1;

	for (int i = 0; i < _sprites_count; ++i) {
		struct sprite_t *spr = &_sprites[i];

		if (prev_tex != spr->tex) {
			struct spritetexture_t *st = &_spritetextures[spr->tex];
			sceGuTexImage(0, st->w, st->h, st->w, st->bitmap);
			prev_tex = spr->tex;
		}

		struct vertex_t *v = (struct vertex_t *)sceGuGetMemory(2 * sizeof(struct vertex_t));
		v[0] = spr->v[0];
		v[1] = spr->v[1];

		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, v);
	}

	sceGuDisable(GU_COLOR_TEST);

	sceGuFinish();
	sceGuSync(GU_SYNC_WHAT_DONE, GU_SYNC_FINISH);

	sceDisplayWaitVblankStart();
	_fb_offset = (uint32_t)sceGuSwapBuffers();
}

static void psp_shake_screen(int dx, int dy) {
	sceGuOffset(2048 - (SCREEN_W / 2) + dx, 2048 - (SCREEN_H / 2) + dy);
}

static void psp_process_events() {

	g_sys.input.direction = 0;

	static const struct {
		int psp;
		int sys;
	} mapping[] = {
		{ PSP_CTRL_UP, INPUT_DIRECTION_UP },
		{ PSP_CTRL_RIGHT, INPUT_DIRECTION_RIGHT },
		{ PSP_CTRL_DOWN, INPUT_DIRECTION_DOWN },
		{ PSP_CTRL_LEFT, INPUT_DIRECTION_LEFT },
		{ 0, 0 }
	};
	SceCtrlData data;
	sceCtrlPeekBufferPositive(&data, 1);
	for (int i = 0; mapping[i].psp != 0; ++i) {
		if (data.Buttons & mapping[i].psp) {
			g_sys.input.direction |= mapping[i].sys;
		}
	}
	static const int lxMargin = 64;
	if (data.Lx < 127 - lxMargin) {
		g_sys.input.direction |= INPUT_DIRECTION_LEFT;
	} else if (data.Lx > 127 + lxMargin) {
		g_sys.input.direction |= INPUT_DIRECTION_RIGHT;
	}
	static const int lyMargin = 64;
	if (data.Ly < 127 - lyMargin) {
		g_sys.input.direction |= INPUT_DIRECTION_UP;
	} else if (data.Ly > 127 + lyMargin) {
		g_sys.input.direction |= INPUT_DIRECTION_DOWN;
	}

	// PSP_CTRL_CROSS
	g_sys.input.space = (data.Buttons & PSP_CTRL_SQUARE) != 0;
	g_sys.input.jump  = (data.Buttons & PSP_CTRL_CIRCLE) != 0;
	// PSP_CTRL_TRIANGLE

	const uint32_t mask = data.Buttons ^ _buttons;
	if ((data.Buttons & PSP_CTRL_LTRIGGER) & mask) {
	}
	if ((data.Buttons & PSP_CTRL_RTRIGGER) & mask) {
	}
	_buttons = data.Buttons;
}

static void psp_sleep(int duration) {
	sceKernelDelayThread(duration * 1000);
}

static uint32_t psp_get_timestamp() {
	struct timeval tv;
	sceKernelLibcGettimeofday(&tv, 0);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void psp_lock_audio() {
	sceKernelWaitSema(_audio_mutex, 1, 0);
}

static void psp_unlock_audio() {
	sceKernelSignalSema(_audio_mutex, 1);
}

static void audio_callback(void *buf, unsigned int samples, void *param) { // 44100hz S16 stereo
	int16_t buf22khz[samples];
	memset(buf22khz, 0, sizeof(buf22khz));
	psp_lock_audio();
	_audio_proc(_audio_param, (uint8_t *)buf22khz, samples);
	psp_unlock_audio();
	uint32_t *buf44khz = (uint32_t *)buf;
	static int16_t prev;
	for (unsigned int i = 0; i < samples; ++i) {
		const int16_t current = buf22khz[i];
		buf44khz[i] = (current << 16) | (((prev + current) >> 1) & 0xFFFF);
		prev = current;
	}
}

static void psp_start_audio(sys_audio_cb callback, void *param) {
	// sceAudioSetFrequency(AUDIO_FREQ);
	pspAudioInit();
	_audio_proc = callback;
	_audio_param = param;
	_audio_mutex = sceKernelCreateSema("audio_lock", 0, 1, 1, 0);
	_audio_channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, AUDIO_SAMPLES_COUNT, PSP_AUDIO_FORMAT_STEREO);
	pspAudioSetChannelCallback(_audio_channel, audio_callback, 0);
}

static void psp_stop_audio() {
	sceAudioChRelease(_audio_channel);
	sceKernelDeleteSema(_audio_mutex);
	pspAudioEnd();
}

static void render_load_sprites(int spr_type, int count, const struct sys_rect_t *r, const uint8_t *data, int w, int h, uint8_t color_key, bool update_pal) {
	struct spritetexture_t *st = &_spritetextures[spr_type];
	st->r = (struct sys_rect_t *)malloc(count * sizeof(struct sys_rect_t));
	if (st->r) {
		memcpy(st->r, r, count * sizeof(struct sys_rect_t));
		st->count = count;
	}
	st->bitmap = (uint8_t *)memalign(16, w * h);
	if (st->bitmap) {
		memcpy(st->bitmap, data, w * h);
		st->w = w;
		st->h = h;
		sceKernelDcacheWritebackRange(st->bitmap, w * h);
	}
}

static void render_unload_sprites(int spr_type) {
	struct spritetexture_t *st = &_spritetextures[spr_type];
	if (st->r) {
		free(st->r);
	}
	if (st->bitmap) {
		free(st->bitmap);
	}
	memset(st, 0, sizeof(struct spritetexture_t));
}

static void render_add_sprite(int spr_type, int frame, int x, int y, int xflip) {
	if (_sprites_count < MAX_SPRITES) {
		struct sprite_t *spr = &_sprites[_sprites_count];
		struct vertex_t *v = spr->v;

		struct sys_rect_t *r = &_spritetextures[spr_type].r[frame];
		if (!xflip) {
			v[0].u = r->x; v[1].u = r->x + r->w;
		} else {
			v[0].u = r->x + r->w; v[1].u = r->x;
		}
		v[0].v = r->y; v[1].v = r->y + r->h;
		v[0].x = x; v[1].x = x + r->w;
		v[0].y = y; v[1].y = y + r->h;
		v[0].z = 0; v[1].z = 0;

		spr->tex = spr_type;

		++_sprites_count;
	}
}

static void render_clear_sprites() {
	_sprites_count = 0;
}

static void render_set_sprites_clipping_rect(int x, int y, int w, int h) {
	sceGuScissor(x, y, w, h);
}

struct sys_t g_sys = {
	.init = psp_init,
	.fini = psp_fini,
	.set_screen_size = psp_set_screen_size,
	.set_screen_palette = psp_set_screen_palette,
	.set_palette_amiga = psp_set_palette_amiga,
	.set_copper_bars = psp_set_copper_bars,
	.set_palette_color = psp_set_palette_color,
	.fade_in_palette = psp_fade_in_palette,
	.fade_out_palette = psp_fade_out_palette,
	.copy_bitmap = psp_copy_bitmap,
	.update_screen = psp_update_screen,
	.shake_screen = psp_shake_screen,
	.transition_screen = psp_transition_screen,
	.process_events = psp_process_events,
	.sleep = psp_sleep,
	.get_timestamp = psp_get_timestamp,
	.start_audio = psp_start_audio,
	.stop_audio = psp_stop_audio,
	.lock_audio = psp_lock_audio,
	.unlock_audio = psp_unlock_audio,
	.render_load_sprites = render_load_sprites,
	.render_unload_sprites = render_unload_sprites,
	.render_add_sprite = render_add_sprite,
	.render_clear_sprites = render_clear_sprites,
	.render_set_sprites_clipping_rect = render_set_sprites_clipping_rect,
	.print_log = print_log,
};

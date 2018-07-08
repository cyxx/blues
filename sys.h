
#ifndef SYS_H__
#define SYS_H__

#include "intern.h"

#define INPUT_DIRECTION_LEFT  (1 << 0)
#define INPUT_DIRECTION_RIGHT (1 << 1)
#define INPUT_DIRECTION_UP    (1 << 2)
#define INPUT_DIRECTION_DOWN  (1 << 3)

#define SYS_AUDIO_FREQ 22050

struct input_t {
	char quit;
	char escape;
	char space;
	char direction;
	char functions[12];
	char digits[10];
	char alphabet[26];
};

typedef void (*sys_audio_cb)(void *, uint8_t *data, int len);

struct sys_t {
	struct input_t	input;
	int	(*init)();
	void	(*fini)();
	void	(*set_screen_size)(int w, int h, const char *caption);
	void	(*set_screen_palette)(const uint8_t *colors, int);
	void	(*set_palette_amiga)(const uint16_t *colors);
	void	(*set_copper_bars)(const uint16_t *data);
	void	(*fade_in_palette)();
	void	(*fade_out_palette)();
	void	(*update_screen)(const uint8_t *p, int present);
	void	(*process_events)();
	void	(*sleep)(int duration);
	uint32_t	(*get_timestamp)();
	void	(*start_audio)(sys_audio_cb callback, void *param);
	void	(*stop_audio)();
	void	(*lock_audio)();
	void	(*unlock_audio)();
};

extern struct sys_t g_sys;

#endif /* SYS_H__ */

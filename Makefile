
SDL_CFLAGS := `sdl2-config --cflags`
SDL_LIBS   := `sdl2-config --libs` -lSDL2_mixer

SRCS := decode.c fileio.c game.c level.c main.c opcodes.c resource.c screen.c sound.c staticres.c sys_sdl2.c triggers.c unpack.c util.c
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)

CPPFLAGS := -Wall -Wpedantic -MMD $(SDL_CFLAGS) -g

blues: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(SDL_LIBS) -lmodplug

clean:
	rm -f *.o *.d

-include $(DEPS)

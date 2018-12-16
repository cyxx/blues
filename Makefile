
SDL_CFLAGS := `sdl2-config --cflags`
SDL_LIBS   := `sdl2-config --libs`

BB := decode.c fileio.c game.c level.c objects.c resource.c screen.c sound.c staticres.c tiles.c unpack.c
JA := game.c level.c resource.c screen.c sound.c staticres.c unpack.c

BB_SRCS := $(foreach f,$(BB),bb/$f)
JA_SRCS := $(foreach f,$(JA),ja/$f)
SRCS := $(BB_SRCS) $(JA_SRCS)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)

CPPFLAGS := -Wall -Wpedantic -MMD $(SDL_CFLAGS) -I. -g

all: blues bbja

blues: main.o sys_sdl2.o util.o $(BB_SRCS:.c=.o)
	$(CC) $(LDFLAGS) -o $@ $^ $(SDL_LIBS) -lmodplug

bbja: main.o sys_sdl2.o util.o $(JA_SRCS:.c=.o)
	$(CC) $(LDFLAGS) -o $@ $^ $(SDL_LIBS) -lmodplug

clean:
	rm -f $(OBJS) $(DEPS)

-include $(DEPS)

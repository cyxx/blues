
SDL_CFLAGS := `sdl2-config --cflags`
SDL_LIBS   := `sdl2-config --libs`

MODPLUG_LIBS ?= -lmodplug

BB := decode.c game.c level.c objects.c resource.c screen.c sound.c staticres.c tiles.c unpack.c
JA := game.c level.c resource.c screen.c sound.c staticres.c unpack.c
P2 := bosses.c game.c level.c monsters.c resource.c screen.c sound.c staticres.c unpack.c

BB_SRCS := $(foreach f,$(BB),bb/$f)
JA_SRCS := $(foreach f,$(JA),ja/$f)
P2_SRCS := $(foreach f,$(P2),p2/$f)
SRCS := $(BB_SRCS) $(JA_SRCS) $(P2_SRCS)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)

CPPFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wpedantic -MMD $(SDL_CFLAGS) -I. -g

all: blues

game_bb.o: CPPFLAGS += -fvisibility=hidden

game_bb.o: $(BB_SRCS:.c=.o)
	ld -r -o $@ $^
	objcopy --localize-hidden $@

game_ja.o: CPPFLAGS += -fvisibility=hidden

game_ja.o: $(JA_SRCS:.c=.o)
	ld -r -o $@ $^
	objcopy --localize-hidden $@

game_p2.o: CPPFLAGS += -fvisibility=hidden

game_p2.o: $(P2_SRCS:.c=.o)
	ld -r -o $@ $^
	objcopy --localize-hidden $@

blues: main.o sys_sdl2.o util.o game_bb.o game_ja.o game_p2.o
	$(CC) $(LDFLAGS) -o $@ $^ $(SDL_LIBS) $(MODPLUG_LIBS)

clean:
	rm -f $(OBJS) $(DEPS) *.o *.d

-include $(DEPS)

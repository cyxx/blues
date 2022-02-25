
#include <getopt.h>
#include <sys/stat.h>
#include "sys.h"
#include "util.h"

struct options_t g_options;

static const char *DEFAULT_DATA_PATH = ".";

static const int DEFAULT_SCALE_FACTOR = 2;

static const char *DEFAULT_SCALE_FILTER = 0; // nearest pixel sampling

static const char *USAGE =
	"Usage: %s [OPTIONS]...\n"
	"  --datapath=PATH   Path to data files (default '.')\n"
	"  --level=NUM       Start at level NUM\n"
	"  --cheats=MASK     Cheats mask\n"
	"  --startpos=XxY    Start at position (X,Y)\n"
	"  --fullscreen      Enable fullscreen\n"
	"  --scale=N         Graphics scaling factor (default 2)\n"
	"  --filter=NAME     Graphics scaling filter (default 'nearest')\n"
	"  --screensize=WxH  Graphics screen size (default 320x200)\n"
	"  --cga             Enable CGA colors\n"
	"  --dosscroll       Enable DOS style screen scrolling\n"
	"  --hybrid          Enable fuchsia color as in Hybrid crack\n"
	"  --palette=NUM     Pick palette NUM for screen colors\n"
;

static struct game_t *detect_game(const char *data_path) {
#if 0
	extern struct game_t bb_game;
	extern struct game_t ja_game;
	extern struct game_t p2_game;
	static struct game_t *games[] = {
		&bb_game,
		&ja_game,
		&p2_game,
		0
	};
	for (int i = 0; games[i]; ++i) {
		if (games[i]->detect(data_path)) {
			return games[i];
		}
	}
	return 0;
#else
	extern struct game_t game;
	return &game;
#endif
}

int main(int argc, char *argv[]) {
	g_options.start_xpos16 = -1;
	g_options.start_ypos16 = -1;
	g_options.screen_w = 320;
	g_options.screen_h = 200;
	g_options.amiga_copper_bars = true;
	g_options.amiga_colors = true;
	// g_options.amiga_status_bar = true;
	g_options.cga_colors = false;
	g_options.dos_scrolling = false;
	g_options.hybrid_color = false;
	const char *data_path = DEFAULT_DATA_PATH;
	int scale_factor = DEFAULT_SCALE_FACTOR;
	const char *scale_filter = DEFAULT_SCALE_FILTER;
	bool fullscreen = false;
	if (argc == 2) {
		struct stat st;
		if (stat(argv[1], &st) == 0 && S_ISDIR(st.st_mode)) {
			data_path = strdup(argv[1]);
		}
	}
	while (1) {
		static struct option options[] = {
			{ "datapath",   required_argument, 0, 1 },
			{ "level",      required_argument, 0, 2 },
			{ "debug",      required_argument, 0, 3 },
			{ "cheats",     required_argument, 0, 4 },
			{ "startpos",   required_argument, 0, 5 },
			{ "fullscreen", no_argument,       0, 6 },
			{ "scale",      required_argument, 0, 7 },
			{ "filter",     required_argument, 0, 8 },
			{ "screensize", required_argument, 0, 9 },
			{ "cga",        no_argument,       0, 10 },
			{ "dosscroll",  no_argument,       0, 11 },
			{ "hybrid",     no_argument,       0, 12 },
			{ "palette",    required_argument, 0, 13 },
			{ 0, 0, 0, 0 },
		};
		int index;
		const int c = getopt_long(argc, argv, "", options, &index);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 1:
			data_path = strdup(optarg);
			break;
		case 2:
			g_options.start_level = atoi(optarg);
			break;
		case 3:
			g_debug_mask = atoi(optarg);
			break;
		case 4:
			g_options.cheats = atoi(optarg);
			break;
		case 5:
			sscanf(optarg, "%dx%d", &g_options.start_xpos16, &g_options.start_ypos16);
			break;
		case 6:
			fullscreen = true;
			break;
		case 7:
			scale_factor = atoi(optarg);
			break;
		case 8:
			scale_filter = strdup(optarg);
			break;
		case 9:
			if (sscanf(optarg, "%dx%d", &g_options.screen_w, &g_options.screen_h) == 2) {
				// align to tile 16x16
				g_options.screen_w =  (g_options.screen_w + 15) & ~15;
				g_options.screen_h = ((g_options.screen_h + 15) & ~15) + 40; // PANEL_H
			}
			break;
		case 10:
			g_options.cga_colors = true;
			break;
		case 11:
			g_options.dos_scrolling = true;
			break;
		case 12:
			g_options.hybrid_color = true;
			break;
		case 13:
			sscanf(optarg, "%hhd", &g_options.palette);
			fprintf(stdout, "Using palette #%d\n", g_options.palette);
			break;
		default:
			fprintf(stdout, USAGE, argv[0]);
			return -1;
		}
	}
	struct game_t *game = detect_game(data_path);
	if (!game) {
		fprintf(stdout, "No data files found\n");
	} else {
		g_sys.init();
		g_sys.set_screen_size(GAME_SCREEN_W, GAME_SCREEN_H, game->name, scale_factor, scale_filter, fullscreen, g_options.hybrid_color);
		game->run(data_path);
		g_sys.fini();
	}
	if (data_path != DEFAULT_DATA_PATH) {
		free((char *)data_path);
	}
	if (scale_filter != DEFAULT_SCALE_FILTER) {
		free((char *)scale_filter);
	}
	return 0;
}

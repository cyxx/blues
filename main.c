
#include <getopt.h>
#include <sys/stat.h>

#include "fileio.h"
#include "game.h"
#include "resource.h"
#include "sys.h"
#include "util.h"

struct options_t g_options;

static const char *DEFAULT_DATA_PATH = ".";

static const char *USAGE =
	"Usage: %s [OPTIONS]...\n"
	"  --datapath=PATH   Path to data files (default '.')\n"
	"  --level=NUM       Start at level NUM\n"
;

int main(int argc, char *argv[]) {
	g_options.start_xpos16 = -1;
	g_options.start_ypos16 = -1;
	g_options.amiga_copper_bars =  true;
	g_options.amiga_colors = true;
	const char *data_path = DEFAULT_DATA_PATH;
	if (argc == 2) {
		struct stat st;
		if (stat(argv[1], &st) == 0 && S_ISDIR(st.st_mode)) {
			data_path = strdup(argv[1]);
		}
	}
	while (1) {
		static struct option options[] = {
			{ "datapath", required_argument, 0, 1 },
			{ "level",    required_argument, 0, 2 },
			{ "debug",    required_argument, 0, 3 },
			{ "cheats",   required_argument, 0, 4 },
			{ "startpos", required_argument, 0, 5 },
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
		default:
			fprintf(stdout, USAGE, argv[0]);
			return -1;
		}
	}
	fio_init(data_path);
	res_init();
	g_sys.init();
	sound_init(SYS_AUDIO_FREQ);
	game_main();
	sound_fini();
	g_sys.fini();
	res_fini();
	fio_fini();
	if (data_path != DEFAULT_DATA_PATH) {
		free((char *)data_path);
	}
	return 0;
}

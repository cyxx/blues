
#include "resource.h"
#include "unpack.h"
#include "util.h"

static const int BACKGROUND_SIZE = 320 * 200;

static const char *_datapath;

struct resource_t g_res;

int g_uncompressed_size;

uint8_t *load_file(const char *filename) {
	FILE *fp = fopen_nocase(_datapath, filename);
	if (fp) {
		print_debug(DBG_RESOURCE, "Loading '%s'...", filename);
		uint8_t *p = unpack(fp, &g_uncompressed_size);
		print_debug(DBG_RESOURCE, "Uncompressed size %d", g_uncompressed_size);
		fclose(fp);
		return p;
	}
	print_error("Unable to open '%s'", filename);
	return 0;
}

static void detect_dos_demo() {
	FILE *fp = fopen_nocase(_datapath, "JOYSTICK.SQZ");
	if (fp) {
		g_res.dos_demo = true;
		fclose(fp);
	}
}

void res_init(const char *path, int vga_size) {
	_datapath = path;

	detect_dos_demo();

	g_res.maps     = load_file("MAP.SQZ");
	g_res.motif    = load_file("MOTIF.SQZ");
	g_res.allfonts = load_file("ALLFONTS.SQZ");
	g_res.sprites  = load_file("SPRITES.SQZ");
	g_res.frontdat = load_file("FRONT.SQZ");
	g_res.frontlen = g_uncompressed_size;
	g_res.uniondat = load_file("UNION.SQZ");
	g_res.unionlen = g_uncompressed_size;
	g_res.keyb     = load_file("KEYB.SQZ");
	g_res.keyblen  = g_uncompressed_size;

	g_res.vga = (uint8_t *)malloc(vga_size);
	if (!g_res.vga) {
		print_error("Failed to allocate vga buffer, %d bytes", vga_size);
	}

	g_res.background = (uint8_t *)malloc(BACKGROUND_SIZE);
	if (!g_res.background) {
		print_error("Failed to allocate background buffer, %d bytes", BACKGROUND_SIZE);
	}

	if (!g_res.dos_demo) {
		g_res.samples = load_file("SAMPLE.SQZ");
	}

	g_res.spr_monsters_offset = 312;
	g_res.spr_monsters_count = g_res.dos_demo ? 446 : 461;
}

void res_fini() {
	free(g_res.maps);
	g_res.maps = 0;
	free(g_res.motif);
	g_res.motif = 0;
	free(g_res.allfonts);
	g_res.allfonts = 0;
	free(g_res.sprites);
	g_res.sprites = 0;
	free(g_res.frontdat);
	g_res.frontdat = 0;
	free(g_res.uniondat);
	g_res.uniondat = 0;
	free(g_res.vga);
	g_res.vga = 0;
	free(g_res.background);
	g_res.background = 0;
	free(g_res.samples);
	g_res.samples = 0;
}

void load_leveldat(const uint8_t *p, struct level_t *level) {
	memcpy(g_res.level.tile_attributes0, p, 256); p += 256;
	memcpy(g_res.level.tile_attributes1, p, 256); p += 256;
	memcpy(g_res.level.tile_attributes2, p, 256); p += 256;
	g_res.level.scrolling_top = READ_LE_UINT16(p); p += 2;
	g_res.level.start_x_pos = READ_LE_UINT16(p); p += 2;
	g_res.level.start_y_pos = READ_LE_UINT16(p); p += 2;
	g_res.level.tilemap_w = READ_LE_UINT16(p); p += 2;
	g_res.level.scrolling_mask = *p++;
	for (int i = 0; i < 256; ++i) {
		g_res.level.front_tiles_lut[i] = READ_LE_UINT16(p); p += 2;
	}
	for (int i = 0; i < MAX_LEVEL_GATES; ++i) {
		struct level_gate_t *gate = &g_res.level.gates_tbl[i];
		gate->enter_pos = READ_LE_UINT16(p); p += 2;
		gate->tilemap_pos = READ_LE_UINT16(p); p += 2;
		gate->dst_pos = READ_LE_UINT16(p); p += 2;
		gate->scroll_flag = *p++;
	}
	for (int i = 0; i < MAX_LEVEL_COLUMNS; ++i) {
		struct level_column_t *column = &g_res.level.columns_tbl[i];
		column->tilemap_pos = READ_LE_UINT16(p); p += 2;
		column->w = *p++;
		column->h = *p++;
		column->trigger_pos = READ_LE_UINT16(p); p += 2;
		column->tiles_offset_buf = READ_LE_UINT16(p); p += 2;
		column->y_target = *p++;
		column->unk9 = *p++;
	}
	const uint8_t *monster_attr = p;
	int monsters_count = 0;
	while (*p < 50) {
		const uint8_t len = p[0];
		const uint8_t type = p[1] & 0x7F;
		const uint16_t spr_num = READ_LE_UINT16(p + 2);
		print_debug(DBG_RESOURCE, "monster %d len %d type %d spr %d", monsters_count, len, type, spr_num);
		assert(monsters_count < MAX_LEVEL_MONSTERS);
		struct level_monster_t *m = &g_res.level.monsters_tbl[monsters_count++];
		m->len = len;
		m->type = p[1];
		m->spr_num = spr_num;
		m->flags = p[4];
		m->energy = p[5];
		m->respawn_ticks = p[6];
		m->current_tick = p[7];
		m->score = p[8];
		m->x_pos = READ_LE_UINT16(p + 0x9);
		m->y_pos = READ_LE_UINT16(p + 0xB);
		switch (type) { /* movement */
		case 0:
			assert(len == 13);
			break;
		case 1:
			assert(len == 13);
			break;
		case 2: /* vertical (eg. spider) */
			assert(len == 15);
			m->type2.y_range = p[0xD];
			m->type2.unkE = p[0xE];
			break;
		case 3:
			assert(len == 14);
			m->type3.unkD = p[0xD];
			break;
		case 4: /* rotate (eg. spider) */
			assert(len == 17);
			m->type4.radius = p[0xD];
			m->type4.unkE = p[0xE];
			m->type4.angle = p[0xF];
			m->type4.angle_step = p[0x10];
			break;
		case 5:
			assert(len == 16);
			m->type5.x_range = p[0xD];
			m->type5.y_range = p[0xE];
			m->type5.unkF = p[0xF];
			break;
		case 6:
			assert(len == 21);
			m->type6.x_range = p[0xD];
			m->type6.pos = 0;
			break;
		case 7:
			assert(len == 15);
			m->type7.unkD = p[0xD];
			m->type7.unkE = p[0xE];
			break;
		case 8: /* jump */
			assert(len == 17);
			m->type8.x_range = p[0xD];
			m->type8.y_step = p[0xE];
			m->type8.x_step = p[0xF];
			m->type8.y_range = p[0x10];
			break;
		case 9: /* horizontal */
			assert(len == 19);
			m->type9.unkD = READ_LE_UINT16(p + 0xD);
			m->type9.unkF = READ_LE_UINT16(p + 0xF);
			m->type9.x_step = p[0x11];
			m->type9.x_dist = p[0x12];
			break;
		case 10: /* come out of the ground */
			assert(len == 14);
			m->type10.unkD = p[0xD];
			break;
		case 11:
			assert(len == 16);
			m->type11.unkD = p[0xD];
			m->type11.unkE = p[0xE];
			m->type11.unkF = p[0xF];
			break;
		case 12:
			assert(len == 15);
			m->type12.unkD = p[0xD];
			break;
		default:
			print_warning("Unhandled monster type %d len %d", type, len);
			break;
		}
		p += len;
	}
	g_res.level.monsters_count = monsters_count;
	p = monster_attr + 0x800;
	g_res.level.items_spr_num_offset = READ_LE_UINT16(p); p += 2;
	g_res.level.monsters_spr_num_offset = READ_LE_UINT16(p); p += 2;
	for (int i = 0; i < MAX_LEVEL_BONUSES; ++i) {
		struct level_bonus_t *bonus = &g_res.level.bonuses_tbl[i];
		bonus->tile_num0 = *p++;
		bonus->tile_num1 = *p++;
		bonus->count = *p++;
		bonus->pos = READ_LE_UINT16(p); p += 2;
	}
	memcpy(g_res.level.tile_attributes3, p, 256); p += 256;
	for (int i = 0; i < MAX_LEVEL_ITEMS; ++i) {
		struct level_item_t *item = &g_res.level.items_tbl[i];
		item->x_pos = READ_LE_UINT16(p); p += 2;
		item->y_pos = READ_LE_UINT16(p); p += 2;
		item->spr_num = READ_LE_UINT16(p); p += 2;
		item->y_delta = *p++;
	}
	for (int i = 0; i < MAX_LEVEL_PLATFORMS; ++i) {
		struct level_platform_t *platform = &g_res.level.platforms_tbl[i];
		platform->x_pos = READ_LE_UINT16(p); p += 2;
		platform->y_pos = READ_LE_UINT16(p); p += 2;
		platform->spr_num = READ_LE_UINT16(p); p += 2;
		platform->flags = *p++;
		const int type = platform->flags & 15;
		if (type == 8) {
			platform->type8.y_velocity = *p++;
			platform->type8.unk8 = *p++;
			platform->type8.unk9 = *p++;
			platform->type8.state = *p++;
			platform->type8.y_delta = READ_LE_UINT16(p); p += 2;
			platform->type8.counter = *p++;
			++p;
		} else {
			platform->other.max_velocity = *p++;
			++p;
			platform->other.unk9 = *p++;
			platform->other.unkA = READ_LE_UINT16(p); p += 2;
			platform->other.counter = READ_LE_UINT16(p); p += 2;
			platform->other.velocity = *p++;
		}
	}
	g_res.level.boss_xmin = READ_LE_UINT16(p); p += 2;
	g_res.level.boss_xmax = READ_LE_UINT16(p); p += 2;
	g_res.level.boss_speed = *p++;
	g_res.level.boss_energy = READ_LE_UINT16(p); p += 2;
	g_res.level.boss_state = *p++;
	g_res.level.boss_x_pos = READ_LE_UINT16(p); p += 2;
	g_res.level.boss_y_pos = READ_LE_UINT16(p); p += 2;
	const int total = p - g_res.leveldat;
	print_debug(DBG_RESOURCE, "level total offset %d", total);
}

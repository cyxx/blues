
#include "resource.h"
#include "sys.h"
#include "unpack.h"
#include "util.h"

struct resource_data_t g_res;

static bool file_exists(const char *filename) {
	FILE *fp = fopen_nocase(g_res.datapath, filename);
	if (fp) {
		fclose(fp);
		return true;
	}
	return false;
}

void res_init(const char *datapath, int vga_size) {
	g_res.datapath = datapath;

	static const int SQL_SIZE = 640 * 25;
	g_res.sql = (uint8_t *)malloc(SQL_SIZE);
	if (!g_res.sql) {
		print_error("Failed to allocate sql buffer, %d bytes", SQL_SIZE);
	}
	static const int SPR_SQV_SIZE = 64000;
	g_res.spr_sqv = (uint8_t *)malloc(SPR_SQV_SIZE);
	if (!g_res.spr_sqv) {
		print_error("Failed to allocate sprite buffer, %d bytes", SPR_SQV_SIZE);
	}
	static const int AVT_SQV_SIZE = 437 * 16;
	g_res.avt_sqv = (uint8_t *)malloc(AVT_SQV_SIZE);
	if (!g_res.avt_sqv) {
		print_error("Failed to allocate avt buffer, %d bytes", AVT_SQV_SIZE);
	}
	const int tmp_size = 32000 + vga_size;
	g_res.tmp = (uint8_t *)malloc(tmp_size);
	if (!g_res.tmp) {
		print_error("Failed to allocate tmp buffer, %d bytes", tmp_size);
	}
	g_res.vga = (uint8_t *)malloc(vga_size);
	if (!g_res.vga) {
		print_error("Failed to allocate vga buffer, %d bytes", vga_size);
	}
	g_res.vga_size = vga_size;
	static const int TILES_SIZE = 640 * 200;
	g_res.tiles = (uint8_t *)malloc(TILES_SIZE);
	if (!g_res.tiles) {
		print_error("Failed to allocate tiles buffer, %d bytes", TILES_SIZE);
	}
	static const char *filename = "sound";
	if (file_exists(filename)) {
		g_res.snd = (uint8_t *)malloc(SOUND_SIZE);
		if (!g_res.snd) {
			print_warning("Failed to allocate sound buffer, %d bytes", SOUND_SIZE);
		} else {
			read_file(filename, g_res.snd, SOUND_SIZE);
		}
	}
	if (file_exists("demomag.sql")) {
		g_res.dos_demo = true;
	}
	if (file_exists("mag.tbl")) {
		g_res.amiga_data = true;
	}
}

void res_fini() {
	free(g_res.sql);
	g_res.sql = 0;
	free(g_res.spr_sqv);
	g_res.spr_sqv = 0;
	free(g_res.avt_sqv);
	g_res.avt_sqv = 0;
	free(g_res.tmp);
	g_res.tmp = 0;
	free(g_res.vga);
	g_res.vga = 0;
	free(g_res.tiles);
	g_res.tiles = 0;
	free(g_res.snd);
	g_res.snd = 0;
}

int read_file(const char *filename, uint8_t *dst, int size) {
	FILE *fp = fopen_nocase(g_res.datapath, filename);
	if (!fp) {
		print_error("Unable to open '%s'", filename);
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	const int filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (size > 0 && size != filesize) {
		print_error("Unexpected '%s' file size %d (%d)", filename, filesize, size);
	} else if (fread(dst, 1, filesize, fp) != filesize) {
		print_error("Failed to read %d bytes from file '%s'", filesize, filename);
	}
	fclose(fp);
	return filesize;
}

int read_compressed_file(const char *filename, uint8_t *dst) {
	FILE *fp = fopen_nocase(g_res.datapath, filename);
	if (!fp) {
		print_error("Unable to open '%s'", filename);
		return 0;
	}
	const int size = unpack(fp, dst);
	fclose(fp);
	return size;
}

extern const uint8_t dither_cga_table[];

static void dither_cga(int num, uint8_t *dst) {
	const uint8_t *p = dither_cga_table + num * 32;
	for (int i = 0; i < 16; ++i, p += 2) {
		dst[i]        = p[0] & 3; // even scanline
		dst[0x10 + i] = p[1] & 3; // odd scanline
	}
}

static void copy_cga(uint8_t *dst, int dst_pitch, uint8_t *dither_lut) {
	for (int y = 0; y < 200; ++y) {
		const uint8_t *p = dither_lut + (y & 1) * 0x10;
		for (int x = 0; x < 320; ++x) {
			dst[x] = p[dst[x]];
		}
		dst += dst_pitch;
	}
}

static void decode_bitplane_scanline(const uint8_t *src, int depth, int w, uint8_t *dst) {
	const int plane_size = w / depth;
	for (int x = 0; x < plane_size; ++x) {
		for (int i = 0; i < 8; ++i) {
			int color = 0;
			const int mask = 1 << (7 - i);
			for (int bit = 0; bit < depth; ++bit) {
				if (src[bit * plane_size] & mask) {
					color |= 1 << bit;
				}
			}
			dst[i] = color;
		}
		++src;
		dst += 8;
	}
}

static void load_iff(const uint8_t *data, uint32_t size, uint8_t *dst, int dst_pitch, int cga_dither_pattern) {
	print_debug(DBG_RESOURCE, "load_iff size %d", size);
	if (data && memcmp(data, "FORM", 4) == 0) {
		int offset = 12;
		while (offset < size) {
			const uint8_t *buf = data + offset;
			const int len = (READ_BE_UINT32(buf + 4) + 1) & ~1;
			print_debug(DBG_RESOURCE, "tag '%c%c%c%c' len %d", buf[0], buf[1], buf[2], buf[3], len);
			if (memcmp(buf, "BMHD", 4) == 0) {
				buf += 8;
				const int w = READ_BE_UINT16(buf);
				const int h = READ_BE_UINT16(buf + 2);
				const int planes = buf[8];
				const int compression = buf[10];
				print_debug(DBG_RESOURCE, "w %d h %d planes %d compression %d", w, h, planes, compression);
				if (w != 320 || h < 200) {
					print_error("Unhandled LBM dimensions %d,%d", w, h);
					return;
				}
				if (planes != 4) {
					print_error("Unhandled LBM planes count %d", planes);
					return;
				}
				if (compression != 1) {
					print_error("Unhandled LBM compression %d", compression);
					return;
				}
			} else if (memcmp(buf, "CMAP", 4) == 0) {
				buf += 8;
				for (int i = 0; i < len; ++i) {
					g_res.palette[i] = buf[i];
				}
			} else if (memcmp(buf, "BODY", 4) == 0) {
				buf += 8;
				int offset = 0;
				int i = 0;
				int y = 0;
				uint8_t scanline[160];
				while (i < len && offset < 32000) {
					int code = (int8_t)buf[i++];
					if (code != -128) {
						const int scanline_offset = offset % 160;
						if (code < 0) {
							code = 1 - code;
							memset(scanline + scanline_offset, buf[i], code);
							++i;
						} else {
							++code;
							memcpy(scanline + scanline_offset, buf + i, code);
							i += code;
						}
						offset += code;
						if ((offset % 160) == 0) {
							decode_bitplane_scanline(scanline, 4, 160, dst + y * dst_pitch);
							++y;
						}
					}

				}
				print_debug(DBG_RESOURCE, "scanlines %d", y);
			}
			offset += 8 + len;
		}
	}
	if (!(cga_dither_pattern < 0)) {
		uint8_t cga_lut[16 * 2];
		dither_cga(cga_dither_pattern, cga_lut);
		copy_cga(dst, dst_pitch, cga_lut);
	}
}

void load_avt(const char *filename, uint8_t *dst, int offset, int dither_pattern) {
	read_compressed_file(filename, dst);
	const uint8_t *ptr = dst;
	const int count = READ_LE_UINT16(ptr); ptr += 6;
	print_debug(DBG_RESOURCE, "avt count %d", count);
	for (int i = 0; i < count; ++i) {
		g_res.avt[offset + i] = ptr;
		ptr += 132;
	}
	g_res.avt_count = count;
	if (!(dither_pattern < 0)) {
		dither_cga(dither_pattern, g_res.cga_lut_avt);
	}
}

static const uint8_t *trigger_lookup_table1(uint8_t num) {
	extern const uint8_t *level_triggersdata_3356[];
	assert(num < 4);
	return level_triggersdata_3356[num];
}

static const uint8_t *trigger_lookup_table2(uint8_t num) {
	if (num == 255) {
		return 0;
	}
	extern uint8_t *level_tilesdata_1e8c[];
	if (num < 128) {
		assert(num < 86);
		return level_tilesdata_1e8c[num];
	}
	extern uint8_t *level_tilesdata_1fe8[];
	num -= 128;
	assert(num < 17);
	return level_tilesdata_1fe8[num];
}

static const uint8_t *trigger_lookup_table3(uint8_t num) {
	if (num == 255) {
		return 0;
	}
	extern const uint8_t *level_triggersdata_2030[];
	assert(num < 61);
	return level_triggersdata_2030[num];
}

void load_bin(const char *filename) {
	uint8_t bin[MAX_TRIGGERS * 10];
	read_file(filename, bin, MAX_TRIGGERS * 10);
	const uint8_t *p = bin;
	for (int i = 0; i < MAX_TRIGGERS; ++i) {
		struct trigger_t *t = &g_res.triggers[i];
		t->tile_type = p[0];
		t->tile_flags = p[1];
		t->op_func = p[2];
		t->op_table1 = trigger_lookup_table1(p[3]);
		t->op_table2 = trigger_lookup_table2(p[4]);
		t->unk10 = p[5];
		t->op_table3 = trigger_lookup_table3(p[6]);
		t->unk16 = p[7];
		t->tile_index = p[8];
		t->foreground_tile_num = p[9];
		p += 10;
	}
}

void load_blk(const char *filename) {
	read_file(filename, g_res.tiles, 256 * 16 * 8);
}

void load_ck(const char *filename, uint16_t offset, int dither_pattern) {
	const int size = read_compressed_file(filename, g_res.tmp);
	switch (offset) {
	case 0x6000: // page3
		offset = 0;
		break;
	case 0x8000: // page4
		offset = 320;
		break;
	default:
		print_error("Unexpected offset 0x%x in load_ck()", offset);
		break;
	}
	load_iff(g_res.tmp, size, g_res.tiles + offset, 640, dither_pattern);
}

void load_img(const char *filename, int screen_w, int dither_pattern) {
	int size;
	const char *ext = strrchr(filename, '.');
	if (ext && strcmp(ext + 1, "lbm") == 0) {
		size = read_file(filename, g_res.tmp, 0);
	} else {
		size = read_compressed_file(filename, g_res.tmp);
	}
	assert(size <= 32000);
	load_iff(g_res.tmp, size, g_res.vga, 320, dither_pattern);
	g_sys.copy_bitmap(g_res.vga, 320, 200);
	if (dither_pattern < 0) {
		g_sys.set_screen_palette(g_res.palette, 0, 16, 8);
	}
}

void load_m(const char *filename) {
	const int filesize = read_file(filename, g_res.tmp, 0);
	const int count = READ_BE_UINT16(g_res.tmp);
	assert(filesize == 2 + count * 32);
}

static int load_spr_helper(int offset, const uint8_t *ptr, uint16_t (*read16)(const uint8_t *), int depth) {
	const int count = read16(ptr); ptr += 6;
	print_debug(DBG_RESOURCE, "spr count %d", count);
	assert(offset + count <= MAX_SPR_FRAMES);
	for (int i = 0; i < count; ++i) {
		g_res.spr_frames[offset + i] = ptr;
		const int h = read16(ptr - 4);
		const int w = read16(ptr - 2);
		assert((w & 3) == 0);
		const int size = h * w * depth / 8;
		print_debug(DBG_RESOURCE, "sprite %d, dim %d,%d size %d", i, w, h, size);
		ptr += size + 4;
	}
	return count;
}

void load_spr(const char *filename, uint8_t *dst, int offset) {
	read_file(filename, dst, 0);
	// player sprites use 8 colors
	const int depth = (offset == 0) ? 3 : 4;
	const int count = load_spr_helper(offset, dst, READ_BE_UINT16, depth);
	g_res.spr_count = offset + count;
	// convert to little endian
	uint8_t *ptr = dst;
	for (int i = 0; i < count; ++i) {
		const int h = READ_BE_UINT16(ptr + 2);
		ptr[2] = h;
		ptr[3] = 0;
		const int w = READ_BE_UINT16(ptr + 4);
		ptr[4] = w;
		ptr[5] = 0;
		assert((w & 3) == 0);
		const int size = h * w * depth / 8;
		ptr += size + 4;
	}
}

void load_sqv(const char *filename, uint8_t *dst, int offset, int dither_pattern) {
	read_compressed_file(filename, dst);
	const int count = load_spr_helper(offset, dst, READ_LE_UINT16, 4);
	g_res.spr_count = offset + count;
	if (!(dither_pattern < 0)) {
		dither_cga(dither_pattern, g_res.cga_lut_sqv);
	}
}

void load_sql(const char *filename) {
	read_compressed_file(filename, g_res.sql);
}

uint8_t *lookup_sql(int x, int y) {
	return g_res.sql + y * 128 + x;
}

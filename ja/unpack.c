
/* uncompress data packed with DIET by T.Matsumoto */

#include "unpack.h"
#include "util.h"

struct unpack_eat_t {
	FILE *fp;
	uint8_t len;
	uint16_t bits;
	uint8_t *dst;
};

static struct unpack_eat_t g_unpack;

static uint16_t fread_le16(FILE *fp) {
	const uint16_t val = fgetc(fp);
	return val | (fgetc(fp) << 8);
}

static int next_bit(struct unpack_eat_t *u) {
	const int bit = (u->bits & (1 << (16 - u->len))) != 0;
	--u->len;
	if (u->len == 0) {
		u->bits = fread_le16(u->fp);
		u->len = 16;
	}
	return bit;
}

static int zero_bits(struct unpack_eat_t *u, int count) {
	int i = 0;
	for (; i < count; ++i) {
		if (next_bit(u)) {
			break;
		}
	}
	return i;
}

static uint8_t get_bits(struct unpack_eat_t *u, int count) {
	assert(count < 8);
	uint8_t val = 0;
	for (int i = 0; i < count; ++i) {
		val = (val << 1) | next_bit(u);
	}
	return val;
}

static void copy_reference(struct unpack_eat_t *u, int count, int offset_hi, int offset_lo) {
	const int16_t offset = offset_hi * 256 + offset_lo;
	for (int i = 0; i < count; ++i) {
		const uint8_t value = u->dst[offset];
		*u->dst++ = value;
	}
}

static int unpack_eat(struct unpack_eat_t *u, uint8_t *output_buffer, int output_buffer_size) {
	uint8_t buffer[17];
	const int header_size = fread(buffer, 1, sizeof(buffer), u->fp);
	if (header_size != 17 || READ_LE_UINT16(buffer + 4) != 0x899D || READ_LE_UINT16(buffer + 6) != 0x6C64) {
		print_error("Unexpected signature for .eat file");
		return 0;
	}
	const uint16_t crc = READ_LE_UINT16(buffer + 12);
	const int output_size = (buffer[14] << 14) + READ_LE_UINT16(buffer + 15);
	print_debug(DBG_UNPACK, "uncompressed size %d crc 0x%04x", output_size, crc);
	if (output_size > output_buffer_size) {
		print_error("Invalid output buffer size %d, need %d", output_buffer_size, output_size);
		return 0;
	}

	u->dst = output_buffer;
	u->len = 16;
	u->bits = fread_le16(u->fp);

	while (1) {
		while (next_bit(u)) {
			*u->dst++ = fgetc(u->fp);
		}
		const int b = next_bit(u);
		const int offset_lo = fgetc(u->fp);
		if (b) {
			int offset_hi = 0xFE | next_bit(u);
			if (!next_bit(u)) {
				int i = 1;
				for (; i < 4 && !next_bit(u); ++i) {
					offset_hi = (offset_hi << 1) | next_bit(u);
				}
				offset_hi -= (1 << i);
			}
			const int n = zero_bits(u, 4);
			if (n != 4) {
				copy_reference(u, n + 3, offset_hi, offset_lo);
			} else if (next_bit(u)) {
				copy_reference(u, next_bit(u) + 7, offset_hi, offset_lo);
			} else if (!next_bit(u)) {
				copy_reference(u, get_bits(u, 3) + 9, offset_hi, offset_lo);
			} else {
				copy_reference(u, fgetc(u->fp) + 17, offset_hi, offset_lo);
			}
		} else {
			if (next_bit(u)) {
				const int offset_hi = (0xF8 | get_bits(u, 3)) - 1;
				copy_reference(u, 2, offset_hi, offset_lo);
			} else if (offset_lo == 0xFF) {
				break;
			} else {
				copy_reference(u, 2, 0xFF, offset_lo);
			}
		}
	}
	assert((u->dst - output_buffer) == output_size);
	return output_size;
}

int unpack(const char *filename, uint8_t *dst, int size) {
	int uncompressed_size = 0;
	FILE *fp = fopen(filename, "rb");
	if (fp) {
		memset(&g_unpack, 0, sizeof(struct unpack_eat_t));
		g_unpack.fp = fp;
		uncompressed_size = unpack_eat(&g_unpack, dst, size);
		fclose(fp);
	} else {
		print_error("Unable to open '%s'", filename);
	}
	return uncompressed_size;
}


#include "util.h"

struct unpack_eat_t {
	FILE *fp;
	uint8_t len;
	uint16_t bits;
	uint8_t *dst;
};

static struct unpack_eat_t uneat;

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

static int unpack_eat(FILE *in, struct unpack_eat_t *u) {
	uint8_t buffer[17];
	const int header_size = fread(buffer, 1, sizeof(buffer), in);
	if (header_size != 17 || READ_LE_UINT16(buffer + 4) != 0x899D || READ_LE_UINT16(buffer + 6) != 0x6C64) {
		print_error("Unexpected signature for .eat file");
		return 0;
	}
	const uint16_t crc = READ_LE_UINT16(buffer + 12);
	const int output_size = (buffer[14] << 14) + READ_LE_UINT16(buffer + 15);
	print_debug(DBG_UNPACK, "uncompressed size %d crc 0x%04x", output_size, crc);

	uint8_t *output_buffer = (uint8_t *)malloc(output_size);
	if (!output_buffer) {
		print_error("Failed to allocate EAT unpack buffer, %d bytes", output_size);
		return 0;
	}
	u->fp = in;
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
	u->dst = output_buffer;
	return output_size;
}

struct unpack_sqz_t {
	uint16_t top_code;
	uint8_t code_size;
	uint16_t new_codes;
	uint8_t *dst;
	int bits_left;
	uint32_t current_bits;
	uint8_t last_code;
	uint16_t previous_code;
	uint16_t prefix[0x1000];
	uint8_t str[0x1000];
	uint8_t stack[0x1000];
};

#define SQZ_CODE_WIDTH 9
#define SQZ_CODE_BASE (1 << (SQZ_CODE_WIDTH - 1))

static const int SQZ_CLEAR_CODE = SQZ_CODE_BASE;
static const int SQZ_END_CODE   = SQZ_CODE_BASE + 1;
static const int SQZ_NEW_CODES  = SQZ_CODE_BASE + 2;

static struct unpack_sqz_t unsqz;

static uint16_t unpack_sqz_get_bits(FILE *in, struct unpack_sqz_t *u, int count) {
	u->current_bits <<= 8;
	u->current_bits |= fgetc(in);
	u->bits_left += 8;
	if (u->bits_left < count) {
		u->current_bits <<= 8;
		u->current_bits |= fgetc(in);
		u->bits_left += 8;
	}
	const uint32_t code = u->current_bits >> (u->bits_left - count);
	u->bits_left -= count;
	u->current_bits &= (1 << u->bits_left) - 1;
	return code;
}

static uint16_t unpack_sqz_get_code(FILE *in, struct unpack_sqz_t *u) {
	if (u->top_code == u->new_codes && u->code_size != 12) {
		++u->code_size;
		u->top_code <<= 1;
	}
	return unpack_sqz_get_bits(in, u, u->code_size);
}

static uint16_t unpack_sqz_clear_code(FILE *in, struct unpack_sqz_t *u) {
	u->top_code = 1 << SQZ_CODE_WIDTH;
	u->code_size = SQZ_CODE_WIDTH;
	u->new_codes = SQZ_NEW_CODES;
	const uint16_t code = unpack_sqz_get_code(in, u);
	if (code != SQZ_END_CODE) {
		u->previous_code = code;
		*u->dst++ = u->last_code = code & 255;
	}
	return code;
}

static int unpack_sqz(FILE *in, struct unpack_sqz_t *u) {
	uint8_t buf[4];
	fread(buf, 1, sizeof(buf), in);
	assert((buf[1] & 0xF0) == 0x10);
	const int output_size = ((buf[0] & 15) << 16) | READ_LE_UINT16(buf + 2);
	print_debug(DBG_UNPACK, "SQZ uncompressed size %d", output_size);
	uint8_t *output_buffer = (uint8_t *)malloc(output_size);
	if (!output_buffer) {
		print_error("Failed to allocate SQZ unpack buffer, %d bytes", output_size);
		return 0;
	}
	u->dst = output_buffer;
	uint16_t code = unpack_sqz_clear_code(in, u);
	assert(code != SQZ_END_CODE);
	while (1) {
		code = unpack_sqz_get_code(in, u);
		if (code == SQZ_END_CODE) {
			print_debug(DBG_UNPACK, "lzw end code");
			break;
		} else if (code == SQZ_CLEAR_CODE) {
			print_debug(DBG_UNPACK, "lzw clear code");
			unpack_sqz_clear_code(in, u);
			continue;
		}
		const uint16_t current_code = code;
		uint8_t *sp = u->stack;
		if (u->new_codes <= code) {
			*sp++ = u->last_code;
			code = u->previous_code;
		}
		while (code >= SQZ_CODE_BASE) {
			*sp++ = u->str[code];
			code = u->prefix[code];
		}
		*sp++ = u->last_code = code & 255;
		do {
			--sp;
			*u->dst++ = *sp;
		} while (sp != u->stack);
		const uint16_t index = u->new_codes;
		if (index < 0x1000) {
			u->str[index] = u->last_code;
			u->prefix[index] = u->previous_code;
			++u->new_codes;
		}
		u->previous_code = current_code;
	}
	const int count = (u->dst - output_buffer);
	print_debug(DBG_UNPACK, "lzw output size %d (expected %d)", count, output_size);
	assert(count == output_size);
	u->dst = output_buffer;
	return count;
}

struct unpack_sqv_t {
	uint8_t dict_buf[0x200 * 2];
	uint8_t rd[0x1000];
	uint8_t *dst;
};

static struct unpack_sqv_t unsqv;

static int unpack_sqv(FILE *in, struct unpack_sqv_t *u) {

	fread(u->rd, 1, 6, in);
	const int uncompressed_size = (READ_LE_UINT16(u->rd) << 16) + READ_LE_UINT16(u->rd + 2);
	const int dict_len = READ_LE_UINT16(u->rd + 4);
	print_debug(DBG_UNPACK, "SQV uncompressed size %d dict_len %d", uncompressed_size, dict_len);
	assert(dict_len <= 0x400);
	fread(u->dict_buf, 1, dict_len, in);

	uint8_t *output_buffer = (uint8_t *)malloc(uncompressed_size);
	if (!output_buffer) {
		print_error("Failed to allocate SQV unpack buffer, %d bytes", uncompressed_size);
		return 0;
	}

	uint8_t *dst = output_buffer;

	const uint8_t *src = u->rd;
	int bits_count = 1;
	int bytes_count = 2;
	int state = 0;
	int count = 0;
	uint8_t code, prev = 0;
	uint16_t bits = 0;
	uint16_t val = 0;
	while ((dst - output_buffer) < uncompressed_size) {
		--bits_count;
		if (bits_count == 0) {
			bytes_count -= 2;
			if (bytes_count == 0) {
				bytes_count = fread(u->rd, 1, 0x1000, in);
				if (bytes_count == 0) {
					break;
				}
				bytes_count += (bytes_count & 1);
				src = u->rd;
			}
			bits = READ_BE_UINT16(src); src += 2;
			bits_count = 16;
		}
		const bool carry = (bits & 0x8000) != 0;
		bits <<= 1;
		if (carry) {
			val += 2;
		}
		assert((val & 1) == 0);
		assert(val < dict_len);
		val = READ_LE_UINT16(u->dict_buf + val);
		if ((val & 0x8000) == 0) {
			continue;
		}
		val &= ~0x8000;
		switch (state) {
		case 0:
			code = val & 255;
			if (val >> 8) {
				switch (code) {
				case 0:
					state = 1;
					break;
				case 1:
					state = 2;
					break;
				default:
					memset(dst, prev, code);
					dst += code;
					break;
				}
			} else {
				*dst++ = prev = code;
			}
			break;
		case 1:
			memset(dst, prev, val);
			dst += val;
			state = 0;
			break;
		case 2:
			count = (val & 255) << 8;
			state = 3;
			break;
		case 3:
			count |= val & 255;
			memset(dst, prev, count);
			dst += count;
			state = 0;
			break;
		}
		val = 0;
	}
	assert((dst - output_buffer) == uncompressed_size);
	u->dst = output_buffer;
	return uncompressed_size;
}

uint8_t *unpack(FILE *in, int *uncompressed_size) {
	const uint16_t sig = fread_le16(in);
	fseek(in, 0, SEEK_SET);
	if (sig == 0x4CB4) {
		memset(&uneat, 0, sizeof(uneat));
		*uncompressed_size = unpack_eat(in, &uneat);
		return uneat.dst;
	} else if ((sig >> 8) == 0x10) {
		memset(&unsqz, 0, sizeof(unsqz));
		*uncompressed_size = unpack_sqz(in, &unsqz);
		return unsqz.dst;
	} else {
		memset(&unsqv, 0, sizeof(unsqv));
		*uncompressed_size = unpack_sqv(in, &unsqv);
		return unsqv.dst;
	}
	*uncompressed_size = 0;
	return 0;
}


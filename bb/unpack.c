
#include "unpack.h"
#include "util.h"

struct unpack_t {
	uint8_t dict_buf[0x200 * 2];
	uint8_t rd[0x1000];
};

static struct unpack_t g_unpack;

int unpack(FILE *in, uint8_t *dst) {
	fread(g_unpack.rd, 1, 6, in);
	const int uncompressed_size = (READ_LE_UINT16(g_unpack.rd) << 16) + READ_LE_UINT16(g_unpack.rd + 2);
	const int dict_len = READ_LE_UINT16(g_unpack.rd + 4);
	print_debug(DBG_UNPACK, "SQV uncompressed size %d dict_len %d", uncompressed_size, dict_len);
	fread(g_unpack.dict_buf, 1, dict_len, in);

	const uint8_t *start = dst;
	const uint8_t *src = g_unpack.rd;
	int len = 1;
	int bytes_count = 2;
	uint16_t bits = 0;
	uint16_t val = 0;
	while ((dst - start) < uncompressed_size) {
		--len;
		if (len == 0) {
			bytes_count -= 2;
			if (bytes_count == 0) {
				bytes_count = fread(g_unpack.rd, 1, 0x1000, in);
				if (bytes_count == 0) {
					break;
				}
				bytes_count += (bytes_count & 1);
				src = g_unpack.rd;
			}
			bits = READ_BE_UINT16(src); src += 2;
			len = 17;
			continue;
		}
		const int carry = (bits & 0x8000) != 0;
		bits <<= 1;
		if (carry) {
			val += 2;
		}
		assert(val < 0x400);
		val = READ_LE_UINT16(g_unpack.dict_buf + val);
		if ((val & 0x8000) == 0) {
			continue;
		}
		*dst++ = val & 255;
		val = 0;
	}
	assert((dst - start) == uncompressed_size);
	return uncompressed_size;
}

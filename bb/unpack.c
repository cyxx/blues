
#include "unpack.h"
#include "util.h"

struct unpack_t {
	uint8_t dict_buf[0x200 * 2];
};

static struct unpack_t g_unpack;

int unpack(FILE *in, uint8_t *dst) {
	uint8_t buf[6];
	fread(buf, 1, 6, in);
	const int uncompressed_size = (READ_LE_UINT16(buf) << 16) + READ_LE_UINT16(buf + 2);
	const int dict_len = READ_LE_UINT16(buf + 4);
	print_debug(DBG_UNPACK, "SQV uncompressed size %d dict_len %d", uncompressed_size, dict_len);
	fread(g_unpack.dict_buf, 1, dict_len, in);

	const uint8_t *start = dst;
	int bits_count = 1;
	uint16_t bits = 0;
	uint16_t val = 0;
	while ((dst - start) < uncompressed_size) {
		--bits_count;
		if (bits_count == 0) {
			if (fread(buf, 1, 2, in) == 0) {
				break;
			}
			bits = READ_BE_UINT16(buf);
			bits_count = 16;
		}
		const bool carry = (bits & 0x8000) != 0;
		bits <<= 1;
		if (carry) {
			val += 2;
		}
		assert((val & 1) == 0);
		assert(val < dict_len);
		val = READ_LE_UINT16(g_unpack.dict_buf + val);
		if ((val & 0x8000) == 0) {
			continue;
		}
		assert((val & 0x7F00) == 0);
		*dst++ = val & 255;
		val = 0;
	}
	assert((dst - start) == uncompressed_size);
	return uncompressed_size;
}

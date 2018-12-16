
#include "fileio.h"
#include "unpack.h"
#include "util.h"

struct unpack_t {
	uint8_t dict_buf[0x200 * 2];
	uint8_t rd[0x1000];
	int size;
	int dict_len;
};

static struct unpack_t g_unpack;

int unpack(const char *filename, uint8_t *dst) {
	const int f = fio_open(filename, 1);

	fio_read(f, g_unpack.rd, 6);
	g_unpack.size = (READ_LE_UINT16(g_unpack.rd) << 16) + READ_LE_UINT16(g_unpack.rd + 2);
	const int dict_len = READ_LE_UINT16(g_unpack.rd + 4);
	print_debug(DBG_UNPACK, "unpack '%s' size %d dict_len %d", filename, g_unpack.size, dict_len);
	fio_read(f, g_unpack.dict_buf, dict_len);

	const uint8_t *src = g_unpack.rd;
	int len = 1;
	int bytes_count = 2;
	uint16_t bits = 0;
	uint16_t val = 0;
	while (1) {
		--len;
		if (len == 0) {
			bytes_count -= 2;
			if (bytes_count == 0) {
				bytes_count = fio_read(f, g_unpack.rd, 0x1000);
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
	fio_close(f);
	return g_unpack.size;
}

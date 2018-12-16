
#include <sys/param.h>
#include "fileio.h"
#include "util.h"

#define MAX_FILEIO_SLOTS 2

struct fileio_slot_t {
	FILE *fp;
	int size;
} fileio_slot_t;

static const char *_data_path;
static struct fileio_slot_t _fileio_slots_table[MAX_FILEIO_SLOTS];

static int find_free_slot() {
	int i, slot = -1;
	for (i = 0; i < MAX_FILEIO_SLOTS; ++i) {
		if (!_fileio_slots_table[i].fp) {
			slot = i;
			break;
		}
	}
	return slot;
}

void fio_init(const char *data_path) {
	_data_path = data_path;
	memset(_fileio_slots_table, 0, sizeof(_fileio_slots_table));
}

void fio_fini() {
}

int fio_open(const char *filename, int error_flag) {
	int slot = find_free_slot();
	if (slot < 0) {
		print_error("Unable to find free slot for '%s'", filename);
	} else {
		struct fileio_slot_t *file_slot = &_fileio_slots_table[slot];
		memset(file_slot, 0, sizeof(fileio_slot_t));
		file_slot->fp = fopen_nocase(_data_path, filename);
		if (!file_slot->fp) {
			if (error_flag) {
				print_error("Unable to open file '%s'", filename);
			} else {
				print_warning("Unable to open file '%s'", filename);
			}
			slot = -1;
		} else {
			fseek(file_slot->fp, 0, SEEK_END);
			file_slot->size = ftell(file_slot->fp);
			fseek(file_slot->fp, 0, SEEK_SET);
		}
	}
	return slot;
}

void fio_close(int slot) {
	if (slot >= 0) {
		struct fileio_slot_t *file_slot = &_fileio_slots_table[slot];
		assert(file_slot->fp);
		fclose(file_slot->fp);
		memset(file_slot, 0, sizeof(fileio_slot_t));
	}
}

int fio_size(int slot) {
	int size = 0;
	if (slot >= 0) {
		struct fileio_slot_t *file_slot = &_fileio_slots_table[slot];
		assert(file_slot->fp);
		size = file_slot->size;
	}
	return size;
}

int fio_read(int slot, void *data, int len) {
	int sz = 0;
	if (slot >= 0) {
		struct fileio_slot_t *file_slot = &_fileio_slots_table[slot];
		assert(file_slot->fp);
		sz = fread(data, 1, len, file_slot->fp);
	}
	return sz;
}

int fio_exists(const char *filename) {
	FILE *fp = fopen_nocase(_data_path, filename);
	if (fp) {
		fclose(fp);
		return 1;
	}
	return 0;
}

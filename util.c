
#include <stdarg.h>
#include <sys/param.h>
#include "util.h"

int g_debug_mask = 0;

void string_lower(char *p) {
	for (; *p; ++p) {
		if (*p >= 'A' && *p <= 'Z') {
			*p += 'a' - 'A';
		}
	}
}

void string_upper(char *p) {
	for (; *p; ++p) {
		if (*p >= 'a' && *p <= 'z') {
			*p += 'A' - 'a';
		}
	}
}

void print_debug(int debug_channel, const char *msg, ...) {
	if (g_debug_mask & debug_channel) {
		char buf[256];
		va_list va;
		va_start(va, msg);
		vsprintf(buf, msg, va);
		va_end(va);
		fprintf(stdout, "%s\n", buf);
	}
}

void print_warning(const char *msg, ...) {
	char buf[256];
	va_list va;
	va_start(va, msg);
	vsprintf(buf, msg, va);
	va_end(va);
	fprintf(stderr, "WARNING: %s\n", buf);
}

void print_error(const char *msg, ...) {
	char buf[256];
	va_list va;
	va_start(va, msg);
	vsprintf(buf, msg, va);
	va_end(va);
	fprintf(stderr, "ERROR: %s!\n", buf);
	exit(-1);
}

void print_info(const char *msg, ...) {
	char buf[256];
	va_list va;
	va_start(va, msg);
	vsprintf(buf, msg, va);
	va_end(va);
	fprintf(stdout, "%s\n", buf);
}

FILE *fopen_nocase(const char *path, const char *filename) {
	char buf[MAXPATHLEN];
	snprintf(buf, sizeof(buf), "%s/%s", path, filename);
	FILE *fp = fopen(buf, "rb");
	if (!fp) {
		char *p = buf + strlen(path) + 1;
		string_upper(p);
		fp = fopen(buf, "rb");
	}
	return fp;
}

uint16_t fread_le16(FILE *fp) {
	const uint16_t val = fgetc(fp);
	return val | (fgetc(fp) << 8);
}

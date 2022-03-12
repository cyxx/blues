
#include <stdarg.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include "sys.h"
#include "util.h"

int g_debug_mask = 0;

static void string_lower(char *p) {
	for (; *p; ++p) {
		if (*p >= 'A' && *p <= 'Z') {
			*p += 'a' - 'A';
		}
	}
}

static void string_upper(char *p) {
	for (; *p; ++p) {
		if (*p >= 'a' && *p <= 'z') {
			*p += 'A' - 'a';
		}
	}
}

#ifndef NDEBUG
void print_debug(int debug_channel, const char *msg, ...) {
	if (g_debug_mask & debug_channel) {
		char buf[256];
		va_list va;
		va_start(va, msg);
		vsprintf(buf, msg, va);
		va_end(va);
		fprintf(stdout, "%s\n", buf);
		g_sys.print_log(stdout, buf);
	}
}
#endif

void print_warning(const char *msg, ...) {
	char buf[256];
	va_list va;
	va_start(va, msg);
	vsprintf(buf, msg, va);
	va_end(va);
	fprintf(stderr, "WARNING: %s\n", buf);
	g_sys.print_log(stderr, buf);
}

void print_error(const char *msg, ...) {
	char buf[256];
	va_list va;
	va_start(va, msg);
	vsprintf(buf, msg, va);
	va_end(va);
	fprintf(stderr, "ERROR: %s!\n", buf);
	g_sys.print_log(stderr, buf);
	exit(-1);
}

void print_info(const char *msg, ...) {
	char buf[256];
	va_list va;
	va_start(va, msg);
	vsprintf(buf, msg, va);
	va_end(va);
	fprintf(stdout, "%s\n", buf);
	g_sys.print_log(stdout, buf);
}

FILE *fopen_nocase(const char *path, const char *filename) {
	char buf[MAXPATHLEN];
	snprintf(buf, sizeof(buf), "%s/%s", path, filename);
	FILE *fp = fopen(buf, "rb");
	if (!fp) {
		static void (*const str[3])(char *) = {
			string_upper,
			string_lower,
			0
		};
		char *p = buf + strlen(path) + 1;
		for (int i = 0; str[i] && !fp; ++i) {
			(str[i])(p);
			if (strcmp(filename, p) != 0) {
				fp = fopen(buf, "rb");
			}
		}
	}
	return fp;
}

uint16_t fread_le16(FILE *fp) {
	const uint16_t val = fgetc(fp);
	return val | (fgetc(fp) << 8);
}

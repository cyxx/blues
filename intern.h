
#ifndef INTERN_H__
#define INTERN_H__

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef ABS
#define ABS(a) (((a)<0)?(-(a)):(a))
#endif
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

static inline uint16_t READ_BE_UINT16(const uint8_t *p) {
	return (p[0] << 8) | p[1];
}

static inline uint32_t READ_BE_UINT32(const uint8_t *p) {
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static inline uint16_t READ_LE_UINT16(const uint8_t *p) {
	return p[0] | (p[1] << 8);
}

#endif

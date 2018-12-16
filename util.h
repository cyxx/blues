
#ifndef UTIL_H__
#define UTIL_H__

#include "intern.h"

#define DBG_GAME      (1 << 0)
#define DBG_FILEIO    (1 << 1)
#define DBG_RESOURCE  (1 << 2)
#define DBG_MIXER     (1 << 3)
#define DBG_SYSTEM    (1 << 4)
#define DBG_UNPACK    (1 << 5)
#define DBG_SCREEN    (1 << 6)

extern int g_debug_mask;

extern void	string_lower(char *p);
extern void	string_upper(char *p);

extern void	print_debug(int debug_channel, const char *msg, ...);
extern void	print_warning(const char *msg, ...);
extern void	print_error(const char *msg, ...);
extern void	print_info(const char *msg, ...);

extern FILE *	fopen_nocase(const char *path, const char *filename);
extern uint16_t	fread_le16(FILE *fp);

#endif /* UTIL_H__ */

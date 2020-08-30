/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <regex.h>
#include <stddef.h>
#include <time.h>

#include "arg.h"

/* main server struct */
struct vhost {
	char *chost;
	char *regex;
	char *dir;
	char *prefix;
	regex_t re;
};

struct map {
	char *chost;
	char *from;
	char *to;
};

struct server {
	char *host;
	char *port;
	char *docindex;
	int listdirs;
	struct vhost *vhost;
	size_t vhost_len;
	struct map *map;
	size_t map_len;
};

#undef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#undef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#undef LEN
#define LEN(x) (sizeof (x) / sizeof *(x))

extern char *argv0;

void warn(const char *, ...);
void die(const char *, ...);

void epledge(const char *, const char *);
void eunveil(const char *, const char *);

int timestamp(char *, size_t, time_t);
int esnprintf(char *, size_t, const char *, ...);
int prepend(char *, size_t, const char *);
void html_escape(const char *, char *, size_t);
void replace(char **, const char *, const char *);
char *read_file(const char* path);

void *reallocarray(void *, size_t, size_t);
long long strtonum(const char *, long long, long long, const char **);

#endif /* UTIL_H */

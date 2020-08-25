/* See LICENSE file for copyright and license details. */
#ifndef DIRL_H
#define DIRL_H

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "http.h"

struct dirl_env_entry {
  char *name;
  char *value;
};

struct dirl_env {
  struct dirl_env_entry *entries;
};

/*
 * Determine if an dirlist entry should be skipped because it has special
 * meaning. Skips header-, entry-, footer templates and dirlist style css.
 */
int dirl_skip(const char*);
enum status dirl_header(int, const struct response*);
enum status dirl_entry(int, const struct dirent*);
enum status dirl_footer(int);

#endif /* DIRL_H */

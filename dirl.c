/* See LICENSE file for copyright and license details. */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "dirl.h"
#include "http.h"
#include "util.h"

static char*
suffix(int t)
{
  switch (t) {
    case DT_FIFO:
      return "|";
    case DT_DIR:
      return "/";
    case DT_LNK:
      return "@";
    case DT_SOCK:
      return "=";
  }

  return "";
}

static void
html_escape(const char* src, char* dst, size_t dst_siz)
{
  const struct
  {
    char c;
    char* s;
  } escape[] = {
    { '&', "&amp;" },  { '<', "&lt;" },    { '>', "&gt;" },
    { '"', "&quot;" }, { '\'', "&#x27;" },
  };
  size_t i, j, k, esclen;

  for (i = 0, j = 0; src[i] != '\0'; i++) {
    for (k = 0; k < LEN(escape); k++) {
      if (src[i] == escape[k].c) {
        break;
      }
    }
    if (k == LEN(escape)) {
      /* no escape char at src[i] */
      if (j == dst_siz - 1) {
        /* silent truncation */
        break;
      } else {
        dst[j++] = src[i];
      }
    } else {
      /* escape char at src[i] */
      esclen = strlen(escape[k].s);

      if (j >= dst_siz - esclen) {
        /* silent truncation */
        break;
      } else {
        memcpy(&dst[j], escape[k].s, esclen);
        j += esclen;
      }
    }
  }
  dst[j] = '\0';
}

/* Try to find templates up until root
 *
 * Iterates the directory hierarchy upwards. Returns the closest path containing
 * one of the template files or NULL if none could be found.
 *
 * Note that we are chrooted.
 */
static char*
dirl_find_templ_dir(const char* start_path)
{
  char* path_buf = calloc(sizeof(char), strlen(start_path)+1);
  strcat(path_buf, start_path);

  if (strlen(path_buf) > 1 && path_buf[strlen(path_buf) - 1] == '/') {
    // don't read last dir twice, except at root
    path_buf[strlen(path_buf) - 1] = '\0';
  }

  while (strlen(path_buf) != 0) {
    DIR* cur = opendir(path_buf);
    struct dirent* de;
    errno = 0;
    while ((de = readdir(cur))) {
      if (de->d_type == DT_REG) {
        if (!strcmp(DIRL_HEADER, de->d_name) || !strcmp(DIRL_ENTRY, de->d_name) ||
            !strcmp(DIRL_FOOTER, de->d_name)) {
          closedir(cur);
          return path_buf;
        }
      }
    }

    if (strlen(path_buf) > 1) {
      char* parent = strrchr(path_buf, '/');
      (*parent) = '\0'; // strip tail from path_buf

      if (strlen(path_buf) == 0) { // we stripped root, let it loop once more
        path_buf[0] = '/';
        path_buf[1] = '\0';
      }
    } else {
      path_buf[0] = '\0'; // we checked root, now terminate loop
    }
  }

  free(path_buf);
  return NULL;
}

/* Helper function to fill template from base+name if file exists */
static void
dirl_fill_templ(char** templ, char* base, char* name, char* def)
{
  if (!base || !name) {
    *templ = def;
    return;
  }

  char* path = calloc(sizeof(char), strlen(base) + strlen(name));
  strcpy(path, base);
  strcat(path, name);

  char* file_buf = read_file(path);
  free(path);

  if (file_buf) {
    *templ = file_buf;
  } else {
    *templ = def;
  }
}

struct dirl_templ
dirl_read_templ(const char* path)
{
  struct dirl_templ templ;

  char* templ_dir = dirl_find_templ_dir(path);

  dirl_fill_templ(&templ.header, templ_dir, DIRL_HEADER, DIRL_HEADER_DEFAULT);
  dirl_fill_templ(&templ.entry, templ_dir, DIRL_ENTRY, DIRL_ENTRY_DEFAULT);
  dirl_fill_templ(&templ.footer, templ_dir, DIRL_FOOTER, DIRL_FOOTER_DEFAULT);

  free(templ_dir);

  return templ;
}

enum status
dirl_header(int fd, const struct response* res, const struct dirl_templ* templ)
{
  /* Replace placeholder */
  char* nhead = calloc(sizeof(char), strlen(templ->header) + 1);
  memcpy(nhead, templ->header, strlen(templ->header) + 1);
  replace(&nhead, "{uri}", res->uri);

  /* Write header */
  write(fd, nhead, strlen(nhead));

  free(nhead);

  /* listing header */
  return 0;
}

enum status
dirl_entry(int fd,
           const struct dirent* entry,
           const struct response* res,
           const struct dirl_templ* templ)
{
  struct stat stat_buf;
  char* path_buf = calloc(sizeof(char), strlen(res->path)+strlen(entry->d_name));
  strcat(path_buf, res->uri);
  strcat(path_buf, entry->d_name);
  lstat(path_buf, &stat_buf);

  char* nentry = calloc(sizeof(char), strlen(templ->entry) + 1);
  memcpy(nentry, templ->entry, strlen(templ->entry) + 1);

  /* Replace placeholder */
  char esc[PATH_MAX * 6];
  html_escape(entry->d_name, esc, PATH_MAX * 6);
  replace(&nentry, "{entry}", entry->d_name);

  replace(&nentry, "{suffix}", suffix(entry->d_type));

  char size_buf[1024];
  if (entry->d_type == DT_REG) {
    snprintf(size_buf, 1024, "%ld", stat_buf.st_size);
  } else {
    sprintf(size_buf, "-");
  }
  replace(&nentry, "{size}", size_buf);

  char time_buf[1024];
  struct tm tm;
  gmtime_r(&stat_buf.st_mtim.tv_sec, &tm);
  strftime(time_buf, 1024, "%F %H:%m", &tm);
  replace(&nentry, "{modified}", time_buf);

  /* Write entry */
  write(fd, nentry, strlen(nentry));

  free(nentry);

  return 0;
}

enum status
dirl_footer(int fd, const struct dirl_templ* templ)
{
  /* Replace placeholder */
  char* nfoot = calloc(sizeof(char), strlen(templ->footer) + 1);
  memcpy(nfoot, templ->footer, strlen(templ->footer) + 1);

  /* Write footer */
  write(fd, nfoot, strlen(nfoot));

  free(nfoot);
  return 0;
}

int
dirl_skip(const char* name)
{
  return name[0] == '.'                //
         || !strcmp(name, DIRL_HEADER) //
         || !strcmp(name, DIRL_ENTRY)  //
         || !strcmp(name, DIRL_FOOTER) //
         || !strcmp(name, DIRL_STYLE)  //
         || !strcmp(name, FAVICON);    //
}

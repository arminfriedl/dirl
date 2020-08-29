/* See LICENSE file for copyright and license details. */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
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

static char*
dirl_read(char* tpl)
{
  FILE* tpl_fp;

  if (!(tpl_fp = fopen(tpl, "r"))) {
    return NULL;
  }

  /* Get size of template */
  if (fseek(tpl_fp, 0L, SEEK_END) < 0) {
    fclose(tpl_fp);
    return NULL;
  }

  long tpl_size;
  if ((tpl_size = ftell(tpl_fp)) < 0) {
    fclose(tpl_fp);
    return NULL;
  }

  rewind(tpl_fp);

  /* Read template into tpl_buf */
  char* tpl_buf = (char*)calloc(sizeof(char), tpl_size);

  if (tpl_buf == NULL) {
    fclose(tpl_fp);
    free(tpl_buf);
    return NULL;
  }

  if (fread(tpl_buf, 1, tpl_size, tpl_fp) < tpl_size) {
    if (feof(tpl_fp)) {
      warn("Reached end of template %s prematurely", tpl);
    } else if (ferror(tpl_fp)) {
      warn("Error while reading template %s", tpl);
    }

    fclose(tpl_fp);
    free(tpl_buf);
    clearerr(tpl_fp);

    return NULL;
  }

  return tpl_buf;
}

/* Try to find templates up until root
 *
 * Iterates the directory hierarchy upwards. Returns the closest path containing
 * a template file or NULL if none could be found.
 *
 * Note that we are chrooted.
 */
static char*
dirl_find_templ_dir(const char* start_path)
{
  char* path_buf = calloc(sizeof(char), strlen(start_path));
  strcat(path_buf, start_path);

  while (strlen(path_buf) != 0) {
    DIR* cur = opendir(path_buf);
    struct dirent* de;
    errno = 0;
    while ((de = readdir(cur))) {
      if (de->d_type == DT_REG) {
        if (strcmp(DIRL_HEADER, de->d_name) || strcmp(DIRL_ENTRY, de->d_name) ||
            strcmp(DIRL_FOOTER, de->d_name)) {
          closedir(cur);
          return path_buf;
        }
      }
    }

    char* parent = strrchr(path_buf, '/');
    (*parent) = '\0'; // strip tail from path_buf
  }

  free(path_buf);
  return NULL;
}

struct dirl_templ
dirl_read_templ(const char* path)
{
  struct dirl_templ templates = { .header = DIRL_HEADER_DEFAULT,
                                  .entry = DIRL_ENTRY_DEFAULT,
                                  .footer = DIRL_FOOTER_DEFAULT };

  char* templ_dir = dirl_find_templ_dir(path);

  char* header_file =
    calloc(sizeof(char), strlen(templ_dir) + strlen(DIRL_HEADER));
  strcpy(header_file, templ_dir);
  strcat(header_file, DIRL_HEADER);
  char* header = dirl_read(header_file);
  if (header)
    templates.header = header;
  free(header_file);

  char* entry_file =
    calloc(sizeof(char), strlen(templ_dir) + strlen(DIRL_ENTRY));
  strcpy(entry_file, templ_dir);
  strcat(entry_file, DIRL_ENTRY);
  char* entry = dirl_read(entry_file);
  if (entry)
    templates.entry = entry;
  free(entry_file);

  char* footer_file =
    calloc(sizeof(char), strlen(templ_dir) + strlen(DIRL_FOOTER));
  strcpy(footer_file, templ_dir);
  strcat(footer_file, DIRL_FOOTER);
  char* footer = dirl_read(footer_file);
  if (footer)
    templates.footer = footer;
  free(footer_file);

  free(templ_dir);

  return templates;
}

enum status
dirl_header(int fd, const struct response* res, const struct dirl_templ* templ)
{
  /* Replace placeholder */
  char* nhead = replace(templ->header, "{curdir}", "something");

  /* Write header */
  write(fd, nhead, strlen(nhead));

  free(nhead);

  /* listing header */
  return 0;
}

enum status
dirl_entry(int fd, const struct dirent* entry, const struct dirl_templ* templ)
{
  /* Replace placeholder */
  char* nentry = replace(templ->entry, "{entry}", entry->d_name);

  /* Write entry */
  write(fd, nentry, strlen(nentry));

  free(nentry);

  return 0;
}

enum status
dirl_footer(int fd, const struct dirl_templ* templ)
{
  /* Replace placeholder */
  char* nfoot = replace(templ->footer, "{idx}", "something");

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
         || !strcmp(name, DIRL_STYLE); //
}

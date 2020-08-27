/* See LICENSE file for copyright and license details. */
#include <dirent.h>
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
dirl_read_template(char* tpl)
{
  /* Try find template in root (note that we are chroot'ed) */
  FILE* tpl_fp;
  char* tpl_abs;

  tpl_abs = calloc(strlen(tpl)+2, sizeof(char));
  if(tpl_abs == NULL) {
    return NULL;
  }
  tpl_abs[0] = '/';
  strcat(tpl_abs, tpl);

  if (!(tpl_fp = fopen(tpl_abs, "r"))) {
    free(tpl_abs);
    return NULL;
  }

  free(tpl_abs);

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
  char* tpl_buf = (char*)malloc(sizeof(char) * tpl_size);

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

static enum status
dirl_header_default(int fd, const struct response* res)
{
  char esc[PATH_MAX];
  html_escape(res->uri, esc, sizeof(esc));
  if (dprintf(fd,
              "<!DOCTYPE html>\n"
              "<html>\n"
              "  <head>\n"
              "    <link rel=\"stylesheet\" href=\"/"DIRL_STYLE"\">\n"
              "    <title>Index of %s</title>\n"
              "  </head>\n"
              "  <body>\n"
              "    <h1>Index of %s</h1>\n"
              "    <a href=\"..\">..</a>",
              esc,
              esc) < 0) {
    return S_REQUEST_TIMEOUT;
  }

  return 0;
}

enum status
dirl_header(int fd, const struct response* res)
{
  char* tpl = dirl_read_template(DIRL_HEADER);

  if(tpl == NULL) {
    return dirl_header_default(fd, res);
  }

  /* Replace placeholder */
  char* nhead = replace(tpl, "{idx}", "something");

  /* Write header */
  write(fd, nhead, strlen(nhead));

  free(tpl);
  free(nhead);

  /* listing header */
  return 0;
}

static enum status
dirl_entry_default(int fd, const struct dirent* entry)
{
  char esc[PATH_MAX];

  html_escape(entry->d_name, esc, sizeof(esc));
  if (dprintf(fd,
              "<br />\n\t\t<a href=\"%s%s\">%s%s</a>",
              esc,
              (entry->d_type == DT_DIR) ? "/" : "",
              esc,
              suffix(entry->d_type)) < 0) {
    return S_REQUEST_TIMEOUT;
  }

  return 0;
}

enum status
dirl_entry(int fd, const struct dirent* entry)
{
  char* tpl = dirl_read_template(DIRL_ENTRY);

  if (tpl == NULL) {
    return dirl_entry_default(fd, entry);
  }

  /* Replace placeholder */
  char* nentry = replace(tpl, "{entry}", entry->d_name);

  /* Write entry */
  write(fd, nentry, strlen(nentry));

  free(tpl);
  free(nentry);

  return 0;
}

static enum status
dirl_footer_default(int fd)
{
  if (dprintf(fd,
              "\n"
              "  </body>\n"
              "</html>") < 0) {
    return S_REQUEST_TIMEOUT;
  }

  return 0;
}

enum status
dirl_footer(int fd)
{
  char* tpl = dirl_read_template(DIRL_FOOTER);

  if(tpl == NULL) {
    return dirl_footer_default(fd);
  }

  /* Replace placeholder */
  char* nfoot = replace(tpl, "{idx}", "something");

  /* Write footer */
  write(fd, nfoot, strlen(nfoot));

  free(tpl);
  free(nfoot);
  return 0;
}

int
dirl_skip(const char* name)
{
  (void)name; // noop; avoid unused warning

  return !strcmp(name, DIRL_HEADER)    //
         || !strcmp(name, DIRL_ENTRY)  //
         || !strcmp(name, DIRL_FOOTER) //
         || !strcmp(name, DIRL_STYLE);
}

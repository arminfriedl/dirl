/* See LICENSE file for copyright and license details. */
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dirl.h"
#include "http.h"
#include "util.h"
#include "config.h"

static char *suffix(int t) {
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

enum status
dirl_header_default(int fd, const struct response *res) {
  char esc[PATH_MAX];
  html_escape(res->uri, esc, sizeof(esc));
  if (dprintf(fd,
              "<!DOCTYPE html>\n<html>\n\t<head>"
              "<title>Index of %s</title></head>\n"
              "\t<body>\n"
              "\t\t<h1>Index of %s</h1>\n"
              "\t\t\t<a href=\"..\">..</a>",
              esc, esc) < 0) {
    return S_REQUEST_TIMEOUT;
  }

  return 0;
}

enum status
dirl_header(int fd, const struct response *res)
{

  /* No header file defined, default */
#ifndef DIRL_HEADER
  return dirl_header_default(fd, res);
#endif

  /* Try find header in root (note that we are chroot'ed) */
  int header_fd;
  if ( (header_fd = open(("/" DIRL_HEADER), O_RDONLY)) < 0 ) {
    return dirl_header_default(fd, res);
  }

  /* Get size of header */
  struct stat header_stat;
  if (fstat(header_fd, &header_stat) < 0) {
    return dirl_header_default(fd, res);
  }

  /* Allocate space for file */
  char *header = (char *) malloc(sizeof(char) * header_stat.st_size);
  if (header == NULL) {
    return dirl_header_default(fd, res);
  }

  /* Read header into string for template replacement */
  if ( (read(header_fd, header, header_stat.st_size)) < 0) {
    free(header);
    return dirl_header_default(fd, res);
  }

  /* Replace placeholder */
  char *nhead = replace(header, "{idx}", "something");

  /* Write header */
  write(fd, nhead, strlen(nhead));

  free(header);
  free(nhead);

  /* listing header */
  return 0;
}

static enum status
dirl_entry_default(int fd, const struct dirent* entry) {
  char esc[PATH_MAX];

  html_escape(entry->d_name, esc, sizeof(esc));
  if (dprintf(fd, "<br />\n\t\t<a href=\"%s%s\">%s%s</a>", esc,
              (entry->d_type == DT_DIR) ? "/" : "", esc,
              suffix(entry->d_type)) < 0) {
    return S_REQUEST_TIMEOUT;
  }

  return 0;
}

enum status
dirl_entry(int fd, const struct dirent* entry) {
  /* Try find entry in root (note that we are chroot'ed) */
  int entry_fd;
  if ((entry_fd = open(("/" DIRL_ENTRY), O_RDONLY)) < 0) {
    return dirl_entry_default(fd, entry);
  }

  /* Get size of entry*/
  struct stat entry_stat;
  if (fstat(entry_fd, &entry_stat) < 0) {
    return dirl_entry_default(fd, entry);
  }

  /* Write entry */
  if (sendfile(fd, entry_fd, NULL, entry_stat.st_size) < 0) {
    return dirl_entry_default(fd, entry);
  }

  return 0;
}

static enum status
dirl_footer_default(int fd) {
  if (dprintf(fd, "\n\t</body>\n</html>\n") < 0) {
    return S_REQUEST_TIMEOUT;
  }

  return 0;
}

enum status
dirl_footer(int fd) {
  /* Try find footer in root (note that we are chroot'ed) */
  int footer_fd;
  if ((footer_fd = open(("/" DIRL_FOOTER), O_RDONLY)) < 0) {
    return dirl_footer_default(fd);
  }

  /* Get size of footer */
  struct stat footer_stat;
  if (fstat(footer_fd, &footer_stat) < 0) {
    return dirl_footer_default(fd);
  }

  /* Write footer */
  if (sendfile(fd, footer_fd, NULL, footer_stat.st_size) < 0) {
    return dirl_footer_default(fd);
  }

   return 0;
}

int dirl_skip(const char *name) {
  (void) name;
#ifdef DIRL_HEADER
  if(!strcmp(name, DIRL_HEADER)) {
    return 1;
  }
#endif
#ifdef DIRL_ENTRY
  if (!strcmp(name, DIRL_ENTRY)) {
    return 1;
  }
#endif
#ifdef DIRL_FOOTER
  if (!strcmp(name, DIRL_FOOTER)) {
    return 1;
  }
#endif
#ifdef DIRL_STYLE
  if (!strcmp(name, DIRL_STYLE)) {
    return 1;
  }
#endif

  return 0;
}

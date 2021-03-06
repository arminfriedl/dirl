/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifdef __OpenBSD__
#include <unistd.h>
#endif /* __OpenBSD__ */

#include "util.h"

char *argv0;

static void
verr(const char *fmt, va_list ap)
{
  if (argv0 && strncmp(fmt, "usage", sizeof("usage") - 1)) {
    fprintf(stderr, "%s: ", argv0);
  }

  vfprintf(stderr, fmt, ap);

  if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
    fputc(' ', stderr);
    perror(NULL);
  } else {
    fputc('\n', stderr);
  }
}

void
warn(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  verr(fmt, ap);
  va_end(ap);
}

void
die(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  verr(fmt, ap);
  va_end(ap);

  exit(1);
}

void
epledge(const char *promises, const char *execpromises)
{
  (void)promises;
  (void)execpromises;

#ifdef __OpenBSD__
  if (pledge(promises, execpromises) == -1) {
    die("pledge:");
  }
#endif /* __OpenBSD__ */
}

void
eunveil(const char *path, const char *permissions)
{
  (void)path;
  (void)permissions;

#ifdef __OpenBSD__
  if (unveil(path, permissions) == -1) {
    die("unveil:");
  }
#endif /* __OpenBSD__ */
}

int
timestamp(char *buf, size_t len, time_t t)
{
  struct tm tm;

  if (gmtime_r(&t, &tm) == NULL ||
      strftime(buf, len, "%a, %d %b %Y %T GMT", &tm) == 0) {
    return 1;
  }

  return 0;
}

int
esnprintf(char *str, size_t size, const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start(ap, fmt);
  ret = vsnprintf(str, size, fmt, ap);
  va_end(ap);

  return (ret < 0 || (size_t)ret >= size);
}

int
prepend(char *str, size_t size, const char *prefix)
{
  size_t len = strlen(str), prefixlen = strlen(prefix);

  if (len + prefixlen + 1 > size) {
    return 1;
  }

  memmove(str + prefixlen, str, len + 1);
  memcpy(str, prefix, prefixlen);

  return 0;
}

void
replace(char **src, const char *old, const char* new) {
  int old_len = strlen(old);
  int new_len = strlen(new);
  int src_len = strlen(*src);

  /* Count needed replacements */
  const char* tmp = *src;
  int replc=0;
  while ((tmp=strstr(tmp, old))) {
    replc++;
    tmp += old_len;
  }

  /* Allocate enough space for the new string */
  size_t buf_size = src_len + replc * (new_len - old_len) + 1;
  char* buf = calloc(sizeof(char), buf_size);

  /* Now start replacing */
  const char *srcidx = *src;
  const char *srcidx_old = *src;
  char *bufidx = buf;

  while (replc--) {
    srcidx_old = strstr(srcidx, old);

    long repl_len = labs(srcidx_old - srcidx);
    bufidx = strncpy(bufidx, srcidx, repl_len) + repl_len;
    bufidx = strcpy(bufidx, new) + new_len;

    srcidx = srcidx_old+old_len;
  }

  strncpy(bufidx, srcidx, strlen(srcidx)); // copy tail

  free(*src);
  *src = buf;
}

char*
read_file(const char* path){
  FILE* tpl_fp;

  if (!(tpl_fp = fopen(path, "r"))) {
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

  if (fread(tpl_buf, 1, tpl_size, tpl_fp) < (size_t) tpl_size) {
    if (feof(tpl_fp)) {
      warn("Reached end of file %s prematurely", path);
    } else if (ferror(tpl_fp)) {
      warn("Error while reading file %s", path);
    }

    fclose(tpl_fp);
    free(tpl_buf);
    clearerr(tpl_fp);

    return NULL;
  }

  return tpl_buf;
}

#define	INVALID  1
#define	TOOSMALL 2
#define	TOOLARGE 3

long long
strtonum(const char *numstr, long long minval, long long maxval,
         const char **errstrp)
{
  long long ll = 0;
  int error = 0;
  char *ep;
  struct errval {
    const char *errstr;
    int err;
  } ev[4] = {
    { NULL,		0 },
    { "invalid",	EINVAL },
    { "too small",	ERANGE },
    { "too large",	ERANGE },
  };

  ev[0].err = errno;
  errno = 0;
  if (minval > maxval) {
    error = INVALID;
  } else {
    ll = strtoll(numstr, &ep, 10);
    if (numstr == ep || *ep != '\0')
      error = INVALID;
    else if ((ll == LLONG_MIN && errno == ERANGE) || ll < minval)
      error = TOOSMALL;
    else if ((ll == LLONG_MAX && errno == ERANGE) || ll > maxval)
      error = TOOLARGE;
  }
  if (errstrp != NULL)
    *errstrp = ev[error].errstr;
  errno = ev[error].err;
  if (error)
    ll = 0;

  return ll;
}

/*
 * This is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
 * if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
 */
#define MUL_NO_OVERFLOW	((size_t)1 << (sizeof(size_t) * 4))

void *
reallocarray(void *optr, size_t nmemb, size_t size)
{
  if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
      nmemb > 0 && SIZE_MAX / nmemb < size) {
    errno = ENOMEM;
    return NULL;
  }
  return realloc(optr, size * nmemb);
}

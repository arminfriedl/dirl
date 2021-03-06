/* See LICENSE file for copyright and license details. */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "http.h"
#include "data.h"
#include "util.h"
#include "dirl.h"

static int
compareent(const struct dirent **d1, const struct dirent **d2)
{
	int v;

	v = ((*d2)->d_type == DT_DIR ? 1 : -1) -
	    ((*d1)->d_type == DT_DIR ? 1 : -1);
	if (v) {
		return v;
	}

	return strcmp((*d1)->d_name, (*d2)->d_name);
}

enum status
data_send_dirlisting(int fd, const struct response *res)
{
	enum status ret = 0;
	struct dirent **e;
	size_t i;
	int dirlen;

	/* read directory */
	if ((dirlen = scandir(res->path, &e, NULL, compareent)) < 0) {
		return S_FORBIDDEN;
	}

  /* read templates */
  struct dirl_templ templates = dirl_read_templ(res->uri);

  /* listing header */
  if ((ret = dirl_header(fd, res, &templates))) {
    return ret;
  }

	/* entries */
	for (i = 0; i < (size_t)dirlen; i++) {
    /*  skip dirl special files */
    if(dirl_skip(e[i]->d_name)) {
      continue;
    }

    /* entry line */
    if ((ret = dirl_entry(fd, e[i], res, &templates))) {
      goto cleanup;
    }

	}

  /* listing footer */
  if ((ret = dirl_footer(fd, &templates))) {
    goto cleanup;
  }

cleanup:
	while (dirlen--) {
		free(e[dirlen]);
	}
	free(e);

	return ret;
}

enum status
data_send_error(int fd, const struct response *res)
{
	if (dprintf(fd,
	            "<!DOCTYPE html>\n<html>\n\t<head>\n"
	            "\t\t<title>%d %s</title>\n\t</head>\n\t<body>\n"
	            "\t\t<h1>%d %s</h1>\n\t</body>\n</html>\n",
	            res->status, status_str[res->status],
		    res->status, status_str[res->status]) < 0) {
		return S_REQUEST_TIMEOUT;
	}

	return 0;
}

enum status
data_send_file(int fd, const struct response *res)
{
	FILE *fp;
	enum status ret = 0;
	ssize_t bread, bwritten;
	size_t remaining;
	static char buf[BUFSIZ], *p;

	/* open file */
	if (!(fp = fopen(res->path, "r"))) {
		ret = S_FORBIDDEN;
		goto cleanup;
	}

	/* seek to lower bound */
	if (fseek(fp, res->file.lower, SEEK_SET)) {
		ret = S_INTERNAL_SERVER_ERROR;
		goto cleanup;
	}

	/* write data until upper bound is hit */
	remaining = res->file.upper - res->file.lower + 1;

	while ((bread = fread(buf, 1, MIN(sizeof(buf),
	                      remaining), fp))) {
		if (bread < 0) {
			ret = S_INTERNAL_SERVER_ERROR;
			goto cleanup;
		}
		remaining -= bread;
		p = buf;
		while (bread > 0) {
			bwritten = write(fd, p, bread);
			if (bwritten <= 0) {
				ret = S_REQUEST_TIMEOUT;
				goto cleanup;
			}
			bread -= bwritten;
			p += bwritten;
		}
	}
cleanup:
	if (fp) {
		fclose(fp);
	}

	return ret;
}

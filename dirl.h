/* See LICENSE file for copyright and license details. */
#ifndef DIRL_H
#define DIRL_H

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "http.h"

#define DIRL_HEADER ".header.tpl"
#define DIRL_ENTRY ".entry.tpl"
#define DIRL_FOOTER ".footer.tpl"
#define DIRL_STYLE "style.css"

/* Default template definitions
 *
 * Used if no template files can be found
 */
#define DIRL_HEADER_DEFAULT                                                    \
  "<!DOCTYPE HTML PUBLIC \" - // W3C//DTD HTML 3.2 Final//EN\">\n"             \
  "<html>\n"                                                                   \
  "  <head>\n"                                                                 \
  "    <link rel=\"stylesheet\" href=\"/" DIRL_STYLE "\">\n"                   \
  "    <title>Index of {curdir}</title>\n"                                     \
  "  </head>\n"                                                                \
  "  <body>\n"                                                                 \
  "    <h1>Index of {curdir}</h1>\n"                                           \
  "    <a href=\"..\">..</a>\n"

#define DIRL_ENTRY_DEFAULT                                                     \
  "    <br />\n"                                                               \
  "    <a href=\"{entry}\">{entry}</a>\n"

#define DIRL_FOOTER_DEFAULT                                                    \
  "  </body>\n"                                                                \
  "</html>"

struct dirl_templ
{
  const char* header;
  const char* entry;
  const char* footer;
};

struct dirl_templ
dirl_read_templ(const char* path);

/* Determine if an dirlist entry should be skipped
 *
 * Skips:
 * - hidden files and directories
 * - special directory entries (., ..)
 * - header template: DIRL_HEADER
 * - entry template: DIRL_ENTRY
 * - footer template: DIRL_FOOTER
 * - dirlist style: DRIL_STYLE
 */
int
dirl_skip(const char*);

/* Print header into the response */
enum status
dirl_header(int, const struct response*, const struct dirl_templ*);

/* Print entry into the response */
enum status
dirl_entry(int, const struct dirent*, const struct dirl_templ*);

/* Print footer into the response */
enum status
dirl_footer(int, const struct dirl_templ*);

#endif /* DIRL_H */

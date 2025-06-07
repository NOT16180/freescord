/* Darren Chauvet 12205390
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *crlf_to_lf(const char *line_with_crlf) {
  if (!line_with_crlf)
    return NULL;

  size_t length = strlen(line_with_crlf);
  char *new_line = malloc(length + 1);
  if (!new_line)
    return NULL;

  size_t j = 0;
  for (size_t i = 0; i < length; i++) {
    if (line_with_crlf[i] == '\r' && (i + 1) < length &&
        line_with_crlf[i + 1] == '\n') {
      new_line[j++] = '\n';
      i++;
    } else {
      new_line[j++] = line_with_crlf[i];
    }
  }
  new_line[j] = '\0';
  return new_line;
}

char *lf_to_crlf(char *line_with_lf) {
  if (!line_with_lf)
    return NULL;

  size_t lf_count = 0;
  size_t len = strlen(line_with_lf);

  for (size_t i = 0; i < len; i++) {
    if (line_with_lf[i] == '\n')
      lf_count++;
  }
  char *result = malloc(len + lf_count + 1);
  if (!result)
    return NULL;

  size_t j = 0;
  for (size_t i = 0; i < len; i++) {
    if (line_with_lf[i] == '\n') {
      result[j++] = '\r';
      result[j++] = '\n';
    } else {
      result[j++] = line_with_lf[i];
    }
  }
  result[j] = '\0';

  return result;
}

void perror_exit(const char *err_mess) {
  perror(err_mess);
  exit(2);
}

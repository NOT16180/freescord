/* Darren Chauvet 12205390
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */

#include "buffer.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

buffer *buff_create(int fd, size_t buffsz) {
  buffer *b = malloc(sizeof(buffer));
  if (!b)
    return NULL;

  b->data = malloc(buffsz);
  if (!b->data) {
    free(b);
    return NULL;
  }

  b->fd = fd;
  b->size = buffsz;
  b->pos = buffsz;
  b->end = 0;
  return b;
}
int buff_getc(buffer *b) {
  if (b->pos >= b->size) {
    ssize_t n = read(b->fd, b->data, b->size);
    if (n <= 0) {
      b->end = 1;
      return EOF;
    }
    b->pos = 0;
    b->size = n;
  }
  return (unsigned char)b->data[b->pos++];
}

int buff_ungetc(buffer *b, int c) {
  if (c == EOF)
    return EOF;
  if (b->pos > 0) {
    b->data[--b->pos] = c;
  } else {
    return EOF;
  }

  return c;
}

void buff_free(buffer *b) {
  if (b == NULL)
    return;
  if (b->data != NULL)
    free(b->data);

  if (b->fd >= 0)
    close(b->fd);

  free(b);
}

int buff_eof(const buffer *buff) {
  return (buff->pos >= buff->size) && buff->end;
}

int buff_ready(const buffer *buff) {
  return (buff != NULL) && (buff->pos < buff->size);
}

char *buff_fgets(buffer *b, char *dest, size_t size) {
  if (!b || !dest || size < 1) {
    if (dest)
      dest[0] = '\0';
    return NULL;
  }

  size_t i = 0;
  while (i < size - 1) {
    int c = buff_getc(b);
    if (c == EOF) {
      if (i == 0)
        return NULL;
      break;
    }
    dest[i++] = (char)c;
    if (c == '\n')
      break;
  }
  dest[i] = '\0';
  return dest;
}

char *buff_fgets_crlf(buffer *b, char *dest, size_t size) {
  if (size < 1 || !b || !dest)
    return NULL;

  size_t i = 0;
  int c;
  int prev_char = 0;

  while (i < size - 1) {
    c = buff_getc(b);
    if (c == EOF) {
      if (i == 0)
        return NULL;
      break;
    }

    dest[i++] = (char)c;

    if (prev_char == '\r' && c == '\n') {
      break;
    }

    prev_char = c;
  }

  dest[i] = '\0';
  return dest;
}

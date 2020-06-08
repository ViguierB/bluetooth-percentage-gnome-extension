#include <string.h>
#include <stdlib.h>

#define _BLE_INTERNAL_
#include "battery_level_engine.h"

static int  count_lines(char *inp, char sep, char **tmp)
{
  int count;

  count = 1;
  while (*inp && *inp == sep)
    inp++;
  if (!(inp = strdup(inp)))
    return (0);
  *tmp = inp;
  if (*inp == '\0')
    return (0);
  while (*inp) {
    if (*inp == sep) {
      *inp = '\0';
      while (*(inp + 1) && *(inp + 1) == sep)
        inp++;
      if (*(inp + 1))
        count++;
    }
      inp++;
  }
  return (count);
}

char  **split(char *inp, char sep, int *nb_lines)
{
  char  **res;
  char  *tmp;
  int   i;
  int   lines;

  i = 0;
  lines = count_lines(inp, sep, &tmp);
  res = malloc(sizeof(char*) * (lines + 1));
  if (!res || !tmp)
    return (NULL);
  if (nb_lines)
    *nb_lines = lines;
  while (i < lines) {
    res[i++] = tmp;
    if (!(i < lines))
      break;
    while (*tmp != '\0')
      tmp++;
    tmp++;
    while (*tmp == sep)
      tmp++;
  }
  res[i] = NULL;
  return (res);
}

void  free_split(char** sstr) {
  free(*sstr);
  free(sstr);
}

char* _ble_strstr(const ble_t* ble, const char* find, size_t slen)
{
  char        c, sc;
  const char* s = ble->buffer;
  size_t      len;

  if ((c = *find++) != '\0') {
    len = strlen(find);
    do {
      do {
        if (slen-- < 1 || (sc = *s++) == '\0')
          return (NULL);
      } while (sc != c);
      if (len > slen)
        return (NULL);
    } while (strncmp(s, find, len) != 0);
    s--;
  }
  return ((char *)s);
}
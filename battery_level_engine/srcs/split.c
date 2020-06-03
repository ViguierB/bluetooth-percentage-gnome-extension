#include <string.h>
#include <stdlib.h>

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
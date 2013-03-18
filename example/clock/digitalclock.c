#include <stdio.h>
#include <time.h>
#include <unistd.h>

int height, width, scale;
int hour, min, sec;

int zero[15] = {
  1, 1, 1,
  1, 0, 1,
  1, 0, 1,
  1, 0, 1,
  1, 1, 1 };
int one[15] = {
  0, 0, 1,
  0, 0, 1,
  0, 0, 1,
  0, 0, 1,
  0, 0, 1 };
int two[15] = {
  1, 1, 1,
  0, 0, 1,
  1, 1, 1,
  1, 0, 0,
  1, 1, 1 };
int three[15] = {
  1, 1, 1,
  0, 0, 1,
  1, 1, 1,
  0, 0, 1,
  1, 1, 1 };
int four[15] = {
  1, 0, 1,
  1, 0, 1,
  1, 1, 1,
  0, 0, 1,
  0, 0, 1 };
int five[15] = {
  1, 1, 1,
  1, 0, 0,
  1, 1, 1,
  0, 0, 1,
  1, 1, 1 };
int six[15] = {
  1, 1, 1,
  1, 0, 0,
  1, 1, 1,
  1, 0, 1,
  1, 1, 1 };
int seven[15] = {
  1, 1, 1,
  0, 0, 1,
  0, 0, 1,
  0, 0, 1,
  0, 0, 1 };
int eight[15] = {
  1, 1, 1,
  1, 0, 1,
  1, 1, 1,
  1, 0, 1,
  1, 1, 1 };
int nine[15] = {
  1, 1, 1,
  1, 0, 1,
  1, 1, 1,
  0, 0, 1,
  1, 1, 1 };

int *numbers[10] = {
  zero, one, two, three, four,
  five, six, seven, eight, nine };

void background(void)
{
  int i;
  for (i = 0; i < scale; i++)
    printf("%c%c%c", 0, 0, 0);
}

void foreground(void)
{
  int i;
  for (i = 0; i < scale; i++)
    printf("%c%c%c", 255, 255, 255);
}

void number(int num, int i, int j)
{
  if (numbers[num][3 * (5 - i) + j])
    foreground();
  else
    background();
}

void dot(int i)
{
  /* if (sec % 2 == 0) background(); else */
  if (i == 2 || i == 4)
    foreground();
  else
    background();
}

void outputbmp(void)
{
  char bm[100] =
    "BM\0\0\0\0\0\0\0\0\x1a\0\0\0\xc\0\0\0\x1\0\x1\0\x1\0\x18\0";
  int i, j, k, l, m;
  int num[6] = {
    hour / 10, hour % 10,
    min / 10, min % 10,
    sec / 10, sec % 10 };
  bm[18] = width % 256; bm[19] = width / 256;
  bm[20] = height % 256; bm[21] = height / 256;
  k = (-(int)width*3) & 3;
  for (i = 0; i < 26; i++) printf("%c", bm[i]);
  for (i = 0; i < height / scale; i++) {
    for (m = 0; m < scale; m++) {
      for (j = 0; j < width / scale; j++) {
        switch (i) {
          case 0: case 6:
            background(); break;
          default:
            switch (j) {
              case 0: case 4: case 8:
              case 10: case 14: case 18:
              case 20: case 24: case 28:
                background();
                break;
              case 9: case 19:
                dot(i);
                break;
              default:
                l = j / 5; number(num[l], i, j - 5 * l + l % 2 - 1);
                break;
            }
        }
      }
      for (j = 0; j < k; j++) printf("%c", 0);
    }
  }
}

int main(int argc, char *argv[])
{
  time_t clock;
  struct tm *t;
  scale = 2;
  width = 29 * scale;
  height = 7 * scale;
  do {
    clock = time(NULL);
    t = localtime(&clock);
    hour = t -> tm_hour % 24;
    min = t -> tm_min % 60;
    sec = t -> tm_sec % 60;
    outputbmp();
    fflush(stdout);
    usleep(1e5);
  } while (1);
  return 0;
}


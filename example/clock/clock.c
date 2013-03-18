#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <math.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

double height, width, size;
int hour, min, sec;
int hour_cache[24][60], min_cache[60][60], sec_cache[60];

void buildcache(void)
{
  int h, m, s;
  for (h = 0; h < 24; h++)
    for (m = 0; m < 60; m++)
      hour_cache[h][m] = h * 30 + m  / 2;
  for (m = 0; m < 60; m++)
    for (s = 0; s < 60; s++)
      min_cache[m][s] = m * 6 + s / 10;
  for (s = 0; s < 60; s++)
    sec_cache[s] = s * 6;
}

double radian(double d)
{
  return d * 3.1416 / 180;
}

double degree(double d)
{
  return d / 3.1416 * 180;
}

#define HAND(name,theta,l,w)\
  double name(double x, double y, double rad) {\
    double t, d, dist, wx, len, innerprod, sint, cost;\
    int b = 0;\
    len = size / l;\
    if (len < rad) return 0;\
    t = radian(180 - (theta));\
    sint = sin(t);\
    cost = cos(t);\
    innerprod = - cost * x + sint * y;\
    if (innerprod < 0) {\
      if (len / 6 < rad) return 0;\
      b = 1;\
    }\
    d = fabs(sint * x + cost * y);\
    wx = len / w;\
    if (d > wx) return 0;\
    dist = d <= wx / 2.4 ? 255 : (wx - d) / wx * 255;\
    if (b) return fmin(dist, (len / 6 - rad) * 255 / wx);\
    if (len < rad + wx) return ((wx - d) / wx * 255 * ((len - rad) / wx));\
    return dist;\
  }

HAND(dhour, hour_cache[hour][min], 3.6, 6);
HAND(dmin, min_cache[min][sec], 2.5, 15);
HAND(dsec, sec_cache[sec], 2.2, 30);

double points(double x, double y, double rad)
{
  double deg, diffdeg;
  int col = 128, col2 = 192, wid = 2;
  if (size / 2.5 > rad) return 0;
  if (size / 2.1 < rad) return 0;
  if (-wid < x && x < wid) return col2;
  if (-wid < y && y < wid) return col2;
  deg = degree(fabs(atan(y / x)));
  if (deg < wid || deg > 90 - wid) return col2;
  diffdeg = fmod(deg, 30);
  if ((diffdeg < wid || diffdeg > 30 - wid) && rad > size / 2.3) return col;
  return 0;
}

double center(double x, double y, double rad)
{
  if (size / 40 < rad) return 0;
  if (size / 120 < rad) return (size / 40 - rad) * 60 / size * 255;
  return 255;
}

void outputbmp(void)
{
  char bm[100] = "BM\0\0\0\0\0\0\0\0\x1a\0\0\0\xc\0\0\0\x1\0\x1\0\x1\0\x18\0";
  int i, j, k, l, h, m, s, p, c;
  double x, y, rad;
  bm[18] = ((int)width) % 256;
  bm[19] = ((int)width) / 256;
  bm[20] = ((int)height) % 256;
  bm[21] = ((int)height) / 256;
  k = (-(int)width*3) & 3;
  for (i = 0; i < 26; i++) printf("%c", bm[i]);
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      x = i - height / 2;
      y = j - width / 2;
      rad = sqrt(x * x + y * y);
      l = ((c = center(x, y, rad)) > 64) ? c :
        ((s = dsec(x, y, rad)) > 64) ? s :
        ((m = dmin(x, y, rad)) > 64) ? m :
        ((h = dhour(x, y, rad)) > 64) ? h :
        ((p = points(x, y, rad)) > 64) ? p :
        fmin(fmin(s, m), fmin(h, fmin(c, h)));
      printf("%c%c%c", l, l, l);
    }
    for (j = 0; j < k; j++) printf("%c", 0);
  }
}

int main(int argc, char *argv[])
{
  struct winsize s;
  time_t clock;
  struct tm *t;
  buildcache();
  ioctl(fileno(fopen("/dev/tty", "r")), TIOCGWINSZ, &s);
  width = height = fmin(s.ws_col, (s.ws_row - 1) * 2);
  size = fmin(width, height) / 1.1;
  do {
    clock = time(NULL);
    t = localtime(&clock);
    hour = (double)(t -> tm_hour % 24);
    min = (double)(t -> tm_min % 60);
    sec = (double)(t -> tm_sec % 60);
    outputbmp();
    fflush(stdout);
    usleep(5e5);
  } while (1);
  return 0;
}


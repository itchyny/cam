/*
 * file: cam.c
 * author: itchyny
 * Last Change: 2013/03/18 19:19:21.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  /* strlen, strcpy */
#include <math.h> /* ceil */
#include <ctype.h> /* isdigit */
#include <err.h> /* warn */
#include <errno.h> /* ERANGE */
#include <fcntl.h> /* O_RDONLY */
#include <signal.h> /* signal */
#include <sys/ioctl.h> /* ioctl, TIOCGWINSZ */
#include <sys/param.h> /* LONG_MAX */
#include <sys/stat.h> /* stat */
#include <unistd.h> /* getopt, usleep */
#ifdef HAVE_CONFIG_H
#  include <config.h> /* PACKAGE, PACKAGE_STRING */
   typedef unsigned char stbi_uc;
   extern stbi_uc *stbi_load_from_file(FILE*, int*, int*, int*, int);
   extern const char *stbi_failure_reason(void);
#else
#  include "stb_image.c" /* stbi_load_from_file */
#endif /* HAVE_CONFIG_H */
#ifndef PACKAGE
#  define PACKAGE "cam"
#endif /* PACKAGE */
#ifndef PACKAGE_STRING
#  define PACKAGE_STRING "cam 0.0.0"
#endif /* PACKAGE_STRING */

#define STDIN_PATH "/dev/stdin"
#define TTY_PATH "/dev/tty"

extern int optind, opterr; /* unistd.h */
extern char *optarg; /* unistd.h */
extern int errno; /* errno.h */

static int terminalwidth, terminalheight, outputwidth, outputheight;
static int
  iflag, /* TODO: bottom information */
  qflag,
  dflag,
  sflag,
  rflag,
  cflag,
  Lflag, PLflag,          /* TODO */
  Rflag, PRflag,          /* TODO */
  Cflag, PCflag,          /* TODO */
  Tflag, PTflag,          /* TODO */
  Bflag, PBflag,          /* TODO */
  Eflag,          /* TODO */
  Pflag,          /* TODO */
  EXCflag,          /* TODO */
  eflag,          /* TODO */
  Sflag;          /* TODO */
static int Wflag, Hflag;
static int wflag, hflag, uflag;
static long wval, hval, Wval, Hval, rval;
static double sval, Sval;
static int filecount;

typedef unsigned char color;

#define getcolor3(red, green, blue)\
  (red / 128) + 2 * (green / 128) + 4 * (blue / 128)

#define getcolor24(red, green, blue)\
  16 + 36 * (red / 43) + 6 * (green / 43) + (blue / 43)

#define ESC "\x1b"
#define CSI "\x1b["

#define CSI_COLOR_FUNC(name,format,num)\
  static inline void name(color red, color green, color blue) {\
    printf(CSI format, num); };
CSI_COLOR_FUNC(setfgcolor3, "%dm", 30 + getcolor3(red, green, blue))
CSI_COLOR_FUNC(setbgcolor3, "%dm", 40 + getcolor3(red, green, blue))
CSI_COLOR_FUNC(setfgcolor24, "38;5;%dm", getcolor24(red, green, blue))
CSI_COLOR_FUNC(setbgcolor24, "48;5;%dm", getcolor24(red, green, blue))
CSI_COLOR_FUNC(coloredspace3, "%dm ", 40 + getcolor3(red, green, blue))
CSI_COLOR_FUNC(coloredspace3_2, "%dm  ", 40 + getcolor3(red, green, blue))
CSI_COLOR_FUNC(coloredspace24, "48;5;%dm ", getcolor24(red, green, blue))
CSI_COLOR_FUNC(coloredspace24_2, "48;5;%dm  ", getcolor24(red, green, blue))

#define ESC_FUNC(name,format)\
  static inline void name(void) {\
    printf(ESC format); };

#define CSI_FUNC(name,format)\
  static inline void name(void) {\
    printf(CSI format); };

CSI_FUNC(erasedown, "J")
CSI_FUNC(eraseup, "1J")
CSI_FUNC(erasescreen, "2J")
CSI_FUNC(erasetoendofline, "K")
CSI_FUNC(erasetostartofline, "1K")
CSI_FUNC(eraseline, "2K")
CSI_FUNC(cursorhide, "?25l")
CSI_FUNC(cursorshow, "?25h")
ESC_FUNC(cursorsave, "7")
ESC_FUNC(cursorunsave, "8")
CSI_FUNC(cursorhome, "H")
CSI_FUNC(scrollscreenall, "r")
CSI_FUNC(setdefaultcolor, "0m")
static inline void newline(void) { printf("\n"); };
static inline void erasescreen_cursorhome(void) {
  erasescreen();
  cursorhome();
}

#define CSI_FUNC1(name,format)\
  static inline void name(int arg) {\
    printf(CSI format, arg); };
CSI_FUNC1(cursorup, "%dA")
CSI_FUNC1(cursordown, "%dB")
CSI_FUNC1(cursorforward, "%dC")
CSI_FUNC1(cursorback, "%dD")
CSI_FUNC1(cursornextline, "%dE")
CSI_FUNC1(cursorpreviousline, "%dF")
CSI_FUNC1(cursorhorizontalabsolute, "%dG")
CSI_FUNC1(scrollup, "%dS")
CSI_FUNC1(scrolldown, "%dT")
static inline void cursorforward_erasetostartofline(int arg) {
  cursorforward(arg);
  erasetostartofline();
}
static inline void cursordown_cursorhorizontalabsolute() {
  cursordown(1);
  cursorhorizontalabsolute(1);
}

#define CSI_FUNC2(name,format)\
  static inline void name(int arg1, int arg2) {\
    printf(CSI format, arg1, arg2); };
CSI_FUNC2(cursormove, "%d;%dH")
CSI_FUNC2(scrollscreen, "%d;%dr")

#define NEWLINE1 "\n"
#define NEWLINE2 NEWLINE1 NEWLINE1
#define NEWLINE4 NEWLINE2 NEWLINE2
#define NEWLINE8 NEWLINE4 NEWLINE4
#define NEWLINE16 NEWLINE8 NEWLINE8
#define NEWLINE32 NEWLINE16 NEWLINE16
static inline void replicatenewline(int count)
{
  while (count >= 32) { printf(NEWLINE32); count -= 32; }
  if (count & 16) printf(NEWLINE16);
  if (count & 8) printf(NEWLINE8);
  if (count & 4) printf(NEWLINE4);
  if (count & 2) printf(NEWLINE2);
  if (count & 1) printf(NEWLINE1);
}

#define SPACE1 " "
#define SPACE2 SPACE1 SPACE1
#define SPACE4 SPACE2 SPACE2
#define SPACE8 SPACE4 SPACE4
#define SPACE16 SPACE8 SPACE8
#define SPACE32 SPACE16 SPACE16
#define SPACE64 SPACE32 SPACE32
static inline void replicatespace(int count)
{
  while (count >= 64) { printf(SPACE64); count -= 64; }
  if (count & 32) printf(SPACE32);
  if (count & 16) printf(SPACE16);
  if (count & 8) printf(SPACE8);
  if (count & 4) printf(SPACE4);
  if (count & 2) printf(SPACE2);
  if (count & 1) printf(SPACE1);
}

static void saftyexit(int status)
{
  setdefaultcolor();
  newline();
  if (uflag) {
    cursorunsave();
  }
  cursorshow();
  exit(status);
}

static void interuppthandler(int signal)
{
  saftyexit(EXIT_FAILURE);
}

#define CHECKNUM(x,y) \
  if ((errno == ERANGE && (val == (x) || val == (y))) \
      || (errno != 0 && val == 0) \
      || (endptr == str) \
      || (nonneg && val < 0)) return 1; \
  else return 0;

static inline int checknuml(long val, char *str, char *endptr, int nonneg)
{
  CHECKNUM(LONG_MAX, LONG_MIN);
}

static inline int checknumd(double val, char *str, char *endptr, int nonneg)
{
  CHECKNUM(HUGE_VAL, -HUGE_VAL);
}

static void usage(void)
{
  (void)fprintf(stderr,
      "usage: "
      PACKAGE
      " [ -iqd |"
      " -LRCTBE |"
      " -v! |"
      " -w width(%%) |"
      " -W width(character) |"
      " -h height(%%) |"
      " -H height(character) |"
      " -[sS] duration(sec) |"
      " -r repeat_count"
      " ] file ...\n");
  fflush(stdout);
  fflush(stderr);
  exit(EXIT_FAILURE);
}

static void version(void)
{
  printf(PACKAGE_STRING "\n");
  fflush(stdout);
  exit(EXIT_SUCCESS);
}

#define CONER      "+"
#define HORIZ_HALF "----------"
#define SPACE_HALF "          "
#define HORIZ_MID  "----"
#define STAR       "****"
#define SPACE      "    "
#define NL         "\n"
#define VL         "|"
#define HC CONER HORIZ_HALF HORIZ_MID HORIZ_HALF CONER
#define HL VL SPACE_HALF
#define HR SPACE_HALF VL
#define BL HL SPACE HR
#define SR HL SPACE_HALF STAR VL
#define SL VL STAR SPACE_HALF HR
#define SM HL STAR HR
#define S2 SPACE SPACE
#define S3 SPACE SPACE SPACE
#define S5 SPACE SPACE SPACE SPACE SPACE
#define ZN HC NL
#define BB BL NL BL NL
#define HT SR NL SR NL
#define SN SL NL SL NL
#define HU SM NL SM NL
#define HP HC SPACE HC SPACE HC NL
#define HS SL SPACE SM SPACE SR NL
#define HH HS HS
#define BS BL SPACE BL SPACE BL NL BL SPACE BL SPACE BL NL
static void position(void)
{
  if (Rflag && !Lflag) {
    if (Bflag)      printf(ZN BB BB HT ZN);
    else if (Tflag) printf(ZN HT BB BB ZN);
    else if (Cflag) printf(ZN BB HT BB ZN);
    else            printf(ZN HT HT HT ZN);
  } else if (Lflag && !Rflag) {
    if (Bflag)      printf(ZN BB BB SN ZN);
    else if (Tflag) printf(ZN SN BB BB ZN);
    else if (Cflag) printf(ZN BB SN BB ZN);
    else            printf(ZN SN SN SN ZN);
  } else if ((Lflag && Rflag) || Cflag || Tflag || Bflag) {
    if (Bflag)      printf(ZN BB BB HU ZN);
    else if (Tflag) printf(ZN HU BB BB ZN);
    else if (Cflag) printf(ZN BB HU BB ZN);
    else            printf(ZN HU HU HU ZN);
  } else {
    printf("Position description" NL NL
      "--, (L)" S5   "   LR" S5 S2         "R"            NL HP HH HH HH HP NL
      "TL, (TLC)" S5 " T, (TC, TLR, TCLR)" S3 "TR, (TRC)" NL HP HH BS BS HP NL
      "CL" S5 S2  "C, (CLR)" S5            "  CR"         NL HP BS HH BS HP NL
      "BL, (BLC)" S5 " B, (BC, BLR, BCLR)" S3 "BR, (BRC)" NL HP BS BS HH HP NL
    );
  }
  fflush(stdout);
  exit(EXIT_SUCCESS);
}

static inline void terminalsize(void)
  /* terminalwidth = ((winsize)size).ws_col
   *               = 80
   * terminalheight = ((winsize)size).ws_row - 1
   *                = 25
   * outputwidth = Wval                                           Wflag && Wval > 0
   *             = ceil(terminalwidth * wval / 100)               wflag && wval > 0
   *             = terminalwidth                                  otherwise
   * outputheight = Hval - iflag                                  Hflag && Hval > 0
   *              = ceil(terminalheight * hval / 100) - iflag     hflag && hval > 0
   *              = terminalheight - iflag                        otherwise */
{
  struct winsize size;
  FILE *console;
  terminalwidth = 110; terminalheight = 34;
  if ((console = fopen(TTY_PATH, "r")) == NULL &&
       ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0) {
    terminalwidth = size.ws_col;
    terminalheight = size.ws_row - 1;
  } else if (ioctl(fileno(console), TIOCGWINSZ, &size) == 0) {
    terminalwidth = size.ws_col;
    terminalheight = size.ws_row - 1;
  }
  if (Wflag && Wval > 0) {
    outputwidth = Wval;
  } else if (wflag && wval > 0) {
    outputwidth = (int)ceil((double)(terminalwidth * wval) / 100);
  } else {
    outputwidth = terminalwidth;
  }
  if (Hflag && Hval > 0) {
    outputheight = Hval;
  } else if (hflag && hval > 0) {
    outputheight = (int)ceil((double)(terminalheight * hval) /100);
  } else {
    outputheight = terminalheight;
  }
  if (iflag) outputheight--;
  if (terminalwidth < 1) terminalwidth = 1;
  if (terminalheight < 1) terminalheight = 1;
  if (outputwidth < 1) outputwidth = 1;
  if (outputheight < 1) outputheight = 1;
}

void inline getaverage(color *p, int istep, int jstep, int count,
    int offsetjstep, int *red, int *green, int *blue)
/* This function doesn't check if the pointer is valid to reduce overhead.
 * The pointer p must be inside the image and following must be kept
 *   i + istep <= height
 *   j + jstep <= width
 *   offset = (i * width + j) * comp
 *   p = image + offset
 *   offsetjstep = (width - jstep) * comp
 *   jstep >= 1
 *   istep >= 1
 */
{
  int k = istep, l;
  *red = *green = *blue = 0;
  do {
    l = jstep;
    do {
      *red += *p++;
      *green += *p++;
      *blue += *p++;
    } while (--l);
    p += offsetjstep;
  } while (--k);
  *red /= count;
  *green /= count;
  *blue /= count;
}

static void outputcolor_1(color *image, int height, int width, int offset,
    int i, int j, int istep, int jstep, int comp)
{
  int red, green, blue, offsetjstep, count;
  count = jstep * istep;
  offsetjstep = (width - jstep) * comp;
  getaverage(image + offset, istep, jstep, count, offsetjstep,
      &red, &green, &blue);
  /* printf("%c", getcolor24(red, green, blue)); */
  coloredspace24(red, green, blue);
}

static void outputcolor_2(color *image, int height, int width, int offset,
    int i, int j, int istep, int jstep, int comp)
{
  coloredspace24_2(image[offset], image[offset + 1], image[offset + 2]);
}

static void outputcolor_3(color *image, int height, int width, int offset,
    int i, int j, int istep, int jstep, int comp)
{
  int red, green, blue, offsetjstep, count;
  count = jstep * istep;
  offsetjstep = (width - jstep) * comp;
  getaverage(image + offset, istep, jstep, count, offsetjstep,
      &red, &green, &blue);
  coloredspace3(red, green, blue);
}

static void outputcolor_4(color *image, int height, int width, int offset,
    int i, int j, int istep, int jstep, int comp)
{
  coloredspace3_2(image[offset], image[offset + 1], image[offset + 2]);
}


#define WARN(a,b) if (!qflag) { warn(a,b); }

#define WARNX(a,b) if (!qflag) { warnx(a,b); }

#define PARSEARGL(a)\
  do {\
    long templ;\
    if (isdigit(optarg[0]) || \
        (optarg[0] == '+' && isdigit(optarg[1]))) {\
      errno = 0;\
      templ = strtol(optarg, &endptr, 10);\
      if (checknuml(templ, argv[optind], endptr, 1)) {\
        WARNX("illegal argument -- %c", optopt);\
        usage();\
      }\
      if (*endptr != '\0' && *endptr != ' ') {\
        if (optarg != argv[optind - 1])\
        strcpy(optarg - 1, endptr);\
        optind--;\
      } else {\
        a = templ;\
      }\
    } else {\
      if (optarg != argv[optind - 1])\
      strcpy(optarg - 1, optarg);\
      optind--;\
    }\
  } while (0)

#define PARSEARGD(a)\
  do {\
    double tempd;\
    if (isdigit(optarg[0]) ||\
        ((optarg[0] == '.' || optarg[0] == '+') && isdigit(optarg[1])) || \
        (optarg[0] == '+' && optarg[1] == '.' && isdigit(optarg[2]))) {\
      errno = 0;\
      tempd = strtod(optarg, &endptr);\
      if (checknumd(tempd, argv[optind], endptr, 1)) {\
        WARNX("illegal argument -- %c", optopt);\
        usage();\
      }\
      if (*endptr != '\0' && *endptr != ' ') {\
        if (optarg != argv[optind - 1])\
        strcpy(optarg - 1, endptr);\
        optind--;\
      } else {\
        a = tempd;\
      }\
    } else {\
      if (optarg != argv[optind - 1])\
      strcpy(optarg - 1, optarg);\
      optind--;\
    }\
  } while (0)

static int checkstdin(void)
{
  struct stat sb;
  if (stat(STDIN_PATH, &sb) < 0)
    return -1;
  if ((sb.st_mode & S_IFMT) == S_IFCHR)
    return -1;
  return 0;
}

static int deal(FILE *fp, char *filename, int pipemode)
{
  color *image;
  void (*outputcolor)();
  int width, height, comp, i, j, istep, jstep, ch, scale, scalew, scaleh,
      offset, imagewidth, imageheight,
      margintopbottom, marginleftright, marginleft, margintop,
      paddingtopbottom, paddingleftright, paddingleft, paddingright,
      paddingtop, paddingbottom,
      ipaddingleftright, ipaddingleft, ipaddingright;

  image = stbi_load_from_file(fp, &width, &height, &comp, 0);

  /* error handling */
  if (image == NULL) {
    if ((!qflag) && (stbi_failure_reason() != NULL))
      warnx("%s: %s", filename, stbi_failure_reason());
    goto ERR_DEAL;
  }
  if (width <= 0 || height <= 0) {
    if (!qflag)
      warnx("%s %s (width: %d, height: %d)", filename,
          "is an invalid image", width, height);
    goto ERR_DEAL;
  }
  /* ok, it is surely a valid image */

  /* get terminal size before processing each image */
  terminalsize();

  /* set scale parameters */
  scale = 1;
  if (Sflag) {
    if (cflag) outputcolor = outputcolor_3;
    else       outputcolor = outputcolor_1;
    scale = (int)Sval; /* TODO */
    istep = 2 * (jstep = scale);
    imageheight = (int)ceil((double)height / (double)istep);
    imagewidth = (int)ceil((double)width / (double)jstep);
  } else if (2 * width <= outputwidth && height <= outputheight) {
    if (cflag) outputcolor = outputcolor_4;
    else       outputcolor = outputcolor_2;
    istep = jstep = 1;
    imagewidth = width * 2;
    imageheight = height;
  } else {
    if (cflag) outputcolor = outputcolor_3;
    else       outputcolor = outputcolor_1;
    scalew = (int)ceil((double)width / (double)outputwidth);
    scaleh = (int)ceil((double)height / 2.0 / (double)outputheight);
    if (Hval == 0 && hval == 0 && (wval > 0 || Wval > 0))      scale = scalew;
    else if ((hval > 0 || Hval > 0) && wval == 0 && Wval == 0) scale = scaleh;
    else                             scale = ((scalew) > (scaleh) ? (scalew) : (scaleh));
    istep = 2 * (jstep = scale);
    imageheight = (int)ceil((double)height / (double)istep);
    imagewidth = (int)ceil((double)width / (double)jstep);
  }
  if (istep * jstep >= INT_MAX / 256 || width * height >= INT_MAX / 3) {
    if (!qflag)
      warnx("%s %s (width: %d, height: %d)",
          filename, "is too large", width, height);
    goto ERR_DEAL;
  }

  /* setting parameters */
  margintopbottom = terminalheight - outputheight - iflag;
  marginleftright = terminalwidth - outputwidth;
  paddingtopbottom = outputheight - imageheight;
  paddingleftright = outputwidth - imagewidth;

  if (Rflag && !Lflag) {
    marginleft = marginleftright;
  } else if (Lflag && !Rflag) {
    marginleft = 0;
  } else if ((Lflag && Rflag) || Cflag || Tflag || Bflag) {
    marginleft = marginleftright / 2;
  } else {
    marginleft = 0;
  }

  if (PRflag && !PLflag) {
    paddingleft = paddingleftright;
  } else if (PLflag && !PRflag) {
    paddingleft = 0;
  } else if ((PLflag && PRflag) || PCflag || PTflag || PBflag) {
    paddingleft = paddingleftright / 2;
  } else if (Rflag && !Lflag) {
    paddingleft = paddingleftright;
  } else if (Lflag && !Rflag) {
    paddingleft = 0;
  } else if ((Lflag && Rflag) || Cflag || Tflag || Bflag) {
    paddingleft = paddingleftright / 2;
  } else {
    paddingleft = 0;
  }

  if (Bflag) {
    margintop = margintopbottom;
  } else if (Tflag) {
    margintop = 0;
  } else if (Cflag) {
    margintop = margintopbottom / 2;
  } else {
    margintop = 0;
  }

  if (PBflag) {
    paddingtop = paddingtopbottom;
  } else if (PTflag) {
    paddingtop = 0;
  } else if (PCflag) {
    paddingtop = paddingtopbottom / 2;
  } else if (Bflag) {
    paddingtop = paddingtopbottom;
  } else if (Tflag) {
    paddingtop = 0;
  } else if (Cflag) {
    paddingtop = paddingtopbottom / 2;
  } else {
    paddingtop = 0;
  }

  paddingright = paddingleftright - paddingleft;
  paddingbottom = paddingtopbottom - paddingtop;

  /* start up */
  setdefaultcolor();
  if (uflag) {
    cursorhide();
    cursorsave();
  }

  /* main process */
  if (Eflag && filecount == 0) erasescreen();
  if (Cflag || Tflag || Bflag) {
    if (!eflag) {
      if (margintop >= 0) cursormove(margintop + 1, 1);
      for (i = 0; i < paddingtop; i++) {
        if (marginleft > 0) cursorforward(marginleft);
        replicatespace(outputwidth);
        cursordown_cursorhorizontalabsolute();
      }
    }
    if (margintop + paddingtop >= 0) cursormove(margintop + paddingtop + 1, 1);
  }
  if (iflag) {
    ipaddingleftright = outputwidth - strlen(filename);
    if (PRflag && !PLflag) {
      ipaddingleft = ipaddingleftright;
    } else if (PLflag && !PRflag) {
      ipaddingleft = 0;
    } else if ((PLflag && PRflag) || PCflag || PTflag || PBflag) {
      ipaddingleft = ipaddingleftright / 2;
    } else if (Rflag && !Lflag) {
      ipaddingleft = ipaddingleftright;
    } else if (Lflag && !Rflag) {
      ipaddingleft = 0;
    } else if ((Lflag && Rflag) || Cflag || Tflag || Bflag) {
      ipaddingleft = ipaddingleftright / 2;
    } else {
      ipaddingleft = 0;
    }
    ipaddingright = ipaddingleftright - ipaddingleft;
    if (Cflag || Lflag || Rflag || Tflag || Bflag ||
        PCflag || PLflag || PRflag || PTflag || PBflag) {
      if (marginleft > 0) cursorforward(marginleft);
      if (ipaddingleft > 0) {
        if (eflag) {
          cursorforward(ipaddingleft);
        } else {
          replicatespace(ipaddingleft);
        }
      } else if (ipaddingleftright < 0) {
        filename[outputwidth] = '\0';
      }
    }
    printf("%s", filename);
    if (Cflag || Tflag || Bflag || PCflag || PTflag || PBflag) {
      if (ipaddingright > 0) {
        if (!eflag) {
          replicatespace(ipaddingright);
        }
      }
      if (Cflag || Tflag || Bflag) {
        cursordown_cursorhorizontalabsolute();
      } else {
        newline();
      }
    } else {
      newline();
    }
  }
  for (i = 0; i < height; i += istep) {
    if (Cflag || Lflag || Rflag || Tflag || Bflag ||
        PCflag || PLflag || PRflag || PTflag || PBflag) {
      if (marginleft > 0) cursorforward(marginleft);
      if (paddingleft > 0) {
        if (eflag) {
          cursorforward(paddingleft);
        } else {
          replicatespace(paddingleft);
        }
      }
    }
    for (j = 0; j < width; j += jstep) {
      offset = (i * width + j) * comp;
      outputcolor(image, height, width, offset, i, j,
          (i + istep > height ? height - i : istep),
          (j + jstep > width ? width - j : jstep), comp);
    }
    setdefaultcolor();
    if (Cflag || Tflag || Bflag || PCflag || PTflag || PBflag) {
      if (paddingright > 0) {
        if (!eflag) {
          replicatespace(paddingright);
        }
      }
      if (Cflag || Tflag || Bflag) {
        cursordown_cursorhorizontalabsolute();
      } else {
        newline();
      }
    } else {
      newline();
    }
  }
  if (Cflag || Tflag) {
    if (!eflag) {
      for (i = 0; i < paddingbottom; i++) {
        if (marginleft > 0) cursorforward(marginleft);
        if (outputwidth > 0) replicatespace(outputwidth);
        cursordown_cursorhorizontalabsolute();
      }
    }
  }

  /* clean up */
  setdefaultcolor();
  if (uflag) {
    cursorunsave();
    cursorshow();
  }
  fflush(stdout);
  stbi_image_free(image);
  ++filecount;

  /* debug mode */
  if (dflag) {
    cursorsave();
    cursormove(1, 1);
    printf(PACKAGE);
    if (iflag) printf(" -i");
    if (qflag) printf(" -q");
    if (sflag) printf(" -s %lf", sval);
    if (rflag) printf(" -r %ld", rval);
    if (Lflag) printf(" -L");
    if (Rflag) printf(" -R");
    if (Cflag) printf(" -C");
    if (Tflag) printf(" -T");
    if (Bflag) printf(" -B");
    if (Eflag) printf(" -E");
    if (hflag) printf(" -h %ld", hval);
    if (Hflag) printf(" -H %ld", Hval);
    if (wflag) printf(" -w %ld", wval);
    if (Wflag) printf(" -W %ld", Wval);
    printf(" %s\n", filename);
    printf("terminalwidth: %d\n", terminalwidth);
    printf("terminalheight: %d\n", terminalheight);
    printf("outputwidth: %d\n", outputwidth);
    printf("outputheight: %d\n", outputheight);
    printf("imagewidth: %d\n", imagewidth);
    printf("imageheight: %d\n", imageheight);
    printf("margintop: %d\n", margintop);
    printf("marginleft: %d\n", marginleft);
    printf("paddingtop: %d\n", paddingtop);
    printf("paddingleft: %d\n", paddingleft);
    printf("paddingbottom: %d\n", paddingbottom);
    printf("paddingright: %d\n", paddingright);
    fflush(stdout);
    cursorunsave();
  }

  /* check next file if pipe or fifo */
  if (pipemode) {
    ch = getc(fp);
    if (ch != EOF) {
      if (ch != '\0' && ch != '\n') {
        ungetc(ch, fp);
      } else if ((ch = getc(fp)) != '\0' && ch != '\n') {
        ungetc(ch, fp);
      } else if ((ch = getc(fp)) != '\0' && ch != '\n') {
        ungetc(ch, fp);
      }
      if (ch != EOF) {
        if (sflag && sval > 0) {
          usleep(sval * 1e6);
        }
        deal(fp, filename, pipemode);
      }
    }
  }

  return 0;

ERR_DEAL:
  return 1;
}

static int dealfile(char *filename)
{
  struct stat sb;
  FILE *fp;
  int result, pipemode;

  pipemode = 0;
  /* file really exists? surely a file? */
  if (stat(filename, &sb) < 0) {
    WARN("%s", filename);
    goto ERR;
  } else {
    switch (sb.st_mode & S_IFMT) {
      case S_IFBLK:  WARNX("%s: is a block device", filename); goto ERR;
      case S_IFCHR:  WARNX("%s: is a character device", filename); goto ERR;
      case S_IFDIR:  WARNX("%s: is a directory", filename); goto ERR;
      case S_IFIFO:  pipemode = 1; break;
      case S_IFLNK:  WARNX("%s: is a symbolic link", filename); goto ERR;
      case S_IFREG:  break;
      case S_IFSOCK: WARNX("%s: is a socket", filename); goto ERR;
      default:       WARNX("%s: is a unknown?", filename); goto ERR;
    }
  }
  /* an empty file? (because stbi_load gets segmentation fault) */
  if (sb.st_size == 0 && (!pipemode)) {
    WARNX("%s: is a blank file", filename);
    goto ERR;
  }

  /* ok, it is surely a file (but it can be non-image file), load the file */
  fp = fopen(filename, "rb");
  if (fp == NULL) {
    WARN("%s", filename);
    goto ERR;
  }

  /* deal image */
  result = deal(fp, filename, pipemode);
  fclose(fp);
  return result;

ERR:
  return 1;
}

int main(int argc, char *argv[])
{
  char *endptr;
  int ch, result, i, j, status, filemode;

  signal(SIGINT, interuppthandler);
  signal(SIGQUIT, interuppthandler);
  signal(SIGTERM, interuppthandler);
  signal(SIGFPE, interuppthandler);
  signal(SIGSEGV, interuppthandler);

  rval = 1;
  status = EXIT_SUCCESS;

  while ((ch = getopt(argc, argv, ":iqds:r:vLRCTBP!ecEuw:h:W:H:S:")) != -1) {
    switch (ch) {
      case 'i': iflag = 1; break;
      case 'q': qflag = 1; break;
      case 'd': dflag = 1; break;
      case 's': sflag = 1; sval = 0; PARSEARGD(sval); break;
      case 'r': rflag = 1; rval = 0; PARSEARGL(rval); break;
      case 'v': version(); break; /* exit */

      case 'L': if (Pflag) { PLflag = 1; } else { Lflag = 1; } break;
      case 'R': if (Pflag) { PRflag = 1; } else { Rflag = 1; } break;
      case 'C': if (Pflag) { PCflag = 1; } else { Cflag = 1; } break;
      case 'T': if (Pflag) { PTflag = 1; PBflag = 0; }
                else { Tflag = 1; Bflag = 0; }
                break;
      case 'B': if (Pflag) { PBflag = 1; PTflag = 0; }
                else { Bflag = 1; Tflag = 0; }
                break;
      case 'P': Pflag = 1; break;
      case 'E': Eflag = 1; break;
      case '!': EXCflag = 1; break;

      case 'w': wflag = 1; PARSEARGL(wval); break;
      case 'h': hflag = 1; PARSEARGL(hval); break;
      case 'W': Wflag = 1; PARSEARGL(Wval); break;
      case 'H': Hflag = 1; PARSEARGL(Hval); break;
      case 'S': Sflag = 1; Sval = 1; PARSEARGD(Sval); break;

      case 'e': eflag = 1; break;
      case 'c': cflag = 1; break;
                /*
                 * o: offset (%)
                 * O: offset (character) 0,-20
                 */

      case 'u': uflag = 1; break; /* TODO */

      case ':': switch (optopt) {
                  case 'w': wflag = 1; wval = 100; break;
                  case 'h': hflag = 1; hval = 100; break;
                  case 's': sflag = 1; sval = 0; break;
                  case 'r': rflag = 1; rval = 0; break;
                  case 'S': Sflag = 1; Sval = 1; break;
                  default: usage();
                }
                break;
      case '?': WARNX("illegal option -- %c", optopt); /* fall through */
      default: usage(); /* exit */
    }
  }
  argc -= optind;
  argv += optind;

  if (EXCflag) {
    position(); /* exit */
  }

  /* we expect file as arguments here, but if nothing specified, use stdin */
  if (argc <= 0) {
    /* but, if stdin has no data, display usage and finish process */
    if (checkstdin() != 0) {
      usage();
    }
    argc = 1;
    rval = 1;
    filemode = 0;
  } else {
    filemode = 1;
  }

  cursorhide();
  if (uflag) {
    cursorsave();
  }

  /* main process */
  for (i = 0; i < rval || rval <= 0; ) {
    /* for all over the files */
    for (j = 0; j < argc; j++) {
      /* deal an image file */
      if (filemode) {
        result = dealfile(argv[j]);
      } else {
        result = deal(stdin, STDIN_PATH, 1);
      }
      /* if everything went well */
      if (result == 0) {
        /* and in slideshow mode */
        if (sflag && sval > 0) {
          usleep(sval * 1e6);
        }
      } else {
        /* fail and no error so far */
        if (status == EXIT_SUCCESS) {
          status = EXIT_FAILURE;
        }
      }
    }
    if (rval > 0) {
      i++;
    }
  }

  if (uflag) {
    cursorunsave();
  }
  cursorshow();

  return status;
}


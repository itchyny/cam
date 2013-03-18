<!-- File: spec/README.md -->
<!-- Author: itchyny -->
<!-- Last Change: 2013/03/18 20:28:30. -->

# cam
## Requirement

    `I want to view my images inside the terminal.'

Everyone may have wished the same thing like this.
But until now, there is no good tool (in my opinion) to fulfill this wish.
The only thing I want the program to be is _UNIX-like_, which means we can use as if we use other traditional unix tools.
Also, I mean it can be run on any architectures: portability.

## Specification
### Basic usage
The command:

    cam file1

outputs the image contents of `file1` to stdout, if `file1` is a valid image file.

When multiple files are given:

    cam file1 file2

first outputs the image contents of `file1`, and next `file2` to stdout.

### File check
Before loading each file, `cam` checks if the file is valid.
A valid file is: *a regular file* or *a FIFO*.
Also, `cam` checks if the file is not a blank file.

### File information (-i)
#### Default
This option is disabled in default.

#### Argument
This option takes no arguments.

#### Detail
If this option is set, `cam` outputs the `filename` to the same place as the image (center if centering mode), before each image contents.

#### Position
Basically, output the `filename` using `printf("%s\n", filename)`.
In `centering mode`, move the `filename` position to the center of the image.

### Error (-q)
#### Default
All the error messages are sent to stderr.
If the option `-q` is set, error messages are suppressed(_quiet_).

#### Basic
Even if an error occurs while processing a file, `cam` tries to handle all the given file.

#### Return status
The `cam` utility exits EXIT\_SUCCESS on success, and EXIT\_FAILURE if _an_ error occurs.

### Debug (-d)
#### Default
Display debug information at the top of the screen. Off in default.

#### Basic
If this option is set, `cam` shows various information.

### Slideshow (-s duration)
#### Default
This option is disabled in default.

#### Argument
The argument can be a non-negative double number, and parsed by `strtod` function.
This argument is interpreted as the interval duration in sec.
When this argument is omitted, the duration is set to 0.

#### Detail
If the option `-s duration` is specified, `cam` toggles `slideshow mode` on.
In the slideshow mode, _after_ processing an image file with no error, sleep for the specified duration.

### Repeat mode (-r repeat\_count)
#### Default
In default,

    repeat_count = 1

#### Argument
Argument can be a non-negative long number, and parsed by `srtol` function.

#### Detail
Repeat viewing all the images for specified times.
If there's no error, basically:

    cam -r 3 [files]
    == cam [files]; cam [files]; cam [files]

and if `repeat_count == 0` then `cam` repeats forever.

### Terminal size
#### Default

    width: 80
    height: 25

#### Detail
When `cam` is ready to output an image, it detects the terminal size.
This process is executed _before_ outputting each valid image file.
If `cam` fails in getting the terminal size, it uses the default fixed value.


`cam` regards terminal height as one line smaller than the actual height.
The reason is as follows.
The user who types

    cam -h 100 [file]

may wish the output to be fitted to the terminal height, the whole the image can be seen.
But the new prompt line exists.
If the image height is _just fit_ to the `outputheight`, which is equals to the real terminal height, the top one line is hidden due to the prompt.
Thus `cam` uses:

    terminalheight = ((winsize)size).ws_row - 1

to avoid this problem.

At first I thought `cam` needs not do this process in slideshow mode because there's no trailing prompt line except for the last image.
But changing the outputheight whether in slideshow mode or not or last image or not is an ugly idea: it's not consistent.


### Output size (-w wval, -h hval, -W Wval -H Hval)
#### Default
Default value is:

    -w 100
    -h 100
    -W ---
    -H ---

#### Argument (-w, -h)
Argument can be a non-negative long number, and parsed by `strtol` function.
And these are interpreted as the percentage to the output size.
If omitted, set to 100.

#### Argument (-W, -H)
Argument can be a non-negative long number, and parsed by `strtol` function.
And these are interpreted as the output size.
If omitted, the option is not used.

#### Detail
After `cam` detects the terminal size, it determines the output size.
If the option `-W Wval` is set and the value is positive, `outputwidth` is set equal to `Wval`.

    outputwidth = Wval

Otherwise, if the option `-w wval` is set, `outputwidth` is set using the following relation.

    outputwidth = ceil(terminalwidth * wval / 100)

The parameter `outputheight` is determined by almost the same procedure as `outputwidth`.
There's one difference about the height parameter.
If `-i` flag is set, `cam` reduces `outputheight` by one.
To conclude:

    terminalwidth = ((winsize)size).ws_col
                  = 80
    terminalheight = ((winsize)size).ws_row - 1
                   = 25

    outputwidth = Wval                                           Wflag && Wval > 0
                = ceil(terminalwidth * wval / 100)               wflag && wval > 0
                = terminalwidth                                  otherwise
    outputheight = Hval - iflag                                  Hflag && Hval > 0
                 = ceil(terminalheight * hval / 100) - iflag     hflag && hval > 0
                 = terminalheight - iflag                        otherwise

### Image size
#### Basic
`cam` automatically resizes the image to fit to the _output size_.
If the image is too large, the image is shrinked.

#### Detail
There are two cases; the case when the image is too small, the case when the image is not so small.


Firstly, consider the case where the image is too small.
Let us denote the size of the image by `width` and `height`.
When the inequality

    2 * width <= outputwidth && height <= outputheight

is satisfied, the program `cam` uses two spaces for each pixels.
In this case, the parameters `imagewidth` and `imageheight`  are calculated as follows:

    imagewidth = width * 2
    imageheight = height


Secondly, consider the case where the image is not so small.
In this case, `cam` calculates the parameter `scale` as follows:

    scalew = ceil(width / outputwidth)
    scaleh = ceil(height / 2.0 / outputheight)
    scale = scalew                  Hval == 0 && hval == 0 && (wval > 0 || Wval > 0)
          = scaleh                  (hval > 0 || Hval > 0) && wval == 0 && Wval == 0
          = max(scalew, scaleh)     otherwise

Using this parameter `scale`, the program `cam` calculates the parameters `imagewidth` and `imageheight` as follows:

    imagewidth = ceil(width / scale)
    imageheight = ceil(height / 2 / scale)

The program `cam` averages `scale` times `scale * 2` pixels for each halfwidth space.


There are two _just fit_ cases.
The first case is

    2 * width == outputwidth
    height == outputheight

and the another case is

    width == outputwidth
    height == 2 * outputheight

The program `cam` averages two vertically adjacent pixels and output one half width space.


The following inequalities should be satisfied.

    (width <= scale * outputwidth && height <= scale * 2 * outputheight) &&
    ((scale - 1) * outputwidth < width || (scale - 1) * 2 * outputheight < height)


### Positioning (-LRCTB)
#### Default
These options are disabled in default.

#### Argument
This option takes no arguments.

#### Detail
If this option is set, `cam` outputs the image contents at the center of the terminal.
If `outputwidth` is larger than `terminalwidth`, or `outputheight` is larger than `terminalheight`, `cam` gives up to make the image center though.
But, if `-c` is set, the position of the filename is always at the center of the image.

    <-----                           terminalwidth                        ----->
    +--------------------------------------------------------------------------+ ^
    |   ^                                                                      | |
    |margintop                                                                 | |
    |   v      <-----                 outputwidth             ----->           | |
    |          +---------------------------------------------------+ ^         | |
    |          |    ^                                              | |         |
    |          |paddingtop                                         | |         |
    |          |    v  <-----         imagewidth     ----->        | |         |
    |          |       +----------------------------------+^       |           |
    |          |       |                                  ||       |           |
    |          |       |                                  ||       |           |
    |          |       |                                  |        |           |
    |          |       |                                  |        |           | terminalheight
    |          |       |                                  |    outputheight    |
    |          |       |                              imageheight  |           |
    |          |       |                                  |        |           |
    |          |       |                                  |        |           |
    |          |<-   ->|                                  |<-    ->|           |
    |         paddingleft                                paddingright          |
    |          |       |                                  |        |           |
    |<-      ->|       |                                  ||       |<-       ->|
    |marginleft|       |                                  ||       |marginright|
    |          |       +----------------------------------+v       | |         |
    |          |   ^                                               | |         |
    |          |paddingbottom                                      | |         |
    |          |   v                                               | |         |
    |          +---------------------------------------------------+ v         | |
    |   ^                                                                      | |
    |marginbottom                                                              | |
    |   v                                                                      | |
    +--------------------------------------------------------------------------+ v

    terminalwidth = ((winsize)size).ws_col
                  = 80
    terminalheight = ((winsize)size).ws_row - 1
                   = 25

    outputwidth = Wval                                           Wflag && Wval > 0
                = ceil(terminalwidth * wval / 100)               otherwise
    outputheight = Hval - iflag                                  Hflag && Hval > 0
                 = ceil(terminalheight * hval / 100) - iflag     otherwise

    scalew = ceil(width / outputwidth)
    scaleh = ceil(height / 2.0 / outputheight)
    scale = scalew         Hval == 0 && hval == 0 && (wval > 0 || Wval > 0)
          = scaleh         (hval > 0 || Hval > 0) && wval == 0 && Wval == 0
          = max(scalew, scaleh)     otherwise

    imagewidth = width * 2                    2 * width <= outputwidth && height <= outputheight
               = ceil(width / scale)          otherwise
    imageheight = height                      2 * width <= outputwidth && height <= outputheight
                = ceil(height / 2 / scale)    otherwise

    paddingtopbottom = outputheight - imageheight
    paddingleftright = outputwidth - imagewidth

    margintopbottom = terminalheight - outputheight - iflag
    marginleftright = terminalwidth - outputwidth

    margintop  = margintopbottom         Bflag
               = 0                       Tflag
               = margintopbottom / 2     Cflag
               = 0                       otherwise

    marginleft = marginleftright         Rflag && !Lflag
               = 0                       Lflag && !Rflag
               = marginleftright / 2     (Lflag && Rflag) || Cflag || Tflag || Bflag
               = 0                       otherwise

    paddingtop = paddingtopbottom        PBflag
               = 0                       PTflag
               = paddingtopbottom / 2    PCflag
               = paddingtopbottom        Bflag
               = 0                       Tflag
               = paddingtopbottom / 2    Cflag
               = 0                       otherwise

    paddingleft = paddingleftright       PRflag && !PLflag
                = 0                      PLflag && !PRflag
                = paddingleftright / 2   (PLflag && PRflag) || PCflag || PTflag || PBflag
                = paddingleftright       PRflag && !PLflag
                = 0                      Lflag && !Rflag
                = paddingleftright / 2   (Lflag && Rflag) || Cflag || Tflag || Bflag
                = 0                      otherwise


    Position figure for margin
    (1) --, (L)                   (2) LR                        (3) R
    +------------------------+    +------------------------+    +------------------------+
    |****                    |    |          ****          |    |                    ****|
    |****                    |    |          ****          |    |                    ****|
    |****                    |    |          ****          |    |                    ****|
    |****                    |    |          ****          |    |                    ****|
    |****                    |    |          ****          |    |                    ****|
    |****                    |    |          ****          |    |                    ****|
    +------------------------+    +------------------------+    +------------------------+

    (4) TL, (TLC)                 (5) T, (TC, TLR, TCLR)        (6) TR, (TRC)
    +------------------------+    +------------------------+    +------------------------+
    |****                    |    |          ****          |    |                    ****|
    |****                    |    |          ****          |    |                    ****|
    |                        |    |                        |    |                        |
    |                        |    |                        |    |                        |
    |                        |    |                        |    |                        |
    |                        |    |                        |    |                        |
    +------------------------+    +------------------------+    +------------------------+

    (7) CL                        (8) C, (CLR)                  (9) CR
    +------------------------+    +------------------------+    +------------------------+
    |                        |    |                        |    |                        |
    |                        |    |                        |    |                        |
    |****                    |    |          ****          |    |                    ****|
    |****                    |    |          ****          |    |                    ****|
    |                        |    |                        |    |                        |
    |                        |    |                        |    |                        |
    +------------------------+    +------------------------+    +------------------------+

    (10) BL, (BLC)                (11) B, (BC, BLR, BCLR)       (12) BR, (BRC)
    +------------------------+    +------------------------+    +------------------------+
    |                        |    |                        |    |                        |
    |                        |    |                        |    |                        |
    |                        |    |                        |    |                        |
    |                        |    |                        |    |                        |
    |****                    |    |          ****          |    |                    ****|
    |****                    |    |          ****          |    |                    ****|
    +------------------------+    +------------------------+    +------------------------+


                     \  Lflag|     0          1          0          1
    Tflag Bflag Cflag \ Rflag|     0          0          1          1
    -------------------------|--------------------------------------------
      0     0     0          |    (1)        (1)        (3)        (2)
      1     0     0          |    (5)        (4)        (6)        (5)
      0     0     1          |    (8)        (7)        (9)        (8)
      0     1     0          |    (11)       (10)       (12)       (11)
      1     0     1          |    (5)        (4)        (6)        (5)
      0     1     1          |    (11)       (10)       (12)       (11)

### Erase terminal (-e)
#### Default
This option is disabled in default.

#### Argument
This option takes no arguments.

#### Detail
If this option is set, `cam` erases all the terminal before outputing the first image contents.

### Version (-v)
If this option is set, output version and exit with EXIT\_SUCCESS.

### Signal
`cam` tries to catch any unexpected signal.

### Standard input, pipe
#### Basic
When no file is given as the argument, `cam` tries to read stdin.
In case stdin has an image data and a file is specified as the argument, `cam` ignores stdin and tries to deal with the argument.
When `cam` choose to read from stdin, we call this `pipe mode`.
Basically:

    cam [opt] [files]
      == cat [files] | cam [opt]
    echo [string] > temp && cam temp && rm temp
      == echo [string] | cam

#### Read once rule
Once read through the input, `cam` discards that data in `pipe mode`.
Due to this rule, repeat mode (-r) is ignored in `pipe mode`.

#### Read from named pipe(FIFO)
Why do we name `pipe mode` and not `stdin mode`? Becasuse `cam` can also read from named pipe:

    mkfifo temp
    cam temp &
    cat [files] > temp
    rm temp

This allows more programmatical use of `cam`, I cannot imagine though.

#### Avoid `get` block
If nothing given for `cam`:

    cam

it shows how to use itself.
So the process should be like:
+ Check argument. If there's a file given, then tries to output the corresponding image and exit.
+ Check stdin. If there's some data given, then tries to output the corresponding image and exit.
+ Otherwise, outputs usage and exit.

## Test
`cam` must be consistent.
If the files and command args are exactly same, its output must be the same.
Also, `cam` should be portable.
However, the `stdin mode` is not portable due to the mulptiple call of `ungetc`.
I'm very sorry for this point.

## Installation

    curl -O https://raw.github.com/itchyny/cam/master/cam-0.0.0.tar.gz
    # wget --no-check-certificate https://raw.github.com/itchyny/cam/master/cam-0.0.0.tar.gz
    tar xvf ./cam-0.0.0.tar.gz
    cd ./cam-0.0.0
    ./configure
    make
    sudo make install


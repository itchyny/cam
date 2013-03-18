# cam example - clock
## Quick start

    gcc -o clock clock.c
    ./clock | cam -C

    gcc -o digitalclock digitalclock.c
    ./digitalclock | cam -C

    ./digitalclock | cam -TR -S 1

    mkfifo fifo
    ./digitalclock > fifo&
    cam -TRue -W 58 fifo&
    vi % do something
    killall digitalclock


/*
 * config.h: badbar configuration
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* Max output length of a command
 *  (modifying this is not necessary
 *   unless the output of a command
 *   will be longer than the length
 *   specified here)
 */
#define config_cmd_output_max 64

#define config_bar_x 1920
#define config_bar_y (56+900-45)
#define config_bar_w 1600
#define config_bar_h 45

#define config_background_color "rgb:20/20/1d"
#define config_foreground_color "rgb:a6/a2/8c"
#define config_font "-misc-*-*-*-*-*-10-*-*-*-*-*-*-*"
#define config_font_size 10

/* Whether or not the program should exit upon encountering an asynchronous X error */
#define config_exit_on_x_error

/* Elapsed time (in seconds) between each run of the below commands */
#define config_timeout 10

struct badbar_mouse_event {
  int button;       /* MOUSE_{LEFT, MIDDLE, RIGHT, SCROLLUP, SCROLLDOWN} */
  char *cmd;        /* Any shell command (the "event command")           */
  int draw_output;  /* Whether the output of the the above event command
                       (DRAW_EVTCMD), main command (DRAW_MAINCMD), or
                       nothing (DRAW_NOTHING) should be drawn on the
                       screen                                            */
};

struct badbar_entry {
  char *prepend;                                  /* Text to be prepended to command output */
  char *cmd;                                      /* Any shell command (the "main command") */
  char *append;                                   /* Text to be appended to command output  */
  const struct badbar_mouse_event mouse_evts[5];  /* An array of the above struct           */
  int width;                                      /* Width of this entry                    */
};

/* Event struct which signals to badbar that a given entry has no event handlers
 *  (this is required because empty array initializers are not valid C code)
 */
#define EVENT_NONE {-1, NULL, 0}

struct badbar_entry config_entries[] = {
  {
    "",
    "",
    "",
    {
      EVENT_NONE
    },
    20
  },
  {
    "",
    "date +'%m/%d/%y %I:%M %p'",
    "",
    {
      EVENT_NONE
    },
    110
  },
  {
    "CPU: ",
    "cat /proc/stat | awk '/^cpu / {div=($2/$5)*100;printf(\"%.2f\", div);}'",
    "",
    {
      EVENT_NONE
    },
    75
  },
  {
    "RAM: ",
    "free -m | awk '/^Mem/ {printf(\"%u\", 100*$3/$2);} END {print \"\"}'",
    "%",
    {
      EVENT_NONE
    },
    65
  },
  {
    "DISK: ",
    "df -h | awk '{if ($6 == \"/\") print substr($4, 0, length($4)-1)}'",
    " GB free",
    {
      EVENT_NONE
    },
    110
  },
  {
    "VOL: ",
    "if [ $(amixer sget Master | awk -F\"[][]\" '/dB/ { print $6 }') = 'on' ]; then amixer sget Master | awk -F\"[][]\" '/dB/ { print $2 }'; else echo 'MUTED'; fi",
    "",
    {
      {MOUSE_LEFT, "amixer set Master toggle -q", DRAW_MAINCMD},
      {MOUSE_SCROLLUP, "amixer set Master 1%+ -q", DRAW_MAINCMD},
      {MOUSE_SCROLLDOWN, "amixer set Master 1%- -q", DRAW_MAINCMD}
    },
    75
  }
};

#endif

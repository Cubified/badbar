/*
 * badbar.c: a bad but small bar for X
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

/*
 * PREPROCESSOR/ENUM DEFINITIONS
 */

enum STDSTREAMS {
  STDOUT = 1,
  STDERR = 2
};

enum MOUSE {
  MOUSE_NONE,
  MOUSE_LEFT,
  MOUSE_MIDDLE,
  MOUSE_RIGHT,
  MOUSE_SCROLLUP,
  MOUSE_SCROLLDOWN
};

enum DRAW {
  DRAW_NOTHING,
  DRAW_MAINCMD,
  DRAW_EVTCMD
};

#define LENGTH(x) (sizeof(x)/sizeof(x[0]))

/*
 * LOCAL INCLUDES
 */

#include "lang.h"
#include "config.h"

/*
 * GLOBAL VARIABLES
 */

int running;
Display *dpy;
Window win;
Font font;

/*
 * STATIC DEFINITIONS
 */

static void info(char *str, int len);
static void err(char *str, int len);

static void badbar_sighandler(int signo);
static int  badbar_on_x_error(Display *dpy, XErrorEvent *e);
static void badbar_button(XButtonEvent *xbutton);

static void badbar_start();
static void badbar_exec(struct badbar_entry entry, int text_x, int button);
static void badbar_events();
static void badbar_entries();
static void badbar_stop(int code);

/*
 * LOGGING
 */

/* Write to stdout */
void info(char *str, int len){
  write(STDOUT, LANG_INFO_PREPEND, LANG_INFO_PREPEND_LEN);
  write(STDOUT, str, len);
  write(STDOUT, "\n", 1);
  write(STDOUT, LANG_LOG_APPEND, LANG_LOG_APPEND_LEN);
}

/* Write to stderr */
void err(char *str, int len){
  write(STDERR, LANG_ERR_PREPEND, LANG_ERR_PREPEND_LEN);
  write(STDERR, str, len);
  write(STDERR, "\n", 1);
  write(STDERR, LANG_LOG_APPEND, LANG_LOG_APPEND_LEN);
}

/*
 * EVENT HANDLERS
 */

/* Trap Ctrl+C and similar */
void badbar_sighandler(int signo){
  badbar_stop(EXIT_CODE_SUCCESS);
}

/* Trap X errors, shutdown gracefully if configured to do so */
int badbar_on_x_error(Display *dpy, XErrorEvent *e){
  char buf[256];
  XGetErrorText(dpy, e->error_code, buf, sizeof(buf));
  err(buf, sizeof(buf));
#ifdef config_exit_on_x_error
  badbar_stop(e->error_code);
#endif

  return 0;
}

/* X mouse event */
void badbar_button(XButtonEvent *xbutton){
  int i;
  int mouse_x = xbutton->x;
  int cumulative_w = 0;

  /* TODO: Store cumulative_w to avoid unnecessary looping */
  for(i=0;i<LENGTH(config_entries);i++){
    if(mouse_x > cumulative_w &&
       mouse_x <= cumulative_w+config_entries[i].width){
      XClearArea(
        dpy,
        win,
        cumulative_w, 0,
        cumulative_w+config_entries[i].width, config_bar_h,
        False
      );
      badbar_exec(config_entries[i], cumulative_w, xbutton->button);
    }
    cumulative_w += config_entries[i].width;
  }
}

/*
 * CORE
 */

/* Init */
void badbar_start(){
  XColor bgd;
  XColor fgd;
  Atom dock_atom;

  running = 1;
  info(LANG_STARTUP, LANG_STARTUP_LEN);

  signal(SIGINT, badbar_sighandler);
  signal(SIGTERM, badbar_sighandler);
  signal(SIGQUIT, badbar_sighandler);
  XSetErrorHandler(&badbar_on_x_error);

  dpy = XOpenDisplay(NULL);
  if(dpy == NULL){
    err(LANG_ERRMSG_DISPLAY, LANG_ERRMSG_DISPLAY_LEN);
    badbar_stop(EXIT_CODE_FAILURE_DISPLAY);
  }

  XAllocNamedColor(
    dpy,
    DefaultColormap(dpy, DefaultScreen(dpy)),
    config_background_color,
    &bgd,
    &bgd
  );
  XAllocNamedColor(
    dpy,
    DefaultColormap(dpy, DefaultScreen(dpy)),
    config_foreground_color,
    &fgd,
    &fgd
  );

  win = XCreateSimpleWindow(
    dpy,
    DefaultRootWindow(dpy),
    config_bar_x, config_bar_y,
    config_bar_w, config_bar_h,
    0,
    0,
    bgd.pixel
  );
  dock_atom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", 0),
  XChangeProperty(
    dpy,
    win,
    XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", 0),
    XA_ATOM,
    32,
    PropModeReplace,
    (unsigned char*)&dock_atom,
    1
  );
  XSelectInput(dpy, win, ButtonPressMask | ButtonReleaseMask);
  XMapWindow(dpy, win);

  font = XLoadFont(dpy, config_font);
  XSetFont(dpy, DefaultGC(dpy, DefaultScreen(dpy)), font);
  XSetForeground(dpy, DefaultGC(dpy, DefaultScreen(dpy)), fgd.pixel);

  XFlush(dpy);
}

/* Generic command runner */
void badbar_exec(struct badbar_entry entry, int text_x, int button){
  int i;
  int hit;
  int draw_output;
  FILE *fp;
  char out[config_cmd_output_max];
  char buf[config_cmd_output_max];

  hit = 0;
  if(button == MOUSE_NONE){
    /* Main command should be run */
    draw_output = DRAW_MAINCMD;
    hit = 1;
    fp = popen(entry.cmd, "r");
  } else {
    /* Search for applicable event command */
    for(i=0;i<LENGTH(entry.mouse_evts);i++){
      if(entry.mouse_evts[i].button == button){
        draw_output = entry.mouse_evts[i].draw_output;
        hit = 1;
        fp = popen(entry.mouse_evts[i].cmd, "r");
      }
    }
  }
  if(!hit){
    /* No command to run */
    return;
  }
  if(fp == NULL){
    err(LANG_ERRMSG_COMMAND, LANG_ERRMSG_COMMAND_LEN);
    badbar_stop(EXIT_CODE_FAILURE_COMMAND);
  }

  if((draw_output == DRAW_EVTCMD && button != MOUSE_NONE) ||
     (draw_output == DRAW_MAINCMD && button == MOUSE_NONE)){
    /* Either the main command or the appropriate event command
     *  is being called */
    fgets(out, sizeof(out), fp);
    out[strlen(out)-1] = '\0';
    sprintf(buf, "%s%s%s", entry.prepend, out, entry.append);
    XDrawString(
      dpy,
      win,
      DefaultGC(dpy, DefaultScreen(dpy)),
      text_x, (config_bar_h+(config_font_size/2))/2,
      buf,
      strlen(buf)
    );
    pclose(fp);
  } else if(draw_output == DRAW_MAINCMD && button != MOUSE_NONE){
    /* The appropriate event command has been called, but this specific entry
     *  has been configured to rerun the main command afterwards */
    pclose(fp);
    badbar_exec(entry, text_x, MOUSE_NONE);
  }
}

/* Handle X events, automatically re-running commands if none occur */
/* https://stackoverflow.com/questions/8592292/how-to-quit-the-blocking-of-xlibs-xnextevent */
void badbar_events(){
  struct timeval tv;
  int x11_fd;
  fd_set in_fds;
  XEvent ev;

  x11_fd = ConnectionNumber(dpy);

  FD_ZERO(&in_fds);
  FD_SET(x11_fd, &in_fds);

  tv.tv_usec = 0;
  tv.tv_sec = config_timeout;

  int num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);
  if(num_ready_fds > 0){
    /* Event */
    switch(ev.type){
      case ButtonPress:
        badbar_button(&ev.xbutton);
        break;
    }
  } else if(num_ready_fds == 0){
    /* Timeout */
    badbar_entries();
  } else {
    err(LANG_ERRMSG_FD, LANG_ERRMSG_FD_LEN);
    badbar_stop(EXIT_CODE_FAILURE_FD);
  }

  while(XPending(dpy)){
    XNextEvent(dpy, &ev);
  }
}

/* Execute all entries' commands and display output */
void badbar_entries(){
  int i;
  int cumulative_w = 0;

  XClearWindow(dpy, win);

  for(i=0;i<LENGTH(config_entries);i++){
    badbar_exec(config_entries[i], cumulative_w, MOUSE_NONE);
    cumulative_w += config_entries[i].width;
  }
  
  XFlush(dpy);
}

/* Shutdown */
void badbar_stop(int code){
  if(running == 1){
    running = 0;

    if(dpy != NULL){
      XUnmapWindow(dpy, win);
      XUnloadFont(dpy, font);
      XCloseDisplay(dpy);
    }

    info(LANG_SHUTDOWN, LANG_SHUTDOWN_LEN);

    exit(code);
  }
}

int main(){
  badbar_start();

  badbar_entries();

  while(1){
    badbar_events();
  }

  badbar_stop(EXIT_CODE_SUCCESS);

  return 0;
}

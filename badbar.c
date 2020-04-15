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
XFontStruct *font;
Pixmap double_buffer;
XColor bgd;
XColor fgd;
int offset_left[LENGTH(config_entries)];

/*
 * STATIC DEFINITIONS
 */

static void info(char *str, int len);
static void err(char *str, int len);

static void badbar_sighandler(int signo);
static int  badbar_on_x_error(Display *dpy, XErrorEvent *e);
static void badbar_button(XButtonEvent *xbutton);

static void badbar_start();
static void badbar_exec(struct badbar_entry entry, int entry_index, int button);
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

  for(i=0;i<LENGTH(config_entries);i++){
    if(mouse_x > offset_left[i] &&
       mouse_x <= offset_left[i]+config_entries[i].width){
      badbar_exec(config_entries[i], i, xbutton->button);
    }
  }
}

/*
 * CORE
 */

/* Init */
void badbar_start(){
  Atom dock_atom;
  int cumulative_width;
  int i;

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

  font = XLoadQueryFont(dpy, config_font);
  XSetFont(dpy, DefaultGC(dpy, DefaultScreen(dpy)), font->fid);

  double_buffer = XCreatePixmap(
    dpy,
    win,
    config_bar_w, config_bar_h,
    DefaultDepth(dpy, DefaultScreen(dpy))
  );

  XFlush(dpy);

  memset(offset_left, 0, sizeof(offset_left));
  for(i=0,cumulative_width=0;i<LENGTH(config_entries);i++){
    offset_left[i] = cumulative_width;
    cumulative_width += config_entries[i].width;
  }
}

/* Generic command runner */
void badbar_exec(struct badbar_entry entry, int entry_index, int button){
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
    memset(out, 0, sizeof(out));
    fgets(out, sizeof(out), fp);
    out[strlen(out)-1] = '\0';
    sprintf(buf, "%s%s%s", entry.prepend, out, entry.append);

    XSetForeground(dpy, DefaultGC(dpy, DefaultScreen(dpy)), bgd.pixel);
    XFillRectangle(
      dpy,
      double_buffer,
      DefaultGC(dpy, DefaultScreen(dpy)),
      offset_left[entry_index], 0,
      offset_left[entry_index]+entry.width, config_bar_h
    );
    XSetForeground(dpy, DefaultGC(dpy, DefaultScreen(dpy)), fgd.pixel);
    XDrawString(
      dpy,
      double_buffer,
      DefaultGC(dpy, DefaultScreen(dpy)),
      offset_left[entry_index], (config_bar_h+(font->max_bounds.width/2))/2,
      buf,
      strlen(buf)
    );
    XCopyArea(
      dpy,
      double_buffer,
      win,
      DefaultGC(dpy, DefaultScreen(dpy)),
      offset_left[entry_index], 0,
      offset_left[entry_index]+entry.width, config_bar_h,
      offset_left[entry_index], 0
    );

    pclose(fp);
  } else if(draw_output == DRAW_MAINCMD && button != MOUSE_NONE){
    /* The appropriate event command has been called, but this specific entry
     *  has been configured to rerun the main command afterwards */
    pclose(fp);
    badbar_exec(entry, entry_index, MOUSE_NONE);
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

  for(i=0;i<LENGTH(config_entries);i++){
    badbar_exec(config_entries[i], i, MOUSE_NONE);
  }
  
  XFlush(dpy);
}

/* Shutdown */
void badbar_stop(int code){
  if(running == 1){
    running = 0;

    if(dpy != NULL){
      XUnmapWindow(dpy, win);
      XFreeFont(dpy, font);
      XFreePixmap(dpy, double_buffer);
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

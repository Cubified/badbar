## badbar

A bad but small bar for X.  16kb, give or take.

#### Screenshot

![badbar screenshot](https://github.com/Cubified/badbar/blob/master/screenshot.png)

#### Features

- A minimal and unobtrusive bar
- Configurable, shell command-driven entries
- X click event handlers (left, middle, and right mouse buttons + upward and downward scrolling)

#### Compiling

badbar depends upon libX11.

     $ make

Optionally (installs to ~/.local/bin):

     $ make install

#### Configuration

Refer to `config.h` for more detail, but here are a few key items:

- `config_bar_{x, y}`:  The {x, y} position of the bar on the screen
- `config_bar_{w, h}`:  The {width, height} of the bar
- `config_{back, fore}ground_color`:  An X11-styled color triple (e.g. "rgb:ab/cd/ef")
- `config_font`:  An X11 logical font description (e.g. "-nerdypepper-scientifica-medium-r-normal-\*-11-\*-\*-\*-\*-\*-\*-\*")
- `config_timeout`:  The elapsed time (in seconds) between each run of the bar's commands

Additionally, a badbar is comprised of "entries," which can be added and removed from the `config_entries` array in `config.h` and take the form of:

```c
struct badbar_entry {
  char *prepend;  /* Text to be prepended to command output   */
  char *cmd;      /* Shell command to be run                  */
  char *append;   /* Text to be appended to command output    */
  const struct badbar_mouse_event mouse_evts[5]; /* See below */
  int width;      /* Width of this specific entry             */
};
```

To interact with a bar entry, mouse event handlers can be added to the `mouse_evts` array, which take the form of:

```c
struct badbar_mouse_event {
  int button;   /* The mouse button which triggers this handler:
                    one of MOUSE_{LEFT, MIDDLE, RIGHT, SCROLLUP, SCROLLDOWN} */
  char *cmd;    /* Shell command to be run                                   */
  int draw_output;  /* See below                                             */
};
```

The `draw_output` variable describes the entry's behavior after calling a given event handler's command, with:
- `DRAW_NOTHING` causing nothing to be redrawn in the entry's space (meaning the handler's command runs but nothing changes visually)
- `DRAW_MAINCMD` causing the entry's main command (i.e. the `cmd` value of the *entry* rather than *handler*) to be rerun, with its output being drawn to the screen
- `DRAW_EVTCMD` causing the output of the event handler's command to be drawn to the screen

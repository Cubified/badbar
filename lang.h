/*
 * lang.h: strings of defined length
 */

#ifndef __LANG_H
#define __LANG_H

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

#define LANG_VER "0.1.0"

#define EXIT_CODE_SUCCESS 0
#define EXIT_CODE_FAILURE_DISPLAY 1
#define EXIT_CODE_FAILURE_COMMAND 2
#define EXIT_CODE_FAILURE_FD 3

#define LANG_INFO_PREPEND COLOR_CYAN "=> "
#define LANG_INFO_PREPEND_LEN 8

#define LANG_ERR_PREPEND COLOR_RED "==> Error: "
#define LANG_ERR_PREPEND_LEN 16

#define LANG_LOG_APPEND COLOR_RESET
#define LANG_LOG_APPEND_LEN 4

#define LANG_STARTUP "badbar v" LANG_VER " starting up."
#define LANG_STARTUP_LEN 26

#define LANG_SHUTDOWN "badbar shutting down."
#define LANG_SHUTDOWN_LEN 21

#define LANG_ERRMSG_DISPLAY "Failed to open display."
#define LANG_ERRMSG_DISPLAY_LEN 23

#define LANG_ERRMSG_COMMAND "Failed to execute command."
#define LANG_ERRMSG_COMMAND_LEN 26

#define LANG_ERRMSG_FD "Failed to open X file descriptor."
#define LANG_ERRMSG_FD_LEN 33

#endif

#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_


static const char* TERM = "dtach -A /tmp/fe-dtach-session -E";
static const char* EDITOR = "nvim";
static const char* OPEN = "open";
static const char* TRASH = "~/.Trash";

static const bool USE_MAGIC = true;

static const float COL_WIDTHS[] = { 0.8, 0.2 };

static const bool ENABLE_LOGGING = true;
static const bool PRINT_LOG_ON_SEG_VAULT = false;
static const bool FORCE_EXIT_ON_ERROR = false;

#endif

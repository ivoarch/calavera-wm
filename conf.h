/*
 * Calavera wm ☠ - window manager for X11/Linux.
 * See LICENSE file for copyright and license details.
 */

#ifndef CONF_H
#define CONF_H

/* OPTIONS */

/* Connect to a specific display */
#define DISPLAY ":0"

/* Focused/Unfocused border color */
#define UNFOCUS "Gray60"
#define FOCUS   "brown"

/* Border pixel around windows */
#define BORDER_SIZE 1

/* Snap distance */
#define SNAP 16

/* Reserved space Top/Bottom of the screen */
#define TOP_SIZE 0
#define BOTTOM_SIZE 0

/* Initial indexing windows 0= 0123456789 1= 123456789 */
#define VIEW_NUMBER_MAP 0

/* X Font cursor theme for normal and command mode
 * see http://tronche.com/gui/x/xlib/appendix/b/
 */
#define CURSOR XC_X_cursor
#define CURSOR_WAITKEY XC_icon

/* Pressing a key sends the cursor to the bottom right corner */
#define HIDE_CURSOR 0

/* Show the cursor when waiting for a key */
#define WAITKEY 1

/* Prefix keys setup default (CTRL+T) */
#define PREFIX_MODKEY ControlMask  /* modifier prefix */
#define PREFIX_KEYSYM XK_t         /* prefix key */

/* COMMANDS */
static const char *CMD_TERM[]    = { "urxvt", NULL };
static const char *CMD_BROWSER[] = { "conkeror", NULL, "Conkeror" };
static const char *CMD_EDITOR[]  = { "emacsclient", "-c", "Emacs" };
static const char *CMD_LOCK[]    = { "xlock", "-mode", "star", NULL };
static const char *CMD_SNAPSHOT[] = { "import", "screenshot.png", NULL };

/* KEY BINDINGS */
static Key keys[] = {
    /* modifier     key        function        argument */
    { None,         XK_a,      exec,           {0} },
    { None,         XK_c,      spawn,          {.v = CMD_TERM } },
    { None,         XK_e,      runorraise,     {.v = CMD_EDITOR } },
    { None,         XK_w,      runorraise,     {.v = CMD_BROWSER } },
    { None,         XK_l,      spawn,          {.v = CMD_LOCK } },
    { None,         XK_Print,  spawn,          {.v = CMD_SNAPSHOT } },
    { None,         XK_b,      banish,         {0} },
    { None,         XK_f,      fullscreen,     {0} },
    { None,         XK_m,      maximize,       {0} },
    { None,         XK_period, center,         {0} },
    { None,         XK_Tab,    switcher,       {.i = +1 } },
    { ShiftMask,    XK_Tab,    switcher,       {.i = -1 } },
    { None,         XK_k,      killfocused,    {0} },
    { None,         XK_0,      view,           {0} },
    { None,         XK_1,      view,           {1} },
    { None,         XK_2,      view,           {2} },
    { None,         XK_3,      view,           {3} },
    { None,         XK_4,      view,           {4} },
    { None,         XK_5,      view,           {5} },
    { None,         XK_6,      view,           {6} },
    { None,         XK_7,      view,           {7} },
    { None,         XK_8,      view,           {8} },
    { None,         XK_9,      view,           {9} },
    { ShiftMask,    XK_r,      reload,         {0} },
    { ShiftMask,    XK_q,      quit,           {0} },

    /* Mixer */
    {0, XF86XK_AudioLowerVolume,
        spawn, {.v = (const char*[]){"amixer", "-q", "-c", "0", "set", "Master", "5-", "unmute", NULL}}},
    {0, XF86XK_AudioRaiseVolume,
        spawn, {.v = (const char*[]){"amixer", "-q", "-c", "0", "set", "Master", "5+", "unmute", NULL}}},
    {0, XF86XK_AudioMute,
        spawn, {.v = (const char*[]){"amixer", "-q", "-c", "0", "set", "Master", "toggle", NULL}}},

    /* EMMS (The Emacs Multimedia System) */
    {0, XF86XK_AudioPlay,
        spawn, {.v = (const char*[]){"emacsclient", "-e", "(emms-toggle)", NULL}}},
    {0, XF86XK_AudioPrev,
        spawn, {.v = (const char*[]){"emacsclient", "-e", "(emms-previous)", NULL}}},
    {0, XF86XK_AudioNext,
        spawn, {.v = (const char*[]){"emacsclient", "-e", "(emms-next)", NULL}}},
};

/* MOUSE BUTTONS */
static Button buttons[] = {
    /* event mask     button      function     argument */
    { ControlMask,    Button1,    movemouse,      {0} },
    { ControlMask,    Button2,    killfocused,    {0} },
    { ControlMask,    Button3,    resizemouse,    {0} },
    { ControlMask,    Button4,    switcher,       {.i = +1 }},
    { ControlMask,    Button5,    switcher,       {.i = -1 }},
};

#endif

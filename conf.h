/* See LICENSE file for copyright and license details. */

/* appearance */
static const char font[]           = "DejaVu Sans Mono-10";
static const unsigned int snap     = 16;   /* snap pixel */
static const unsigned int borderpx = 2;    /* border pixel of floating windows */
static const int barheight         = 24;   /* the height of the bar in pixels */
static const Bool showbar          = True; /* False means no bar */
static const Bool topbar           = True; /* False means bottom bar */
static const Bool showtitle        = True; /* False means do not show title in status bar */
static const char clock_format[]   = "%a %B %d, %R"; /* Clock format */
static const unsigned int systrayspacing = 2; /* systray spacing */
static const Bool follow_mouse     = True; /* Focus the window with the mouse */
static const Bool waitkey          = True; /* Show the cursor when waiting for a key */
static const Bool guru_mode        = False;/* Completely ignore the mouse */

/* X Font cursor theme for command mode
 * see http://tronche.com/gui/x/xlib/appendix/b/
 */
#define CURSOR_WAITKEY XC_icon

/* Number of Workspaces */
#define N_WORKSPACES 4

/* key definitions */
#define PREFIX_MODKEY ControlMask /* modifier prefix */
#define PREFIX_KEYSYM XK_t  /* prefix key */

#define WS_KEY(KEY,WS) \
	{ None,                       KEY,      change_workspace,  {.ui = 1 << WS} }, \
	{ ShiftMask,                  KEY,      moveto_workspace,  {.ui = 1 << WS} },

/* default commands */
static const char *CMD_TERM[]  = { "urxvt", NULL };
static const char *CMD_BROWSER[] = { "conkeror", NULL };
static const char *CMD_EDITOR[] = { "emacsclient", "-n", "-c", "-a", "", NULL };
static const char *CMD_LOCK[] = { "xlock", "-mode", "star", NULL };

/* KEY BINDINGS */
static Key keys[] = {
	/* modifier                   key        function        argument */
        { None,                       XK_F2,     launcher,       {0} },
	{ None,                       XK_c,      spawn,          {.v = CMD_TERM } },
        { None,                       XK_e,      spawn,          {.v = CMD_EDITOR } },
	{ None,                       XK_w,      spawn,          {.v = CMD_BROWSER } },
	{ None,                       XK_l,      spawn,          {.v = CMD_LOCK } },
	{ None,                       XK_f,      fullscreen,     {0} },
	{ None,                       XK_m,      maximize,       {0} },
	{ None,                       XK_h,      horizontalmax,  {0} },
        { None,                       XK_v,      verticalmax,    {0} },
	{ None,                       XK_period, center,         {0} },
	{ None,                       XK_n,      focusstack,     {.i = +1 } },
	{ None,                       XK_p,      focusstack,     {.i = -1 } },
	{ None,                       XK_k,      killfocused,    {0} },
	WS_KEY(                        XK_1,                      0)
	WS_KEY(                        XK_2,                      1)
	WS_KEY(                        XK_3,                      2)
	WS_KEY(                        XK_4,                      3)
	{ ShiftMask,                  XK_r,      reload,         {0} },
        { ShiftMask,                  XK_q,      quit,           {0} },

        /* Multimedia keys */
	{0, XF86XK_AudioLowerVolume,
            spawn, {.v = (const char*[]){"amixer", "-q", "-c", "0", "set", "Master", "5-", "unmute", NULL}}},
        {0, XF86XK_AudioRaiseVolume,
	    spawn, {.v = (const char*[]){"amixer", "-q", "-c", "0", "set", "Master", "5+", "unmute", NULL}}},
        {0, XF86XK_AudioMute,
	    spawn, {.v = (const char*[]){"amixer", "-q", "-c", "0", "set", "Master", "toggle", NULL}}},
};

/* BUTTONS*/
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkClientWin,         ControlMask,    Button1,        movemouse,      {0} },
	{ ClkClientWin,         ControlMask,    Button3,        resizemouse,    {0} },
        { ClkClientWin,         ControlMask,    Button2,        killfocused,    {0} },
};

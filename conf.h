/* See LICENSE file for copyright and license details. */
#include <X11/XF86keysym.h>

/* appearance */
static const char font[] = "cantarell:size=11:bold:antialias=true:hinting=true";

#define NUMCOLORS 8
static const char colors[NUMCOLORS][ColLast][8] = {
    /* border       fg         bg */
    { "#eee8d5", "#eeeeee", "#202420" }, /* x01 - normal */
    { "#222222", "#eeeeee", "#386596" }, /* x02 - selected */
    { "#676767", "#dfaf8f", "#000000" }, /* x03 - urgent */
    { "#383838", "#eeeeee", "#000000" }, /* x04 - DATE TIME, Systray */
    { "#383838", "#dc322f", "#000000" }, /* x05 - red */
    { "#383838", "#859900", "#000000" }, /* x06 - green */
    { "#383838", "#b58900", "#000000" }, /* x07 - yellow */
    { "#383838", "#268bd2", "#000000" }, /* x08 - blue */
};

static const unsigned int snap  = 32;   /* snap pixel */
static const unsigned int borderpx  = 1;/* border pixel of floating windows */
static const Bool showbar       = True; /* False means no bar */
static const Bool topbar        = True; /* False means bottom bar */
static const char clock_fmt[] = "%a %I:%M %p"; /* Clock format on the bar */

#define CURSOR_WAITKEY XC_icon /* X Font cursor theme for command mode
			        * see http://tronche.com/gui/x/xlib/appendix/b/ */
static const Bool waitkey    = 1; /* 1 the cursor should change into a square when waiting for a key. */
static const Bool banishhook    = 0; /* 1 the banish command will be executed, when the prefix key is pressed */
static const Bool clicktofocus = 0; /* 1 Change focus only on click */

static const unsigned int systrayspacing = 2; /* systray spacing */
static const Bool showsystray  = True; /* False means no systray */

/* autostart script path */
#define HOME "/home/ivo"
static const char autostartscript[] = HOME"/swm/autostart.sh";

/* tagging */
#define NTAGS 4
#define Start_On_Tag 1 /* Start swm on a different tag selection */

/* rules */
#define TAG(t) (1 << (t - 1))
#define ONE   1
#define TWO   2
#define THREE 3
#define FOUR  4

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class      instance    title       tags mask     isfloating   monitor */
        { "Gimp",      NULL,       NULL,      TAG(FOUR),    True,        -1 },
	{ "Conkeror",  NULL,       NULL,      TAG(TWO),     False,       -1 },
        { "Emacs",     NULL,       NULL,      TAG(THREE),   False,       -1 },
        { "Xmessage",  NULL,       NULL,       0,           True,        -1 },
};

static const Bool resizehints = False; /* True means respect size hints in tiled resizals */

/* key definitions */
#define PREFIX_MODKEY ControlMask /* modifier prefix */
#define PREFIX_KEYSYM XK_t  /* prefix key */

#define TAGKEYS(KEY,TAG) \
	{ None,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ ControlMask,                KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ ShiftMask,                  KEY,      tag,            {.ui = 1 << TAG} }, \
	{ ControlMask | ShiftMask,    KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
#define MENU "dmenu_run"
#define PROMPT "Run Command: "
static const char *dmenucmd[] =
{
  MENU,
  "-fn", font,
  "-nb", colors[0][ColBG],
  "-nf", colors[0][ColFG],
  "-sb", colors[1][ColBG],
  "-sf", colors[1][ColFG],
  "-p",  PROMPT,
  NULL
};

static const char *termcmd[]  = { "urxvt", NULL };
static const char *conkeror[] = { "conkeror", NULL };
static const char *emacscmd[] = { "emacsclient", "-c", NULL };

/* KEY BINDINGS */
static Key keys[] = {
	/* modifier                   key        function        argument */
	{ None,                       XK_d,      spawn,          {.v = dmenucmd } },
	{ None,                       XK_c,      spawn,          {.v = termcmd } },
        { None,                       XK_e,      spawn,          {.v = emacscmd } },
	{ None,                       XK_w,      spawn,          {.v = conkeror } },
	{ ShiftMask,                  XK_b,      togglebar,      {0} },
	{ None,                       XK_b,      banish,         {0} },
	{ None,                       XK_n,      focusstack,     {.i = +1 } },
	{ None,                       XK_p,      focusstack,     {.i = -1 } },
	{ None,                       XK_Tab,    view,           {0} },
	{ None,                       XK_k,      killclient,     {0} },
	{ None,                       XK_f,      togglefloating, {0} },
	{ None,                       XK_0,      view,           {.ui = ~0 } },
	{ ShiftMask,                  XK_0,      tag,            {.ui = ~0 } },
	{ None,                       XK_comma,  focusmon,       {.i = -1 } },
	{ None,                       XK_period, focusmon,       {.i = +1 } },
	{ ShiftMask,                  XK_comma,  tagmon,         {.i = -1 } },
	{ ShiftMask,                  XK_period, tagmon,         {.i = +1 } },
	{ None,                       XK_1,      focusvisible,   {.i = 0 } },
	{ None,                       XK_2,      focusvisible,   {.i = 1 } },
	{ None,                       XK_3,      focusvisible,   {.i = 2 } },
	{ None,                       XK_4,      focusvisible,   {.i = 3 } },
        { None,                       XK_5,      focusvisible,   {.i = 4 } },
        { None,                       XK_6,      focusvisible,   {.i = 5 } },
        { None,                       XK_7,      focusvisible,   {.i = 6 } },
        { None,                       XK_8,      focusvisible,   {.i = 7 } },
        { None,                       XK_9,      focusvisible,   {.i = 8 } },
	TAGKEYS(                        XK_F1,                      0)
	TAGKEYS(                        XK_F2,                      1)
	TAGKEYS(                        XK_F3,                      2)
	TAGKEYS(                        XK_F4,                      3)
	{ ShiftMask,                  XK_q,      quit,           {0} },
};

/* button definitions */
/* click can be ClkStatusText, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkClientWin,         ControlMask,    Button1,        movemouse,      {0} },
	{ ClkClientWin,         ControlMask,    Button2,        togglefloating, {0} },
	{ ClkClientWin,         ControlMask,    Button3,        resizemouse,    {0} },
        { ClkTagBar,            0,              Button1,        view,           {0} },
        { ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
};
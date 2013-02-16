/* See LICENSE file for copyright and license details. */

/* appearance */
#include "themes/default.c"
static const char font[] = "Sans:size=11:bold:antialias=true:hinting=true";
static const unsigned int snap  = 16;   /* snap pixel */
static const unsigned int borderpx  = 2;/* border pixel of floating windows */
static const Bool showbar       = True; /* False means no bar */
static const Bool topbar        = True; /* False means bottom bar */
static const char clock_fmt[] = "%a %I:%M %p"; /* Clock format on the bar */

#define CURSOR_WAITKEY XC_icon /* X Font cursor theme for command mode
			        * see http://tronche.com/gui/x/xlib/appendix/b/ */
static const Bool waitkey    = 1; /* 1 the cursor should change into a square when waiting for a key. */
static const Bool banishhook    = 0; /* 1 the banish command will be executed, when the prefix key is pressed */

static const unsigned int systrayspacing = 2; /* systray spacing */
static const Bool showsystray  = True; /* False means no systray */

#define HOME "/home/ivo"  /* autostart script path */
static const char autostartscript[] = HOME"/swm/autostart.sh";

#define N_WORKSPACES 4         /* Number of Workspaces */
#define FLOATING_AS_DEFAULT 1; /* 0 Monocle as default */

/* key definitions */
#define PREFIX_MODKEY ControlMask /* modifier prefix */
#define PREFIX_KEYSYM XK_t  /* prefix key */

#define TK(KEY,TAG) \
	{ None,                       KEY,      usetag,           {.ui = 1 << TAG} }, \
	{ ShiftMask,                  KEY,      movetag,          {.ui = 1 << TAG} },

/* commands */
#define MENU "dmenu_run"
#define PROMPT "Run Command: "
static const char *dmenucmd[] =
{
  MENU,
  "-fn", font,
  "-nb", normbgcolor,
  "-nf", normfgcolor,
  "-sb", selbgcolor,
  "-sf", selfgcolor,
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
	{ None,                       XK_k,      killclient,     {0} },
	{ None,                       XK_f,      togglefloating, {0} },
	TK(                        XK_F1,                      0)
	TK(                        XK_F2,                      1)
	TK(                        XK_F3,                      2)
	TK(                        XK_F4,                      3)
	{ ShiftMask,                  XK_q,      quit,           {0} },
};

/* BUTTONS*/
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkClientWin,         ControlMask,    Button1,        movemouse,      {0} },
	{ ClkClientWin,         ControlMask,    Button3,        resizemouse,    {0} },
        { ClkTagBar,            0,              Button1,        usetag,         {0} },
};

/* See LICENSE file for copyright and license details. */
#include <X11/XF86keysym.h>

/* appearance */
static const char font[] = "cantarell:size=11:bold:antialias=true:hinting=true";
static const char normbordercolor[] = "#eee8d5";
static const char normbgcolor[] = "#000000";
static const char normfgcolor[] = "#eeeeee";
static const char selbordercolor[] = "#383838";
static const char selbgcolor[] = "#386596";
static const char selfgcolor[] = "#eeeeee";
static const char tags_bgcolor[] = "#202420";
static const char tags_fgcolor[] = "#eeeeee";
static const unsigned int snap  = 16;   /* snap pixel */
static const unsigned int borderpx  = 2;/* border pixel of floating windows */
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
static const char autostartscript[] = HOME"/dwm/autostart.sh";

/* tagging */
#define NTAGS 4
#define Start_On_Tag 1 /* Start swm on a different tag selection */
#define FLOATING_AS_DEFAULT 0; /* 1 floating layout as default */

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
	{ None,                       XK_Tab,    view,           {0} },
	{ None,                       XK_k,      killclient,     {0} },
	{ None,                       XK_f,      togglefloating, {0} },
	{ None,                       XK_0,      view,           {.ui = ~0 } },
	{ ShiftMask,                  XK_0,      tag,            {.ui = ~0 } },
	{ None,                       XK_comma,  focusmon,       {.i = -1 } },
	{ None,                       XK_period, focusmon,       {.i = +1 } },
	{ ShiftMask,                  XK_comma,  tagmon,         {.i = -1 } },
	{ ShiftMask,                  XK_period, tagmon,         {.i = +1 } },
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

/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance.  Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag.  Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */

/* headers */
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <fontconfig/fontconfig.h>
#include <X11/Xft/Xft.h>
#include <X11/XKBlib.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */

/* for multimedia keys, etc. */
#include <X11/XF86keysym.h>

#define WMNAME "swm"

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m) (MAX(0, MIN((x)+(w),(m)->mx+(m)->mw) - MAX((x),(m)->mx)) \
			      * MAX(0, MIN((y)+(h),(m)->my+(m)->mh) - MAX((y),(m)->my)))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << N_WORKSPACES) - 1)
#define TEXTW(X)                (textnw(X, strlen(X)) + dc.font.height)
#define RESIZE_MASK             (CWX|CWY|CWWidth|CWHeight|CWBorderWidth)

/* systray  */
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY      0
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_FOCUS_IN             4
#define XEMBED_MODALITY_ON         10

#define XEMBED_MAPPED              (1 << 0)
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_WINDOW_DEACTIVATE    2

#define VERSION_MAJOR               0
#define VERSION_MINOR               0
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR

/* enums */
enum { PrefixKey, CmdKey };                              /* prefix key */
enum { CurNormal, CurResize, CurMove, CurCmd, CurLast }; /* cursor */
enum { ColBorder, ColFG, ColBG, ColLast };               /* color */
enum { ClkTagBar, ClkClientWin, ClkRootWin, ClkLast };   /* clicks */

/* EWMH atoms */
enum {
    NetActiveWindow,
    NetSupported,
    NetSupportingCheck,
    NetSystemTray,
    NetSystemTrayOP,
    NetSystemTrayOrientation,
    NetWMName,
    NetWMPid,
    NetWMState,
    NetWMFullscreen,
    NetWMWindowType,
    NetWMWindowTypeDialog,
    NetWMWindowTypeDesktop,
    NetClientList,
    NetClientListStacking,
    NetWMDesktop,
    NetNumberOfDesktops,
    NetCurrentDesktop,
    Utf8String,
    NetLast
};

/* Xembed atoms */
enum {
    Manager,
    Xembed,
    XembedInfo,
    XLast
};

/* default atoms */
enum {
    WMProtocols,
    WMDelete,
    WMState,
    WMTakeFocus,
    WMLast
};

typedef union {
    int i;
    unsigned int ui;
    float f;
    const void *v;
} Arg; /* argument structure by conf.h */

typedef struct {
    unsigned int click;
    unsigned int mask;
    unsigned int button;
    void (*func)(const Arg *arg);
    const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
    char name[256];
    float mina, maxa;
    int x, y, w, h;  /* current position and size */
    int sfx, sfy, sfw, sfh; /* stored float geometry, used on mode revert */
    int oldx, oldy, oldw, oldh;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int bw, oldbw;
    unsigned int tags;
    Bool isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen, needresize;
    Client *next;
    Client *snext;
    Monitor *mon;
    Window win; /* The window */
};

typedef struct {
    int x, y, w, h;
    Drawable drawable;
    XftColor norm[ColLast];
    XftColor sel[ColLast];
    XftColor tags[ColLast];
    GC gc;
    struct {
	int ascent;
	int descent;
	int height;
	XftFont *xfont;
    } font;
} DC; /* draw context */

/* key struct */
typedef struct {
    unsigned int mod;
    KeySym keysym;
    void (*func)(const Arg *);
    const Arg arg;
} Key;

struct Monitor {
    float mfact;
    int num;
    int by;               /* bar geometry */
    int mx, my, mw, mh;   /* screen size */
    int wx, wy, ww, wh;   /* window area  */
    unsigned int seltags;
    unsigned int tagset[2];
    Bool showbar;
    Bool topbar;
    Client *clients;
    Client *sel;
    Client *stack;
    Monitor *next;
    Window barwin;
};

typedef struct {
    const char *class;
    const char *instance;
    const char *title;
    unsigned int tags;
    Bool isfloating;
    int monitor;
} Rule;

typedef struct Systray   Systray;
struct Systray {
    Window win;
    Client *icons;
};

/* function declarations */
static void applyrules(Client *c);
static Bool applysizehints(Client *c, int *x, int *y, int *w, int *h, Bool interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void attachend(Client *c);
static void attachstackend(Client *c);
static void banish(const Arg *arg);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clearurgent(Client *c);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void create_bar(void);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static void eprint(const char *errstr, ...);
static void drawbar(Monitor *m);
static void drawbars(void);
static void drawtext(const char *text, XftColor col[ColLast], Bool invert);
static void enternotify(XEvent *e);
static void ewmh_init(void);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusstack(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static XftColor getcolor(const char *colstr);
static Bool getrootptr(int *x, int *y);
static long getstate(Window w);
static unsigned int getsystraywidth();
static Bool gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grab_pointer(void);
static void grabbuttons(Client *c, Bool focused);
static void grabkeys(int keytype);
static void initfont(const char *fontstr);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static void moveto_workspace(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *);
static void propertynotify(XEvent *e);
static void reload(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void removesystrayicon(Client *i);
static void resize(Client *c, int x, int y, int w, int h, Bool interact);
static void resizebarwin(Monitor *m);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void restack(Monitor *m);
static void resizerequest(XEvent *e);
static void run(void);
static void scan(void);
static Bool sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, Bool fullscreen);
static void setnumbdesktops(void);
static void setup(void);
static void savefloat(Client *c);
static void showhide(Client *c);
static void sigchld(int unused);
static void spawn(const Arg *arg);
static Bool typedesktop(Window *w);
static int textnw(const char *text, unsigned int len);
static void togglefloating(const Arg *arg);
static void togglefullscreen(const Arg *arg);
static void unfocus(Client *c, Bool setfocus);
static void unmanage(Client *c, Bool destroyed);
static void unmapnotify(XEvent *e);
static void updateclientdesktop(Client *c);
static void updatecurrenddesktop(void);
static Bool updategeom(void);
static void updatebarpos(Monitor *m);
static void updateclientlist(void);
static void updateclientlist_stacking(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatesystray(void);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
static void updatewindowtype(Client *c);
static void updatetitle(Client *c);
static void updatewmhints(Client *c);
static void change_workspace(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static Client *wintosystrayicon(Window w);
static int xerror(Display *display, XErrorEvent *ee);
static int xerrordummy(Display *display, XErrorEvent *ee);
static int xerrorstart(Display *display, XErrorEvent *ee);

/* variables */
static char *wm_name = WMNAME;
static Systray *systray = NULL;
static unsigned long systrayorientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
static const char broken[] = "broken";
static char **cargv;
static char stext[256];
static int screen, screen_w, screen_h;  /* X display screen geometry width, height */
static int bh;      /* bar geometry */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0; /* dynamic key lock mask */
/* Events array */
static void (*handler[LASTEvent]) (XEvent *) = {
    [ButtonPress] = buttonpress,
    [ClientMessage] = clientmessage,
    [ConfigureRequest] = configurerequest,
    [ConfigureNotify] = configurenotify,
    [DestroyNotify] = destroynotify,
    [EnterNotify] = enternotify,
    [Expose] = expose,
    [FocusIn] = focusin,
    [KeyPress] = keypress,
    [MappingNotify] = mappingnotify,
    [MapRequest] = maprequest,
    [MotionNotify] = motionnotify,
    [PropertyNotify] = propertynotify,
    [ResizeRequest] = resizerequest,
    [UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast];
static Bool running = True;
static Cursor cursor[CurLast];
static Display *display; /* The connection to the X server. */
static DC dc;
static Monitor *mons = NULL, *selmon = NULL;
static Window root;

/* configuration, allows nested code to access above variables */
#include "conf.h"

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[N_WORKSPACES > 31 ? -1 : 1]; };

/* function implementations */
void applyrules(Client *c) {
    XClassHint ch = { NULL, NULL };

    /* rule matching */
    c->isfloating = FLOATING_AS_DEFAULT;
    c->tags = 0;
    XGetClassHint(display, c->win, &ch);

    if(ch.res_class)
        XFree(ch.res_class);
    if(ch.res_name)
        XFree(ch.res_name);
    c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

Bool applysizehints(Client *c, int *x, int *y, int *w, int *h, Bool interact) {
    Bool baseismin;
    Monitor *m = c->mon;

    /* set minimum possible */
    *w = MAX(1, *w);
    *h = MAX(1, *h);
    if(interact) {
	if(*x > screen_w)
	    *x = screen_w - WIDTH(c);
	if(*y > screen_h)
	    *y = screen_h - HEIGHT(c);
	if(*x + *w + 2 * c->bw < 0)
	    *x = 0;
	if(*y + *h + 2 * c->bw < 0)
	    *y = 0;
    }
    else {
	if(*x >= m->wx + m->ww)
	    *x = m->wx + m->ww - WIDTH(c);
	if(*y >= m->wy + m->wh)
	    *y = m->wy + m->wh - HEIGHT(c);
	if(*x + *w + 2 * c->bw <= m->wx)
	    *x = m->wx;
	if(*y + *h + 2 * c->bw <= m->wy)
	    *y = m->wy;
    }
    if(*h < bh)
	*h = bh;
    if(*w < bh)
	*w = bh;
    if(c->isfloating) {
	/* see last two sentences in ICCCM 4.1.2.3 */
	baseismin = c->basew == c->minw && c->baseh == c->minh;
	if(!baseismin) { /* temporarily remove base dimensions */
	    *w -= c->basew;
	    *h -= c->baseh;
	}
	/* adjust for aspect limits */
	if(c->mina > 0 && c->maxa > 0) {
	    if(c->maxa < (float)*w / *h)
		*w = *h * c->maxa + 0.5;
	    else if(c->mina < (float)*h / *w)
		*h = *w * c->mina + 0.5;
	}
	if(baseismin) { /* increment calculation requires this */
	    *w -= c->basew;
	    *h -= c->baseh;
	}
	/* adjust for increment value */
	if(c->incw)
	    *w -= *w % c->incw;
	if(c->inch)
	    *h -= *h % c->inch;
	/* restore base dimensions */
	*w = MAX(*w + c->basew, c->minw);
	*h = MAX(*h + c->baseh, c->minh);
	if(c->maxw)
	    *w = MIN(*w, c->maxw);
	if(c->maxh)
	    *h = MIN(*h, c->maxh);
    }
    return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void arrange(Monitor *m) {
    if(m)
	showhide(m->stack);
    else for(m = mons; m; m = m->next)
	     showhide(m->stack);
    if(m) {
	arrangemon(m);
	restack(m);
    } else for(m = mons; m; m = m->next)
	       arrangemon(m);
}

inline void arrangemon(Monitor *m) {
    monocle(m);
}

void attachend(Client *c) {
    Client *p = c->mon->clients;

    if(p) {
	for(; p->next; p = p->next);
        p->next = c;
    }  else {
	attach(c);
    }
}

void attach(Client *c) {
    c->next = c->mon->clients;
    c->mon->clients = c;
}

void attachstackend(Client *c) {
    Client *p = c->mon->stack;

    if(p) {
	for(; p->snext; p = p->snext);
	p->snext = c;
    } else {
	attachstack(c);
    }
}

void attachstack(Client *c) {
    c->snext = c->mon->stack;
    c->mon->stack = c;
}

void banish(const Arg *arg) {
    XWarpPointer(display, None, root, 0, 0, 0, 0, screen_w, screen_h);
}


void buttonpress(XEvent *e) {
    unsigned int i, x, click;
    char buf[5];
    Arg arg = {0};
    Client *c;
    Monitor *m;
    XButtonPressedEvent *ev = &e->xbutton;

    click = ClkRootWin;
    /* focus monitor if necessary */
    if((m = wintomon(ev->window)) && m != selmon) {
        unfocus(selmon->sel, True);
        selmon = m;
        focus(NULL);
    }
    if(ev->window == selmon->barwin) {
        i = x = 0;
                do
                    x += TEXTW(buf);
                while(ev->x >= x && ++i < (N_WORKSPACES));
                if(i < (N_WORKSPACES)) {
                    click = ClkTagBar;
                    arg.ui = 1 << i;
                }
    }
    if((c = wintoclient(ev->window))) {
	focus(c);
	click = ClkClientWin;
    }
    for(i = 0; i < LENGTH(buttons); i++)
	if(click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
	   && CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
	    buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void checkotherwm(void) {
    xerrorxlib = XSetErrorHandler(xerrorstart);
    /* this causes an error if some other window manager is running */
    XSelectInput(display, DefaultRootWindow(display), SubstructureRedirectMask);
    XSync(display, False);
    XSetErrorHandler(xerror);
    XSync(display, False);
}

void cleanup(void) {
    Arg a = {.ui = ~0};
    Monitor *m;

    moveto_workspace(&a);
    for(m = mons; m; m = m->next)
	while(m->stack)
	    unmanage(m->stack, False);
    XUngrabKey(display, AnyKey, AnyModifier, root);
    XFreePixmap(display, dc.drawable);
    XFreeGC(display, dc.gc);
    XFreeCursor(display, cursor[CurNormal]);
    XFreeCursor(display, cursor[CurResize]);
    XFreeCursor(display, cursor[CurMove]);
    while(mons)
	cleanupmon(mons);
    if(showsystray) {
	XUnmapWindow(display, systray->win);
	XDestroyWindow(display, systray->win);
	free(systray);
    }
    XSync(display, False);
    XSetInputFocus(display, PointerRoot, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(display, root, netatom[NetActiveWindow]);
}

void cleanupmon(Monitor *mon) {
    Monitor *m;

    if(mon == mons)
	mons = mons->next;
    else {
	for(m = mons; m && m->next != mon; m = m->next);
	m->next = mon->next;
    }
    XUnmapWindow(display, mon->barwin);
    XDestroyWindow(display, mon->barwin);
    free(mon);
}

void clearurgent(Client *c) {
    XWMHints *wmh;

    c->isurgent = False;
    if(!(wmh = XGetWMHints(display, c->win)))
	return;
    wmh->flags &= ~XUrgencyHint;
    XSetWMHints(display, c->win, wmh);
    XFree(wmh);
}

void clientmessage(XEvent *e) {
    XWindowAttributes wa;
    XSetWindowAttributes swa;
    XClientMessageEvent *cme = &e->xclient;
    Client *c = wintoclient(cme->window);

    if(showsystray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
	/* add systray icons */
	if(cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
	    if(!(c = (Client *)calloc(1, sizeof(Client))))
                eprint("fatal: could not malloc() %u bytes\n", sizeof(Client));
	    c->win = cme->data.l[2];
	    c->mon = selmon;
	    c->next = systray->icons;
	    systray->icons = c;
	    XGetWindowAttributes(display, c->win, &wa);
	    c->x = c->oldx = c->y = c->oldy = 0;
	    c->w = c->oldw = wa.width;
	    c->h = c->oldh = wa.height;
	    c->oldbw = wa.border_width;
	    c->bw = 0;
	    c->isfloating = True;
	    /* reuse tags field as mapped status */
	    c->tags = 1;
	    updatesizehints(c);
	    updatesystrayicongeom(c, wa.width, wa.height);
	    XAddToSaveSet(display, c->win);
	    XSelectInput(display, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
	    XReparentWindow(display, c->win, systray->win, 0, 0);
	    /* use parents background pixmap */
	    swa.background_pixmap = ParentRelative;
	    XChangeWindowAttributes(display, c->win, CWBackPixmap, &swa);
	    sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
	    /* FIXME not sure if I have to send these events, too */
	    sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_FOCUS_IN, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
	    sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
	    sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_MODALITY_ON, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
	    resizebarwin(selmon);
	    updatesystray();
	    setclientstate(c, NormalState);
	}
	return;
    }

    if(!c)
	return;
    if(cme->message_type == netatom[NetWMState]) {
	if(cme->data.l[1] == netatom[NetWMFullscreen] || cme->data.l[2] == netatom[NetWMFullscreen])
	    setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
			      || (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
    }
    else if(cme->message_type == netatom[NetActiveWindow]) {
	if(!ISVISIBLE(c)) {
	    c->mon->seltags ^= 1;
	    c->mon->tagset[c->mon->seltags] = c->tags;
	}
	pop(c);
    }
}

void configure(Client *c) {
    XConfigureEvent ce;

    ce.type = ConfigureNotify;
    ce.display = display;
    ce.event = c->win;
    ce.window = c->win;
    ce.x = c->x;
    ce.y = c->y;
    ce.width = c->w;
    ce.height = c->h;
    ce.border_width = c->bw;
    ce.above = None;
    ce.override_redirect = False;
    XSendEvent(display, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void configurenotify(XEvent *e) {
    Monitor *m;
    XConfigureEvent *ev = &e->xconfigure;
    Bool dirty;

    // TODO: updategeom handling sucks, needs to be simplified
    if(ev->window == root) {
	dirty = (screen_w != ev->width || screen_h != ev->height);
	screen_w = ev->width;
	screen_h = ev->height;
	if(updategeom() || dirty) {
	    if(dc.drawable != 0)
		XFreePixmap(display, dc.drawable);
	    dc.drawable = XCreatePixmap(display, root, screen_w, bh, DefaultDepth(display, screen));
	    create_bar();
	    for(m = mons; m; m = m->next)
		resizebarwin(m);
	    focus(NULL);
	    arrange(NULL);
	}
    }
}

void configurerequest(XEvent *e) {
    Client *c;
    Monitor *m;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    if((c = wintoclient(ev->window))) {
	if(ev->value_mask & CWBorderWidth)
	    c->bw = ev->border_width;
	else if(c->isfloating) {
	    m = c->mon;
	    if(ev->value_mask & CWX) {
		c->oldx = c->x;
		c->x = m->mx + ev->x;
	    }
	    if(ev->value_mask & CWY) {
		c->oldy = c->y;
		c->y = m->my + ev->y;
	    }
	    if(ev->value_mask & CWWidth) {
		c->oldw = c->w;
		c->w = ev->width;
	    }
	    if(ev->value_mask & CWHeight) {
		c->oldh = c->h;
		c->h = ev->height;
	    }
	    if((c->x + c->w) > m->mx + m->mw && c->isfloating)
		c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
	    if((c->y + c->h) > m->my + m->mh && c->isfloating)
		c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
	    if((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
		configure(c);
	    if(ISVISIBLE(c))
		XMoveResizeWindow(display, c->win, c->x, c->y, c->w, c->h);
	    else
		c->needresize = True;
	}
	else
	    configure(c);
    }
    else {
	wc.x = ev->x;
	wc.y = ev->y;
	wc.width = ev->width;
	wc.height = ev->height;
	wc.border_width = ev->border_width;
	wc.sibling = ev->above;
	wc.stack_mode = ev->detail;
	XConfigureWindow(display, ev->window, ev->value_mask, &wc);
    }
    XSync(display, False);
}

Monitor *createmon(void) {
    Monitor *m;

    if(!(m = (Monitor *)calloc(1, sizeof(Monitor))))
	eprint("fatal: could not malloc() %u bytes\n", sizeof(Monitor));

    m->tagset[0] = m->tagset[1] = 1;
    m->showbar = showbar;
    m->topbar = topbar;

    return m;
}

void destroynotify(XEvent *e) {
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if((c = wintoclient(ev->window)))
	unmanage(c, True);
    else if((c = wintosystrayicon(ev->window))) {
	removesystrayicon(c);
	resizebarwin(selmon);
	updatesystray();
    }
}

void detach(Client *c) {
    Client **tc;

    for(tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
    *tc = c->next;
}

void detachstack(Client *c) {
    Client **tc, *t;

    for(tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
    *tc = c->snext;

    if(c == c->mon->sel) {
	for(t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
	c->mon->sel = t;
    }
}

void eprint(const char *errstr, ...) {
    va_list ap;

    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void drawbar(Monitor *m) {
    int x;
    unsigned int i, occ = 0, urg = 0;
    XftColor *col;
    Client *c;

    resizebarwin(m);
    for(c = m->clients; c; c = c->next) {
	occ |= c->tags;
	if(c->isurgent)
	    urg |= c->tags;
    }
    dc.x = 0;

    /* init count workspaces */
    for(i = 0; i < N_WORKSPACES; i++) {
	char buf[5];
	int n = 0;
	for(c = m->clients; c; c = c->next) {
	    if(c->tags & 1 << i)
		n++;
	}
	snprintf(buf, 5, "%d", n);
	dc.w = TEXTW(buf);

        col = m->tagset[m->seltags] & 1 << i ? dc.sel : dc.tags;
        drawtext(buf, col, urg & 1 << i);
	dc.x += dc.w;
        XSetForeground(display, dc.gc, dc.sel[ColBorder].pixel);
        XDrawRectangle(display, dc.drawable ,dc.gc, 0, 0, dc.x-1, bh-1);
    }
    x = dc.x;
    if(m == selmon) { /* status is only drawn on selected monitor */
        dc.w = TEXTW(stext);
	dc.x = m->ww - dc.w;
	if(showsystray && m == selmon) {
	    dc.x -= getsystraywidth();
	}
	if(dc.x < x) {
	    dc.x = x;
	    dc.w = m->ww - x;
	}
        drawtext(stext, dc.norm, False);
    }
    else
	dc.x = m->ww;
    if((dc.w = dc.x - x) > bh) {
	char buf[20];
	time_t t;
	struct tm *tm;
	int len;

	dc.x = x;
	time(&t);
	tm = localtime(&t);
	strftime(buf, 20, clock_fmt, tm);
	len = TEXTW(buf);
        drawtext(NULL, dc.norm, False);
       	dc.w = MIN(dc.w, len);
	dc.x = MAX(dc.x, (m->mw / 2) - (len / 2));
	drawtext(buf, dc.norm, False);

	dc.x = x;
        /* draw client title */
	if(m->sel && showtitle) {
	drawtext(m->sel->name, dc.norm, False);
    }
    XCopyArea(display, dc.drawable, m->barwin, dc.gc, 0, 0, m->ww, bh, 0, 0);
    XSync(display, False);
    }
}

void drawbars(void) {
    Monitor *m;

    for(m = mons; m; m = m->next)
	drawbar(m);
    updatesystray();
}

void drawtext(const char *text, XftColor col[ColLast], Bool invert) {
    char buf[256];
    int i, x, y, h, len, olen;
    XftDraw *d;

    XSetForeground(display, dc.gc, col[invert ? ColFG : ColBG].pixel);
    XFillRectangle(display, dc.drawable, dc.gc, dc.x, dc.y, dc.w, dc.h);
    if(!text)
	return;
    olen = strlen(text);
    h = dc.font.ascent + dc.font.descent;
    y = dc.y + (dc.h / 2) - (h / 2) + dc.font.ascent;
    x = dc.x + (h / 2);
    /* shorten text if necessary */
    for(len = MIN(olen, sizeof buf); len && textnw(text, len) > dc.w - h; len--);
    if(!len)
	return;
    memcpy(buf, text, len);
    if(len < olen)
	for(i = len; i && i > len - 3; buf[--i] = '.');

    d = XftDrawCreate(display, dc.drawable, DefaultVisual(display, screen), DefaultColormap(display,screen));

    XftDrawStringUtf8(d, &col[invert ? ColBG : ColFG], dc.font.xfont, x, y, (XftChar8 *) buf, len);
	XftDrawDestroy(d);
}

void enternotify(XEvent *e) {
    Client *c;
    Monitor *m;
    XCrossingEvent *ev = &e->xcrossing;

    //    if((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
	return;
    c = wintoclient(ev->window);
    m = c ? c->mon : wintomon(ev->window);
    if(m != selmon) {
	unfocus(selmon->sel, True);
	selmon = m;
    }
    else if(!c || c == selmon->sel)
	return;
    focus(c);
}

void ewmh_init(void) {
    XSetWindowAttributes wa;
    Window win;

    /* init atoms */
    wmatom[WMProtocols] = XInternAtom(display, "WM_PROTOCOLS", False);
    wmatom[WMDelete] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    wmatom[WMState] = XInternAtom(display, "WM_STATE", False);
    wmatom[WMTakeFocus] = XInternAtom(display, "WM_TAKE_FOCUS", False);
    netatom[NetActiveWindow] = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    netatom[NetSupported] = XInternAtom(display, "_NET_SUPPORTED", False);
    netatom[NetSupportingCheck] = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
    netatom[NetSystemTray] = XInternAtom(display, "_NET_SYSTEM_TRAY_S0", False);
    netatom[NetSystemTrayOP] = XInternAtom(display, "_NET_SYSTEM_TRAY_OPCODE", False);
    netatom[NetSystemTrayOrientation] = XInternAtom(display, "_NET_SYSTEM_TRAY_ORIENTATION", False);
    netatom[NetWMName] = XInternAtom(display, "_NET_WM_NAME", False);
    netatom[NetWMPid] = XInternAtom(display, "_NET_WM_PID", False);
    netatom[NetWMState] = XInternAtom(display, "_NET_WM_STATE", False);
    netatom[NetWMFullscreen] = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    netatom[NetWMWindowType] = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    netatom[NetWMWindowTypeDialog] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    netatom[NetWMWindowTypeDesktop] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    netatom[NetClientList] = XInternAtom(display, "_NET_CLIENT_LIST", False);
    netatom[NetClientListStacking] = XInternAtom(display, "_NET_CLIENT_LIST_STACKING", False);
    netatom[NetNumberOfDesktops] = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
    netatom[NetCurrentDesktop] = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
    netatom[NetWMDesktop] = XInternAtom(display, "_NET_WM_DESKTOP", False);
    netatom[Utf8String] = XInternAtom(display, "UTF8_STRING", False);
    xatom[Manager] = XInternAtom(display, "MANAGER", False);
    xatom[Xembed] = XInternAtom(display, "_XEMBED", False);
    xatom[XembedInfo] = XInternAtom(display, "_XEMBED_INFO", False);

    /* Tell which ewmh atoms are supported */
    XChangeProperty(display, root, netatom[NetSupported], XA_ATOM, 32,
		    PropModeReplace, (unsigned char *) netatom, NetLast);

    /* Create our own window! */
    wa.override_redirect = True;
    win = XCreateWindow(display, root, -100, 0, 1, 1,
			0, DefaultDepth(display, screen), CopyFromParent,
			DefaultVisual(display, screen), CWOverrideRedirect, &wa);

    XChangeProperty(display, root, netatom[NetSupportingCheck], XA_WINDOW, 32,
		    PropModeReplace, (unsigned char*)&win, 1);

    /* Set WM name */
    XChangeProperty(display, win, netatom[NetWMName], netatom[Utf8String], 8,
		    PropModeReplace, (unsigned char*)wm_name, strlen(wm_name));

    /* Set WM pid */
    int pid = getpid();
    XChangeProperty(display, root, netatom[NetWMPid], XA_CARDINAL, 32,
    		    PropModeReplace, (unsigned char*)&pid, 1);
}

void expose(XEvent *e) {
    Monitor *m;
    XExposeEvent *ev = &e->xexpose;

    if(ev->count == 0 && (m = wintomon(ev->window)))
	drawbar(m);
}

void focus(Client *c) {
    if(!c || !ISVISIBLE(c))
	for(c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
    /* was if(selmon->sel) */
    if(selmon->sel && selmon->sel != c)
	unfocus(selmon->sel, False);
    if(c) {
	if(c->mon != selmon)
	    selmon = c->mon;
	if(c->isurgent)
	    clearurgent(c);
	detachstack(c);
	attachstack(c);
	grabbuttons(c, True);
        XSetWindowBorder(display, c->win, dc.sel[ColBorder].pixel);
	setfocus(c);
    }
    else {
	XSetInputFocus(display, root, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(display, root, netatom[NetActiveWindow]);
    }
    selmon->sel = c;
    drawbars();
}

void focusin(XEvent *e) { /* there are some broken focus acquiring clients */
    XFocusChangeEvent *ev = &e->xfocus;

    if(selmon->sel && ev->window != selmon->sel->win)
	setfocus(selmon->sel);
}

void focusstack(const Arg *arg) {
    Client *c = NULL, *i;

    if(!selmon->sel)
	return;
    if(arg->i > 0) { /* next */
	for(c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
	if(!c)
	    for(c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
    }
    else { /* prev */
	for(i = selmon->clients; i != selmon->sel; i = i->next)
	    if(ISVISIBLE(i))
		c = i;
	if(!c)
	    for(; i; i = i->next)
		if(ISVISIBLE(i))
		    c = i;
    }
    if(c) {
	focus(c);
	restack(selmon);
    }
}

Atom getatomprop(Client *c, Atom prop) {
    int di;
    unsigned long dl;
    unsigned char *p = NULL;
    Atom da, atom = None;
    /* FIXME getatomprop should return the number of items and a pointer to
        * the stored data instead of this workaround */
    Atom req = XA_ATOM;
    if(prop == xatom[XembedInfo])
	req = xatom[XembedInfo];

    if(XGetWindowProperty(display, c->win, prop, 0L, sizeof atom, False, req,
			  &da, &di, &dl, &dl, &p) == Success && p) {
	atom = *(Atom *)p;
	if(da == xatom[XembedInfo] && dl == 2)
	    atom = ((Atom *)p)[1];
	XFree(p);
    }
    return atom;
}

XftColor getcolor(const char *colstr) {
    XftColor color;

    if(!XftColorAllocName(display, DefaultVisual(display, screen), DefaultColormap(display, screen), colstr, &color))
	eprint("error, cannot allocate color '%s'\n", colstr);

    return color;
}

Bool getrootptr(int *x, int *y) {
    int di;
    unsigned int dui;
    Window dummy;

    return XQueryPointer(display, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long getstate(Window w) {
    int format;
    long result = -1;
    unsigned char *p = NULL;
    unsigned long n, extra;
    Atom real;

    if(XGetWindowProperty(display, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
			  &real, &format, &n, &extra, (unsigned char **)&p) != Success)
	return -1;
    if(n != 0)
	result = *p;
    XFree(p);
    return result;
}

unsigned int getsystraywidth() {
    unsigned int w = 0;
    Client *i;

    if(showsystray)
	for(i = systray->icons; i; w += i->w + systrayspacing, i = i->next) ;
    return w ? w + systrayspacing : 1;
}

Bool gettextprop(Window w, Atom atom, char *text, unsigned int size) {
    char **list = NULL;
    int n;
    XTextProperty name;

    if(!text || size == 0)
	return False;
    text[0] = '\0';
    XGetTextProperty(display, w, &name, atom);
    if(!name.nitems)
	return False;
    if(name.encoding == XA_STRING)
	strncpy(text, (char *)name.value, size - 1);
    else {
	if(XmbTextPropertyToTextList(display, &name, &list, &n) >= Success && n > 0 && *list) {
	    strncpy(text, *list, size - 1);
	    XFreeStringList(list);
	}
    }
    text[size - 1] = '\0';
    XFree(name.value);
    return True;
}

void grab_pointer() {
    XGrabPointer (display, root, True, 0,
		  GrabModeAsync, GrabModeAsync,
		  None, cursor[CurCmd], CurrentTime);
}

void grabbuttons(Client *c, Bool focused) {
    updatenumlockmask();
    {
	unsigned int i, j;
	unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
	XUngrabButton(display, AnyButton, AnyModifier, c->win);
	if(focused) {
	    for(i = 0; i < LENGTH(buttons); i++)
		if(buttons[i].click == ClkClientWin)
		    for(j = 0; j < LENGTH(modifiers); j++)
			XGrabButton(display, buttons[i].button,
				    buttons[i].mask | modifiers[j],
				    c->win, False, BUTTONMASK,
				    GrabModeAsync, GrabModeSync, None, None);
	}
	else
	    XGrabButton(display, AnyButton, AnyModifier, c->win, False,
			BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
    }
}

void grabkeys(int keytype) {
    unsigned int i;
    unsigned int modifiers[] = {
        0,
        LockMask,
        numlockmask,
	LockMask | numlockmask
    };
    KeyCode code;

    if(keytype == CmdKey) {
	XGrabKey(display, AnyKey, AnyModifier, root, True, GrabModeAsync,
		 GrabModeAsync);

        if (waitkey) {
	    grab_pointer();
	}
    }
    else {
	XUngrabKey(display, AnyKey, AnyModifier, root);

        if(banishhook) {
	    XWarpPointer(display, None, root, 0, 0, 0, 0, screen_w, screen_h);
        }

        if((code = XKeysymToKeycode(display, PREFIX_KEYSYM)))
	    for(i = 0; i < LENGTH(modifiers); i++)
		XGrabKey(display, code, PREFIX_MODKEY | modifiers[i],
			 root, True, GrabModeAsync,
			 GrabModeAsync);

	XUngrabPointer(display, CurrentTime);
    }
}

void initfont(const char *fontstr) {
    if(!(dc.font.xfont = XftFontOpenName(display,screen,fontstr))
       && !(dc.font.xfont = XftFontOpenName(display,screen,"fixed")))
        eprint("error, cannot load font: '%s'\n", fontstr);

    dc.font.ascent = dc.font.xfont->ascent;
    dc.font.descent = dc.font.xfont->descent;
    dc.font.height = dc.font.ascent + dc.font.descent;
}

#ifdef XINERAMA
static Bool isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info) {
    while(n--)
	if(unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
	   && unique[n].width == info->width && unique[n].height == info->height)
	    return False;
    return True;
}
#endif /* XINERAMA */

void keypress(XEvent *e) {
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev;
    static int prefixset = 0;

    ev = &e->xkey;
    keysym = XkbKeycodeToKeysym(display, (KeyCode)e->xkey.keycode, 0, 0);

    if(!prefixset && keysym == PREFIX_KEYSYM
       && CLEANMASK(ev->state) == PREFIX_MODKEY) {
	prefixset = 1;
	grabkeys(CmdKey);
    }
    else {
	for(i = 0; i < LENGTH(keys); i++)
	    if(keysym == keys[i].keysym
	       && CLEANMASK(ev->state) == keys[i].mod && keys[i].func)
		keys[i].func(&(keys[i].arg));

	prefixset = 0;
	grabkeys(PrefixKey);
    }
}

void killclient(const Arg *arg) {
    if(!selmon->sel)
	return;
    if(!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0 , 0, 0)) {
	XGrabServer(display);
	XSetErrorHandler(xerrordummy);
	XSetCloseDownMode(display, DestroyAll);
	XKillClient(display, selmon->sel->win);
	XSync(display, False);
	XSetErrorHandler(xerror);
	XUngrabServer(display);
    }
}

/* manage the new client */
void manage(Window w, XWindowAttributes *wa) {
    Client *c, *t = NULL;
    Window trans = None;
    XWindowChanges wc;

    if (typedesktop(&w)) {
	return;
    }

    if(!(c = calloc(1, sizeof(Client))))
	eprint("fatal: could not malloc() %u bytes\n", sizeof(Client));
    c->win = w;
    updatetitle(c);
    if(XGetTransientForHint(display, w, &trans) && (t = wintoclient(trans))) {
	c->mon = t->mon;
	c->tags = t->tags;
    }
    else {
	c->mon = selmon;
	applyrules(c);
    }
    /* geometry */
    c->x = c->oldx = wa->x;
    c->y = c->oldy = wa->y;
    c->w = c->oldw = wa->width;
    c->h = c->oldh = wa->height;
    c->oldbw = wa->border_width;

    if(c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
	c->x = c->mon->mx + c->mon->mw - WIDTH(c);
    if(c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
	c->y = c->mon->my + c->mon->mh - HEIGHT(c);
    c->x = MAX(c->x, c->mon->mx);
    /* only fix client y-offset, if the client center might cover the bar */
    c->y = MAX(c->y, ((c->mon->by == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx)
		      && (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? bh : c->mon->my);
    c->bw = borderpx;

    wc.border_width = c->bw;
    XConfigureWindow(display, w, CWBorderWidth, &wc);
    XSetWindowBorder(display, w, dc.norm[ColBorder].pixel);
    configure(c); /* propagates border_width, if size doesn't change */
    updatewindowtype(c);
    updatesizehints(c);
    updatewmhints(c);
    savefloat(c);
    XSelectInput(display, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
    grabbuttons(c, False);
    if(!c->isfloating)
	c->isfloating = c->oldstate = trans != None || c->isfixed;
    if(c->isfloating)
	XRaiseWindow(display, c->win);
    attachend(c);
    attachstackend(c);
    focus(c);
    XChangeProperty(display, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		    (unsigned char *) &(c->win), 1);
    XChangeProperty(display, root, netatom[NetClientListStacking], XA_WINDOW, 32, PropModeAppend,
		    (unsigned char *) &(c->win), 1);
    XMoveResizeWindow(display, c->win, c->x + 2 * screen_w, c->y, c->w, c->h); /* some windows require this */
    setclientstate(c, NormalState);
    if (c->mon == selmon)
	unfocus(selmon->sel, False);
    c->mon->sel = c;
    arrange(c->mon);
    XMapWindow(display, c->win); /* maps the window */
    focus(NULL);
    /* set clients tag as current desktop (_NET_WM_DESKTOP) */
    updateclientdesktop(c);
}

/* regrab when keyboard map changes */
void mappingnotify(XEvent *e) {
    XMappingEvent *ev = &e->xmapping;

    XRefreshKeyboardMapping(ev);
    if(ev->request == MappingKeyboard) {
	updatenumlockmask();
	grabkeys(PrefixKey);
    }
}

void maprequest(XEvent *e) {
    static XWindowAttributes wa;
    XMapRequestEvent *ev = &e->xmaprequest;
    Client *i;
    if((i = wintosystrayicon(ev->window))) {
	sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
	resizebarwin(selmon);
	updatesystray();
    }

    if(!XGetWindowAttributes(display, ev->window, &wa))
	return;
    if(wa.override_redirect)
	return;
    if(!wintoclient(ev->window))
	manage(ev->window, &wa);
}

void monocle(Monitor *m) {
    Client *c;

    for(c = nexttiled(m->clients); c; c = nexttiled(c->next)) {
	/* Don't draw borders. */
	c->bw = 0;
	resize(c, m->wx, m->wy, m->ww, m->wh, False);
	/* Restore borders. */
	c->bw = borderpx;
    }
}

void motionnotify(XEvent *e) {
    static Monitor *mon = NULL;
    Monitor *m;
    XMotionEvent *ev = &e->xmotion;

    if(ev->window != root)
	return;
    if((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
	unfocus(selmon->sel, True);
	selmon = m;
	focus(NULL);
    }
    mon = m;
}

void movemouse(const Arg *arg) {
    int x, y, ocx, ocy, nx, ny;
    Client *c;
    Monitor *m;
    XEvent ev;

    if(!(c = selmon->sel))
	return;
    if(c->isfullscreen) /* no support moving fullscreen windows by mouse */
	return;
    restack(selmon);
    ocx = c->x;
    ocy = c->y;
    XWarpPointer(display, None, c->win, 0, 0, 0, 0, c->w / 2, c->h / 2);
    if(XGrabPointer(display, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		    None, cursor[CurMove], CurrentTime) != GrabSuccess)
	return;
    if(!getrootptr(&x, &y))
	return;
    do {
	XMaskEvent(display, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
	switch(ev.type) {
	case ConfigureRequest:
	case Expose:
	case MapRequest:
	    handler[ev.type](&ev);
	    break;
	case MotionNotify:
	    nx = ocx + (ev.xmotion.x - x);
	    ny = ocy + (ev.xmotion.y - y);
	    if(nx >= selmon->wx && nx <= selmon->wx + selmon->ww
	       && ny >= selmon->wy && ny <= selmon->wy + selmon->wh) {
		if(abs(selmon->wx - nx) < snap)
		    nx = selmon->wx;
		else if(abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
		    nx = selmon->wx + selmon->ww - WIDTH(c);
		if(abs(selmon->wy - ny) < snap)
		    ny = selmon->wy;
		else if(abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
		    ny = selmon->wy + selmon->wh - HEIGHT(c);
                if(!c->isfloating && (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
		    togglefloating(NULL);
	    }
            if(c->isfloating)
		resize(c, nx, ny, c->w, c->h, True);
	    break;
	}
    } while(ev.type != ButtonRelease);
    XUngrabPointer(display, CurrentTime);
    if((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
	sendmon(c, m);
	selmon = m;
	focus(NULL);
    }
}

Client *nexttiled(Client *c) {
    for(; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
    return c;
}

void pop(Client *c) {
    detach(c);
    attach(c);
    focus(c);
    arrange(c->mon);
}

void propertynotify(XEvent *e) {
    Client *c;
    Window trans;
    XPropertyEvent *ev = &e->xproperty;

    if((c = wintosystrayicon(ev->window))) {
	if(ev->atom == XA_WM_NORMAL_HINTS) {
	    updatesizehints(c);
	    updatesystrayicongeom(c, c->w, c->h);
	}
	else
	    updatesystrayiconstate(c, ev);
	resizebarwin(selmon);
	updatesystray();
    }
    if((ev->window == root) && (ev->atom == XA_WM_NAME))
	updatestatus();
    else if(ev->state == PropertyDelete)
	return; /* ignore */
    else if((c = wintoclient(ev->window))) {
	switch(ev->atom) {
	default: break;
	case XA_WM_TRANSIENT_FOR:
	    if(!c->isfloating && (XGetTransientForHint(display, c->win, &trans)) &&
	       (c->isfloating = (wintoclient(trans)) != NULL))
		arrange(c->mon);
	    break;
	case XA_WM_NORMAL_HINTS:
	    updatesizehints(c);
	    break;
	case XA_WM_HINTS:
	    updatewmhints(c);
	    drawbars();
	    break;
	}
	if(ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
	    updatetitle(c);
	    if(c == c->mon->sel)
		drawbar(c->mon);
	}
	if(ev->atom == netatom[NetWMWindowType])
	    updatewindowtype(c);
    }
}

void reload(const Arg *arg) {
    running = False;
    if (arg) {
	cleanup();
	execvp(cargv[0], cargv);
	eprint("Can't exec: %s\n", strerror(errno));
    }
}

Monitor *recttomon(int x, int y, int w, int h) {
    Monitor *m, *r = selmon;
    int a, area = 0;

    for(m = mons; m; m = m->next)
	if((a = INTERSECT(x, y, w, h, m)) > area) {
	    area = a;
	    r = m;
	}
    return r;
}

void removesystrayicon(Client *i) {
    Client **ii;

    if(!showsystray || !i)
	return;
    for(ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
    if(ii)
	*ii = i->next;
    free(i);
}

void resize(Client *c, int x, int y, int w, int h, Bool interact) {
    if(applysizehints(c, &x, &y, &w, &h, interact))
	resizeclient(c, x, y, w, h);
}

void resizebarwin(Monitor *m) {
    unsigned int w = m->ww;

    if(showsystray && m == selmon)
	w -= getsystraywidth();
    XMoveResizeWindow(display, m->barwin, m->wx, m->by, w, bh);
}

void resizeclient(Client *c, int x, int y, int w, int h) {
    XWindowChanges wc;

    c->oldx = c->x; c->x = wc.x = x;
    c->oldy = c->y; c->y = wc.y = y;
    c->oldw = c->w; c->w = wc.width = w;
    c->oldh = c->h; c->h = wc.height = h;
    wc.border_width = c->bw;
    XConfigureWindow(display, c->win, RESIZE_MASK, &wc);
    configure(c);
    XSync(display, False);
}

void resizerequest(XEvent *e) {
    XResizeRequestEvent *ev = &e->xresizerequest;
    Client *i;

    if((i = wintosystrayicon(ev->window))) {
	updatesystrayicongeom(i, ev->width, ev->height);
	resizebarwin(selmon);
	updatesystray();
    }
}

void resizemouse(const Arg *arg) {
    int ocx, ocy;
    int nw, nh;
    Client *c;
    Monitor *m;
    XEvent ev;

    if(!(c = selmon->sel))
	return;
    if(c->isfullscreen) /* no support resizing fullscreen windows by mouse */
	return;
    restack(selmon);
    ocx = c->x;
    ocy = c->y;
    if(XGrabPointer(display, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		    None, cursor[CurResize], CurrentTime) != GrabSuccess)
	return;
    XWarpPointer(display, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
    do {
	XMaskEvent(display, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
	switch(ev.type) {
	case ConfigureRequest:
	case Expose:
	case MapRequest:
	    handler[ev.type](&ev);
	    break;
	case MotionNotify:
	    nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
	    nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
	    if(c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
	       && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
		{
		    if(!c->isfloating && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
			togglefloating(NULL);
		}
	    if(c->isfloating)
		resize(c, c->x, c->y, nw, nh, True);
	    break;
	}
    } while(ev.type != ButtonRelease);
    XWarpPointer(display, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
    XUngrabPointer(display, CurrentTime);
    while(XCheckMaskEvent(display, EnterWindowMask, &ev));
    if((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
	sendmon(c, m);
	selmon = m;
	focus(NULL);
    }
}

/* restores all clients */
void restack(Monitor *m) {
    XEvent ev;

    drawbar(m);
    if(!m->sel)
	return;
    XRaiseWindow(display, m->sel->win);
    XSync(display, False);
    while(XCheckMaskEvent(display, EnterWindowMask, &ev));
}

void run(void) {
    XEvent ev;

    /* execute autostart script */
    if (access(autostartscript, F_OK) == 0) {
	if (system(autostartscript) == -1) {
	    printf("There was an error while executing autostart script: %s\n",
		   autostartscript);
	}
    }

    /* main event loop */
    XSync(display, False);
    while(running && !XNextEvent(display, &ev))
	if(handler[ev.type])
	    handler[ev.type](&ev); /* call handler */
}

void scan(void) {
    unsigned int i, num;
    Window d1, d2, *wins = NULL;
    XWindowAttributes wa;

    if(XQueryTree(display, root, &d1, &d2, &wins, &num)) {
	for(i = 0; i < num; i++) {
	    if(!XGetWindowAttributes(display, wins[i], &wa)
	       || wa.override_redirect || XGetTransientForHint(display, wins[i], &d1))
		continue;
	    if(wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
		manage(wins[i], &wa);
	}
	for(i = 0; i < num; i++) { /* now the transients */
	    if(!XGetWindowAttributes(display, wins[i], &wa))
		continue;
	    if(XGetTransientForHint(display, wins[i], &d1)
	       && (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
		manage(wins[i], &wa);
	}
	if(wins)
	    XFree(wins);
    }
}

void sendmon(Client *c, Monitor *m) {
    if(c->mon == m)
	return;
    unfocus(c, True);
    detach(c);
    detachstack(c);
    c->mon = m;
    c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
    updateclientdesktop(c);
    attach(c);
    attachstack(c);
    focus(NULL);
    arrange(NULL);
}

void setclientstate(Client *c, long state) {
    long data[] = { state, None };

    XChangeProperty(display, c->win, wmatom[WMState], wmatom[WMState], 32,
		    PropModeReplace, (unsigned char *)data, 2);
}

Bool sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4) {
    int n;
    Atom *protocols, mt;
    Bool exists = False;
    XEvent ev;

    if(proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
	mt = wmatom[WMProtocols];
	if(XGetWMProtocols(display, w, &protocols, &n)) {
            while(!exists && n--)
		exists = protocols[n] == proto;
	    XFree(protocols);
	}
    }
    else {
	exists = True;
	mt = proto;
    }
    if(exists) {
	ev.type = ClientMessage;
	ev.xclient.window = w;
	ev.xclient.message_type = mt;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = d0;
	ev.xclient.data.l[1] = d1;
	ev.xclient.data.l[2] = d2;
	ev.xclient.data.l[3] = d3;
	ev.xclient.data.l[4] = d4;
	XSendEvent(display, w, False, mask, &ev);
    }
    return exists;
}

void setfocus(Client *c) {
    if(!c->neverfocus) {
	XSetInputFocus(display, c->win, RevertToPointerRoot, CurrentTime);
	XChangeProperty(display, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
    }
    sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
}

void setfullscreen(Client *c, Bool fullscreen) {
    if(fullscreen) {
	XChangeProperty(display, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
	c->isfullscreen = True;
	c->oldstate = c->isfloating;
	c->oldbw = c->bw;
	c->bw = 0;
	c->isfloating = True;
	resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
	XRaiseWindow(display, c->win);
    }
    else {
	XChangeProperty(display, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
	c->isfullscreen = False;
	c->isfloating = c->oldstate;
	c->bw = c->oldbw;
	c->x = c->oldx;
	c->y = c->oldy;
	c->w = c->oldw;
	c->h = c->oldh;
	resizeclient(c, c->x, c->y, c->w, c->h);
	arrange(c->mon);
    }
}

void setcurrentdesktop(void) {
    long data[] = { 0 };

    XChangeProperty(display, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32,
		    PropModeReplace, (unsigned char *)data, 1);
}

void setnumbdesktops(void) {
    long data[] = { TAGMASK };

    XChangeProperty(display, root, netatom[NetNumberOfDesktops], XA_CARDINAL, 32,
		    PropModeReplace, (unsigned char *)data, 1);
}

void setup(void) {
    XSetWindowAttributes wa;

    /* clean up any zombies immediately */
    sigchld(0);

    /* init screen */
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);
    initfont(font); /* init font */
    screen_w = DisplayWidth(display, screen);
    screen_h = DisplayHeight(display, screen);
    bh = dc.h = dc.font.height + 2;
    if(bh < barheight) bh = dc.h = barheight;
    updategeom();

    /* Standard & EWMH atoms */
    ewmh_init();

    /* init cursors */
    cursor[CurNormal] = XCreateFontCursor(display, XC_left_ptr);
    cursor[CurResize] = XCreateFontCursor(display, XC_bottom_right_corner);
    cursor[CurMove] = XCreateFontCursor(display, XC_fleur);
    cursor[CurCmd] = XCreateFontCursor(display, CURSOR_WAITKEY);
    /* init appearance */
    dc.norm[ColBorder] = getcolor(normbordercolor);
    dc.norm[ColBG] = getcolor(normbgcolor);
    dc.norm[ColFG] = getcolor(normfgcolor);
    dc.sel[ColBorder] = getcolor(selbordercolor);
    dc.sel[ColBG] = getcolor(selbgcolor);
    dc.sel[ColFG] = getcolor(selfgcolor);
    dc.tags[ColBG] = getcolor(workspaces_bgcolor);
    dc.tags[ColFG] = getcolor(workspaces_fgcolor);
    dc.drawable = XCreatePixmap(display, root, DisplayWidth(display, screen), bh, DefaultDepth(display, screen));
    dc.gc = XCreateGC(display, root, 0, NULL);
    /* set line drawing attributes */
    XSetLineAttributes(display, dc.gc, 1, LineSolid, CapButt, JoinMiter);
    /* init system tray */
    updatesystray();
    /* init bars */
    create_bar();
    updatestatus();
    XDeleteProperty(display, root, netatom[NetClientList]);
    XDeleteProperty(display, root, netatom[NetClientListStacking]);
    /* set EWMH NUMBER_OF_DESKTOPS */
    setnumbdesktops();
    /* initialize EWMH CURRENT_DESKTOP */
    setcurrentdesktop();
    /* select for events */
    wa.cursor = cursor[CurNormal];
    wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask|PointerMotionMask
	|EnterWindowMask|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
    XChangeWindowAttributes(display, root, CWEventMask|CWCursor, &wa);
    XSelectInput(display, root, wa.event_mask);
    updatenumlockmask();
    grabkeys(PrefixKey);
}

/* restore floats dimensions */
void savefloat(Client *c) {
    c->sfx = c->x;
    c->sfy = c->y;
    c->sfw = c->w;
    c->sfh = c->h;
}

void showhide(Client *c) {
    if(!c)
	return;
    if(ISVISIBLE(c)) { /* show clients top down */
	if(c->needresize) {
	    c->needresize = False;
	    XMoveResizeWindow(display, c->win, c->x, c->y, c->w, c->h);
	} else {
	    XMoveWindow(display, c->win, c->x, c->y);
	}
	if(c->isfloating && !c->isfullscreen)
	    resize(c, c->x, c->y, c->w, c->h, False);
	showhide(c->snext);
    }
    else { /* hide clients bottom up */
	showhide(c->snext);
	XMoveWindow(display, c->win, WIDTH(c) * -2, c->y);
    }
}

void sigchld(int unused) {
    if(signal(SIGCHLD, sigchld) == SIG_ERR)
	eprint("Can't install SIGCHLD handler");
    while(0 < waitpid(-1, NULL, WNOHANG));
}

void spawn(const Arg *arg) {
    if(fork() == 0) {
	if(display)
	    close(ConnectionNumber(display));
	setsid();
	execvp(((char **)arg->v)[0], (char **)arg->v);
	fprintf(stderr, "swm: execvp %s", ((char **)arg->v)[0]);
	perror(" failed");
	exit(EXIT_SUCCESS);
    }
}

Bool typedesktop(Window *w) {
    int f;
    unsigned char *data = NULL;
    unsigned long n, extra, i;
    Atom real, result = None;

    if(XGetWindowProperty(display, *w, netatom[NetWMWindowType], 0L, 0x7FFFFFFF, False, AnyPropertyType,
			  &real, &f, &n, &extra, &data) == Success) {

	for(i = 0; i < n; ++i)
	result = * (Atom *) data;

	XMapWindow(display, *w);
	XMapSubwindows(display, *w);
    }

    XFree(data);
    return result == netatom[NetWMWindowTypeDesktop] ? True : False;
}

void moveto_workspace(const Arg *arg) {
    if(selmon->sel && arg->ui & TAGMASK) {
	selmon->sel->tags = arg->ui & TAGMASK;
        updateclientdesktop(selmon->sel);
	focus(NULL);
	arrange(selmon);
    }
    updatecurrenddesktop();
}

int textnw(const char *text, unsigned int len) {

    XGlyphInfo ext;
    XftTextExtentsUtf8(display, dc.font.xfont, (XftChar8 *) text, len, &ext);
    return ext.xOff;
}

void togglefloating(const Arg *arg) {
    if(!selmon->sel)
	return;
    if(selmon->sel->isfullscreen) /* no support for fullscreen windows */
	return;
    selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
    if(selmon->sel->isfloating)
	/*restore last known float dimensions*/
	resize(selmon->sel, selmon->sel->sfx, selmon->sel->sfy,
	       selmon->sel->sfw, selmon->sel->sfh, False);
    else
	/* save last known float dimensions */
    savefloat(selmon->sel);
    arrange(selmon);
}

void togglefullscreen(const Arg *arg) {

    if(!selmon->sel)
	return;
    setfullscreen(selmon->sel, !selmon->sel->isfullscreen);
}

void unfocus(Client *c, Bool setfocus) {
    if(!c)
	return;
    grabbuttons(c, False);
    XSetWindowBorder(display, c->win, dc.norm[ColBorder].pixel);
    if(setfocus) {
	XSetInputFocus(display, root, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(display, root, netatom[NetActiveWindow]);
    }
}

/* destroy the client */
void unmanage(Client *c, Bool destroyed) {
    Monitor *m = c->mon;
    XWindowChanges wc;

    /* The server grab construct avoids race conditions. */
    detach(c);
    detachstack(c);
    if(!destroyed) {
	wc.border_width = c->oldbw;
	XGrabServer(display);
	XSetErrorHandler(xerrordummy);
	XConfigureWindow(display, c->win, CWBorderWidth, &wc); /* restore border */
	XUngrabButton(display, AnyButton, AnyModifier, c->win);
	setclientstate(c, WithdrawnState);
	XSync(display, False);
	XSetErrorHandler(xerror);
	XUngrabServer(display);
    }
    free(c);
    focus(NULL);
    updateclientlist();
    updateclientlist_stacking();
    arrange(m);
}

void unmapnotify(XEvent *e) {
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if((c = wintoclient(ev->window))) {
	if(ev->send_event)
	    setclientstate(c, WithdrawnState);
	else
	    unmanage(c, False);
    }
    else if((c = wintosystrayicon(ev->window))) {
	removesystrayicon(c);
	resizebarwin(selmon);
	updatesystray();
    }
}

void create_bar(void) {
    unsigned int w;
    Monitor *m;
    XSetWindowAttributes wa = {
	.override_redirect = True,
	.background_pixmap = ParentRelative,
	.event_mask = ButtonPressMask|ExposureMask
    };
    for(m = mons; m; m = m->next) {
	if (m->barwin)
	    continue;
	w = m->ww;
	if(showsystray && m == selmon)
	    w -= getsystraywidth();
	m->barwin = XCreateWindow(display, root, m->wx, m->by, w, bh, 0, DefaultDepth(display, screen),
				  CopyFromParent, DefaultVisual(display, screen),
				  CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
	XDefineCursor(display, m->barwin, cursor[CurNormal]);
	XMapRaised(display, m->barwin);
    }
}

void updatebarpos(Monitor *m) {
    m->wy = m->my;
    m->wh = m->mh;
    if(m->showbar) {
	m->wh -= bh;
	m->by = m->topbar ? m->wy : m->wy + m->wh;
	m->wy = m->topbar ? m->wy + bh : m->wy;
    }
    else
	m->by = -bh;
}

void updateclientlist() {
    Client *c;
    Monitor *m;

    XDeleteProperty(display, root, netatom[NetClientList]);
    for(m = mons; m; m = m->next)
	for(c = m->clients; c; c = c->next)
	    XChangeProperty(display, root, netatom[NetClientList],
			    XA_WINDOW, 32, PropModeAppend,
			    (unsigned char *) &(c->win), 1);
}

void updateclientlist_stacking() {
    Client *c;
    Monitor *m;

    XDeleteProperty(display, root, netatom[NetClientListStacking]);
    for(m = mons; m; m = m->next)
        for(c = m->clients; c; c = c->snext)
            XChangeProperty(display, root, netatom[NetClientListStacking],
                            XA_WINDOW, 32, PropModeAppend,
                            (unsigned char *) &(c->win), 1);
}

void updateclientdesktop(Client *c) {
     long data[] = { c->tags };

     XChangeProperty(display, c->win, netatom[NetWMDesktop], XA_CARDINAL, 32,
		     PropModeReplace, (unsigned char *)data, 1);
}

void updatecurrenddesktop() {
    long data[] = { selmon->tagset[selmon->seltags] };

    XChangeProperty(display, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32,
		    PropModeReplace, (unsigned char *)data, 1);
}

Bool updategeom(void) {
    Bool dirty = False;

#ifdef XINERAMA
    if(XineramaIsActive(display)) {
	int i, j, n, nn;
	Client *c;
	Monitor *m;
	XineramaScreenInfo *info = XineramaQueryScreens(display, &nn);
	XineramaScreenInfo *unique = NULL;

	for(n = 0, m = mons; m; m = m->next, n++);
	/* only consider unique geometries as separate screens */
	if(!(unique = (XineramaScreenInfo *)malloc(sizeof(XineramaScreenInfo) * nn)))
	    eprint("fatal: could not malloc() %u bytes\n", sizeof(XineramaScreenInfo) * nn);
	for(i = 0, j = 0; i < nn; i++)
	    if(isuniquegeom(unique, j, &info[i]))
		memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
	XFree(info);
	nn = j;
	if(n <= nn) {
	    for(i = 0; i < (nn - n); i++) { /* new monitors available */
		for(m = mons; m && m->next; m = m->next);
		if(m)
		    m->next = createmon();
		else
		    mons = createmon();
	    }
	    for(i = 0, m = mons; i < nn && m; m = m->next, i++)
		if(i >= n
		   || (unique[i].x_org != m->mx || unique[i].y_org != m->my
		       || unique[i].width != m->mw || unique[i].height != m->mh))
		    {
			dirty = True;
			m->num = i;
			m->mx = m->wx = unique[i].x_org;
			m->my = m->wy = unique[i].y_org;
			m->mw = m->ww = unique[i].width;
			m->mh = m->wh = unique[i].height;
			updatebarpos(m);
		    }
	}
	else { /* less monitors available nn < n */
	    for(i = nn; i < n; i++) {
		for(m = mons; m && m->next; m = m->next);
		while(m->clients) {
		    dirty = True;
		    c = m->clients;
		    m->clients = c->next;
		    detachstack(c);
		    c->mon = mons;
		    attach(c);
		    attachstack(c);
		}
		if(m == selmon)
		    selmon = mons;
		cleanupmon(m);
	    }
	}
	free(unique);
    }
    else
#endif /* XINERAMA */
        /* default monitor setup */
        {
	    if(!mons)
		mons = createmon();
	    if(mons->mw != screen_w || mons->mh != screen_h) {
		dirty = True;
		mons->mw = mons->ww = screen_w;
		mons->mh = mons->wh = screen_h;
		updatebarpos(mons);
	    }
        }
    if(dirty) {
	selmon = mons;
	selmon = wintomon(root);
    }
    return dirty;
}

void updatenumlockmask(void) {
    unsigned int i, j;
    XModifierKeymap *modmap;

    numlockmask = 0;
    modmap = XGetModifierMapping(display);
    for(i = 0; i < 8; i++)
	for(j = 0; j < modmap->max_keypermod; j++)
	    if(modmap->modifiermap[i * modmap->max_keypermod + j]
	       == XKeysymToKeycode(display, XK_Num_Lock))
		numlockmask = (1 << i);
    XFreeModifiermap(modmap);
}

void updatesizehints(Client *c) {
    long msize;
    XSizeHints size;

    if(!XGetWMNormalHints(display, c->win, &size, &msize))
	/* size is uninitialized, ensure that size.flags aren't used */
	size.flags = PSize;
    if(size.flags & PBaseSize) {
	c->basew = size.base_width;
	c->baseh = size.base_height;
    }
    else if(size.flags & PMinSize) {
	c->basew = size.min_width;
	c->baseh = size.min_height;
    }
    else
	c->basew = c->baseh = 0;
    if(size.flags & PResizeInc) {
	c->incw = size.width_inc;
	c->inch = size.height_inc;
    }
    else
	c->incw = c->inch = 0;
    if(size.flags & PMaxSize) {
	c->maxw = size.max_width;
	c->maxh = size.max_height;
    }
    else
	c->maxw = c->maxh = 0;
    if(size.flags & PMinSize) {
	c->minw = size.min_width;
	c->minh = size.min_height;
    }
    else if(size.flags & PBaseSize) {
	c->minw = size.base_width;
	c->minh = size.base_height;
    }
    else
	c->minw = c->minh = 0;
    if(size.flags & PAspect) {
	c->mina = (float)size.min_aspect.y / size.min_aspect.x;
	c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
    }
    else
	c->maxa = c->mina = 0.0;
    c->isfixed = (c->maxw && c->minw && c->maxh && c->minh
		  && c->maxw == c->minw && c->maxh == c->minh);
}

void updatetitle(Client *c) {
    if(!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
	gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
    if(c->name[0] == '\0') /* hack to mark broken clients */
	strcpy(c->name, broken);
}

void updatestatus(void) {
    char *username;
    username = getlogin();

    if(!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
        strcpy(stext, username);
    drawbar(selmon);
}

void updatesystrayicongeom(Client *i, int w, int h) {
    if(i) {
	i->h = bh;
	if(w == h)
	    i->w = bh;
	else if(h == bh)
	    i->w = w;
	else
	    i->w = (int) ((float)bh * ((float)w / (float)h));
	applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
	/* force icons into the systray dimenons if they don't want to */
	if(i->h > bh) {
	    if(i->w == i->h)
		i->w = bh;
	    else
		i->w = (int) ((float)bh * ((float)i->w / (float)i->h));
	    i->h = bh;
	}
        XMoveResizeWindow(display, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
    }
}

void updatesystrayiconstate(Client *i, XPropertyEvent *ev) {
    long flags;
    int code = 0;

    if(!showsystray || !i || ev->atom != xatom[XembedInfo] ||
       !(flags = getatomprop(i, xatom[XembedInfo])))
	return;

    if(flags & XEMBED_MAPPED && !i->tags) {
	i->tags = 1;
	code = XEMBED_WINDOW_ACTIVATE;
	XMapRaised(display, i->win);
	setclientstate(i, NormalState);
    }
    else if(!(flags & XEMBED_MAPPED) && i->tags) {
	i->tags = 0;
	code = XEMBED_WINDOW_DEACTIVATE;
	XUnmapWindow(display, i->win);
	setclientstate(i, WithdrawnState);
    }
    else
	return;
    sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
	      systray->win, XEMBED_EMBEDDED_VERSION);
}

void updatesystray(void) {
    XSetWindowAttributes wa;
    Client *i;
    unsigned int x = selmon->mx + selmon->mw;
    unsigned int w = 1;

    if(!showsystray)
	return;
    if(!systray) {
	/* init systray */
	if(!(systray = (Systray *)calloc(1, sizeof(Systray))))
	    eprint("fatal: could not malloc() %u bytes\n", sizeof(Systray));
        systray->win = XCreateSimpleWindow(display, root, x, selmon->by, w, bh, 0, 0, dc.norm[ColBG].pixel);
	wa.event_mask = ButtonPressMask | ExposureMask;
	wa.override_redirect = True;
        wa.background_pixmap = ParentRelative;
        wa.background_pixel = dc.norm[ColBG].pixel;
	XSelectInput(display, systray->win, SubstructureNotifyMask);
	XChangeProperty(display, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)&systrayorientation, 1);
	XChangeWindowAttributes(display, systray->win, CWEventMask | CWOverrideRedirect | CWBackPixel, &wa);
	XMapRaised(display, systray->win);
	XSetSelectionOwner(display, netatom[NetSystemTray], systray->win, CurrentTime);
	if(XGetSelectionOwner(display, netatom[NetSystemTray]) == systray->win) {
	    sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
	    XSync(display, False);
	}
	else {
	    fprintf(stderr, "swm: unable to obtain system tray.\n");
	    free(systray);
	    systray = NULL;
	    return;
	}
    }
    for(w = 0, i = systray->icons; i; i = i->next) {
        XMapRaised(display, i->win);
        w += systrayspacing;
        XMoveResizeWindow(display, i->win, (i->x = w), 0, i->w, i->h);
        w += i->w;
	if(i->mon != selmon)
	    i->mon = selmon;
    }
    w = w ? w + systrayspacing : 1;
    x -= w;
    XMoveResizeWindow(display, systray->win, x, selmon->by, w, bh);
    XSync(display, False);
}

void updatewindowtype(Client *c) {
    Atom state = getatomprop(c, netatom[NetWMState]);
    Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

    if(state == netatom[NetWMFullscreen])
	setfullscreen(c, True);
    if(wtype == netatom[NetWMWindowTypeDialog])
	c->isfloating = True;
}

Client *wintosystrayicon(Window w) {
    Client *i = NULL;

    if(!showsystray || !w)
	return i;
    for(i = systray->icons; i && i->win != w; i = i->next) ;
    return i;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.  */

void updatewmhints(Client *c) {
    XWMHints *wmh;

    if((wmh = XGetWMHints(display, c->win))) {
	if(c == selmon->sel && wmh->flags & XUrgencyHint) {
	    wmh->flags &= ~XUrgencyHint;
	    XSetWMHints(display, c->win, wmh);
	}
	else
	    c->isurgent = (wmh->flags & XUrgencyHint) ? True : False;
	if(wmh->flags & InputHint)
	    c->neverfocus = !wmh->input;
	else
	    c->neverfocus = False;
	XFree(wmh);
    }
}

void change_workspace(const Arg *arg) {

    if((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
	return;
    selmon->seltags ^= 1; /* toggle sel tagset */
    if(arg->ui & TAGMASK)
	selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
    focus(NULL);
    arrange(selmon);
    updatecurrenddesktop();
}

Client *wintoclient(Window w) {
    Client *c;
    Monitor *m;

    for(m = mons; m; m = m->next)
	for(c = m->clients; c; c = c->next)
	    if(c->win == w)
		return c;
    return NULL;
}

Monitor *wintomon(Window w) {
    int x, y;
    Client *c;
    Monitor *m;

    if(w == root && getrootptr(&x, &y))
	return recttomon(x, y, 1, 1);
    for(m = mons; m; m = m->next)
	if(w == m->barwin)
	    return m;
    if((c = wintoclient(w)))
	return c->mon;
    return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.  */
int xerror(Display *display, XErrorEvent *ee) {
    if(ee->error_code == BadWindow
       || (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
       || (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
       || (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
       || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
       || (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
       || (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
       || (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
       || (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
	return 0;
    fprintf(stderr, "swm: fatal error: request code=%d, error code=%d\n",
	    ee->request_code, ee->error_code);
    return xerrorxlib(display, ee); /* may call exit */
}

int xerrordummy(Display *display, XErrorEvent *ee) {
    return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int xerrorstart(Display *display, XErrorEvent *ee) {
    eprint("swm: another window manager is already running\n");
    return -1;
}

int main(int argc, char *argv[]) {
    if(argc == 2 && !strcmp("-v", argv[1]))
	eprint("swm-"VERSION", © 2006-2012 dwm engineers, see LICENSE for details\nCustomized and patched by Ivaylo Kuzev.\n");
    else if(argc != 1)
	eprint("usage: swm [-v]\n");
    if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
	fputs("warning: no locale support\n", stderr);
    if(!(display = XOpenDisplay(NULL)))
	eprint("swm: cannot open display\n");
    cargv = argv;
    checkotherwm();
    setup();
    scan();
    run();
    cleanup();
    /* Close display */
    XCloseDisplay(display);
    return EXIT_SUCCESS;
}

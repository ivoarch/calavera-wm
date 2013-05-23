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
#include <limits.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */

/* for multimedia keys, etc. */
#include <X11/XF86keysym.h>

/* windows manager name */
#define WMNAME "calavera-wm"

#define BUFSIZE 256

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

//#define MIX_WS 0 
//#define MAX_WS 8
#define N_WORKSPACES 10
#define TAGMASK                 ((1 << N_WORKSPACES) - 1)

#define RESIZE_MASK             (CWX|CWY|CWWidth|CWHeight|CWBorderWidth)
#define EVENT_MASK              (EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask)
#define ROOT                    RootWindow(display, DefaultScreen(display))

/* enums */
enum { PrefixKey, CmdKey };                              /* prefix key */
enum { CurNormal, CurResize, CurMove, CurCmd, CurLast }; /* cursor */
enum { ClkClientWin, ClkRootWin, ClkLast };              /* clicks */

/* EWMH atoms */
enum {
    NetActiveWindow,
    NetClientList,
    NetClientListStacking,
    NetCurrentDesktop,
    NetNumberOfDesktops,
    NetSupported,
    NetSupportingCheck,
    NetWMDesktop,
    NetWMName,
    NetWMState,
    NetWMFullscreen,
    NetWMWindowType,
    NetWMWindowTypeNotification,
    NetWMWindowTypeSplash,
    NetWMWindowTypeDock,
    NetWMWindowTypeDialog,
    NetLast
};

/* default atoms */
enum {
    WMProtocols,
    WMDelete,
    WMState,
    WMTakeFocus,
    WMLast,
    Utf8String,
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
    char name[BUFSIZE];
    float mina, maxa;
    int x, y, w, h;  /* current position and size */
    int oldx, oldy, oldw, oldh;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int bw, oldbw;
    unsigned int tags;
    Bool isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen, needresize; // isdock;
    Client *next;
    Client *snext;
    Monitor *mon;
    Window win; /* The window */
};

/* key struct */
typedef struct {
    unsigned int mod;
    KeySym keysym;
    void (*func)(const Arg *);
    const Arg arg;
} Key;

struct Monitor {
    int num;
    int mx, my, mw, mh;   /* screen size */
    int wx, wy, ww, wh;   /* window area  */
    unsigned int seltags;
    unsigned int tagset[2];
    Client *clients;
    Client *sel;
    Client *stack;
    Monitor *next;
};

typedef struct {
    const char *class;
    unsigned int tags;
    Bool isfloating;
    int monitor;
} Rule;

/* DATA */

// atoms - ewmh
static void ewmh_init(void);
static long ewmh_getstate(Window w);
static void ewmh_setclientstate(Client *c, long state);
static Bool sendevent(Client *c, Atom proto);
static void ewmh_setnumbdesktops(void);
static void ewmh_updatecurrenddesktop(void);
static void ewmh_updateclientdesktop(Client *c);
static void ewmh_updateclientlist(void);
static void ewmh_updateclientlist_stacking(void);
static void ewmh_updatewindowtype(Client *c);

// bar
static void set_padding(Monitor *m);

// colors
static unsigned long getcolor(const char *colstr);

// clients
static Bool applysizehints(Client *c, int *x, int *y, int *w, int *h, Bool interact);
static void attach(Client *c);
static void attachstack(Client *c);
static void attachend(Client *c);
static void attachstackend(Client *c);
static void clearurgent(Client *c);
static void configure(Client *c);
static void detach(Client *c);
static void detachstack(Client *c);
static void focus(Client *c);
static void killclient(Client *c);
static void grabbuttons(Client *c, Bool focused);
static void pop(Client *);
static void resize(Client *c, int x, int y, int w, int h, Bool interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void sendmon(Client *c, Monitor *m);
static void setfocus(Client *c);
static void setfullscreen(Client *c, Bool fullscreen);
static void showhide(Client *c);
static void unfocus(Client *c, Bool setfocus);
static void unmanage(Client *c, Bool destroyed);
static void updatesizehints(Client *c);
static void updatewmhints(Client *c);
static Client *wintoclient(Window w);

// events
static void buttonpress(XEvent *e);
static void clientmessage(XEvent *e);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void focusin(XEvent *e);
static void keypress(XEvent *e);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void motionnotify(XEvent *e);
static void propertynotify(XEvent *e);
static void unmapnotify(XEvent *e);

// manage
static void grabkeys(int keytype);
static void manage(Window w, XWindowAttributes *wa);
static void grab_pointer(void);
static void updatenumlockmask(void);

// main
static void autorun(void);
static void checkotherwm(void);
static Bool checkdock(Window *w);
static void cleanup(void);
static void eprint(const char *errstr, ...);
static Bool getrootptr(int *x, int *y);
static void handle_events(void);
static void scan(void);
static void setup(void);
static void sigchld(int unused);
static void sync_display(void);
static int xerror(Display *display, XErrorEvent *ee);
static int xerrordummy(Display *display, XErrorEvent *ee);
static int xerrorstart(Display *display, XErrorEvent *ee);

// monitor
static void arrange(Monitor *m);
static void cleanupmon(Monitor *mon);
static Monitor *createmon(void);
static Monitor *recttomon(int x, int y, int w, int h);
static void restack(Monitor *m);
static Bool updategeom(void);
static Monitor *wintomon(Window w);

// actions
static void banish(const Arg *arg);
static void center(const Arg *arg);
static void focusstack(const Arg *arg);
static void killfocused(const Arg *arg);
static void exec(const Arg *arg);
static void maximize(const Arg *arg);
static void horizontalmax(const Arg *arg);
static void verticalmax(const Arg *arg);
static void movemouse(const Arg *arg);
static void moveto_workspace(const Arg *arg);
static void moveresize(const Arg *arg);
static void quit(const Arg *arg);
static void reload(const Arg *arg);
static void resizemouse(const Arg *arg);
static void spawn(const Arg *arg);
static void fullscreen(const Arg *arg);
static void change_workspace(const Arg *arg);

/* variables */
static unsigned int win_focus;                                                                                                
static unsigned int win_unfocus;
static char *wm_name = WMNAME;
static char **cargv;
static int screen, screen_w, screen_h;  /* X display screen geometry width, height */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0; /* dynamic key lock mask */
/* Events array */
static void (*handler[LASTEvent]) (XEvent *) = {
    [ButtonPress] = buttonpress,
    [ClientMessage] = clientmessage,
    [ConfigureRequest] = configurerequest,
    [ConfigureNotify] = configurenotify,
    [DestroyNotify] = destroynotify,
    [FocusIn] = focusin,
    [KeyPress] = keypress,
    [MappingNotify] = mappingnotify,
    [MapRequest] = maprequest,
    [MotionNotify] = motionnotify,
    [PropertyNotify] = propertynotify,
    [UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast];
static Bool running = True;
static Cursor cursor[CurLast];
static Display *display; /* The connection to the X server. */
static Monitor *mons = NULL, *selmon = NULL;
static Window root;

/* configuration, allows nested code to access above variables */
#include "conf.h"

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[N_WORKSPACES > 31 ? -1 : 1]; };

/* function implementations */
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
    if(*h < DOCK_SIZE)
      *h = DOCK_SIZE;
    if(*w < DOCK_SIZE)
      *w = DOCK_SIZE;
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
    restack(m);
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

void autorun(){
    struct stat st;
    char path[PATH_MAX];
    char *home;

    /* execute autostart script */
    if (!(home = getenv("HOME")))
      return;

    snprintf(path, sizeof(path), "%s/calavera-wm/autostart", home);


    if (stat(path, &st) != 0)
      return;

    const char* autostartcmd[] = { path, NULL };
    Arg a = {.v = autostartcmd };

    /* Check if file is executable */
    if (S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR))
    spawn(&a);
}

void buttonpress(XEvent *e) {
    unsigned int i, click;
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
    if((c = wintoclient(ev->window))) {
	focus(c);
	click = ClkClientWin;
    }
    for(i = 0; i < LENGTH(buttons); i++)
	if(click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
	   && CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
	    buttons[i].func(&buttons[i].arg);
}

void banish(const Arg *arg) {
  XWarpPointer(display, None, root, 0, 0, 0, 0, screen_w, screen_h);
}

void center(const Arg *arg) {
    if(!selmon->sel || selmon->sel->isfullscreen || !(selmon->sel->isfloating))
	return;
    resize(selmon->sel, selmon->wx + 0.5 * (selmon->ww - selmon->sel->w), selmon->wy + 0.5 * (selmon->wh - selmon->sel->h),
	   selmon->sel->w, selmon->sel->h, False);
    arrange(selmon);
}

/* FIXME */
Bool checkdock(Window *w) {
    int format;
    unsigned char *p = NULL;
    unsigned long n, extra;
    Atom real, result = None;
  
    if(XGetWindowProperty(display, *w, netatom[NetWMWindowType], 0L, 0xffffffff, False, AnyPropertyType,
                          &real, &format, &n, &extra, &p) == Success) {
      if (n != 0)
        result = * (Atom *) p;
    }
    XFree(p);
    XMapWindow(display, *w);
    return result == netatom[NetWMWindowTypeDock]  
      || result == netatom[NetWMWindowTypeNotification]  
      || result == netatom[NetWMWindowTypeSplash]? True : False;
}

void checkotherwm(void) {
    xerrorxlib = XSetErrorHandler(xerrorstart);
    /* this causes an error if some other window manager is running */
    XSelectInput(display, DefaultRootWindow(display), SubstructureRedirectMask);
    sync_display();
    XSetErrorHandler(xerror);
    sync_display();
}

void cleanup(void) {
    Arg a = {.ui = ~0};
    Monitor *m;

    moveto_workspace(&a);
    for(m = mons; m; m = m->next)
	while(m->stack)
	    unmanage(m->stack, False);
    XUngrabKey(display, AnyKey, AnyModifier, root);
    XFreeCursor(display, cursor[CurNormal]);
    XFreeCursor(display, cursor[CurResize]);
    XFreeCursor(display, cursor[CurMove]);
    cleanupmon(mons);
}

void cleanupmon(Monitor *mon) {
    Monitor *m;

    if(mon == mons)
	mons = mons->next;
    else {
	for(m = mons; m && m->next != mon; m = m->next);
	m->next = mon->next;
    }
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
    XClientMessageEvent *cme = &e->xclient;
    Client *c = wintoclient(cme->window);

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
    XConfigureEvent *ev = &e->xconfigure;
    Bool dirty;

    // TODO: updategeom handling sucks, needs to be simplified
    if(ev->window == root) {
	dirty = (screen_w != ev->width || screen_h != ev->height);
	screen_w = ev->width;
	screen_h = ev->height;
	if(updategeom() || dirty) {
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
    sync_display();
}

Monitor *createmon(void) {
    Monitor *m;

    if(!(m = (Monitor *)calloc(1, sizeof(Monitor))))
	eprint("fatal: could not malloc() %u bytes\n", sizeof(Monitor));

    m->tagset[0] = m->tagset[1] = 1;
    return m;
}

void destroynotify(XEvent *e) {
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if((c = wintoclient(ev->window)))
	unmanage(c, True);
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

void ewmh_init(void) {
    XSetWindowAttributes wa;
    Window win;

    /* ICCCM */
    wmatom[WMProtocols] = XInternAtom(display, "WM_PROTOCOLS", False);
    wmatom[WMDelete] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    wmatom[WMState] = XInternAtom(display, "WM_STATE", False);
    wmatom[WMTakeFocus] = XInternAtom(display, "WM_TAKE_FOCUS", False);
 
    /* EWMH */
    netatom[NetActiveWindow] = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    netatom[NetSupported] = XInternAtom(display, "_NET_SUPPORTED", False);
    netatom[NetSupportingCheck] = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
    netatom[NetClientList] = XInternAtom(display, "_NET_CLIENT_LIST", False);
    netatom[NetClientListStacking] = XInternAtom(display, "_NET_CLIENT_LIST_STACKING", False);
    netatom[NetNumberOfDesktops] = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
    netatom[NetCurrentDesktop] = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);

    /* STATES */
    netatom[NetWMState] = XInternAtom(display, "_NET_WM_STATE", False);
    netatom[NetWMFullscreen] = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

    /* TYPES */
    netatom[NetWMWindowType] = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    netatom[NetWMWindowTypeNotification] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False);
    netatom[NetWMWindowTypeDock] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    netatom[NetWMWindowTypeDialog] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    netatom[NetWMWindowTypeSplash] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_SPLASH", False);
 
    /* CLIENTS */
    netatom[NetWMName] = XInternAtom(display, "_NET_WM_NAME", False);
    netatom[NetWMDesktop] = XInternAtom(display, "_NET_WM_DESKTOP", False);


    /* OTHER */
    netatom[Utf8String] = XInternAtom(display, "UTF8_STRING", False);

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
        XSetWindowBorder(display, c->win, win_focus);
	setfocus(c);
    }
    else {
	XSetInputFocus(display, root, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(display, root, netatom[NetActiveWindow]);
    }
    selmon->sel = c;
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

    if(XGetWindowProperty(display, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
			  &da, &di, &dl, &dl, &p) == Success && p) {
	atom = *(Atom *)p;
	XFree(p);
    }
    return atom;
}

unsigned long getcolor(const char *colstr) {
    Colormap cmap = DefaultColormap(display, screen);
    XColor color;

    if(!XAllocNamedColor(display, cmap, colstr, &color, &color))
	eprint("error, cannot allocate color '%s'\n", colstr);
    return color.pixel;
}

Bool getrootptr(int *x, int *y) {
    int di;
    unsigned int dui;
    Window dummy;

    return XQueryPointer(display, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long ewmh_getstate(Window w) {
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

        if (WAITKEY) {
	    grab_pointer();
	}
    }
    else {
	XUngrabKey(display, AnyKey, AnyModifier, root);

        if(HIDE_CURSOR) {
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

void handle_events(void) {
  XEvent ev;

  /* main event loop */
  XSync(display, False);
  while(running && !XNextEvent(display, &ev))
    if(handler[ev.type])
      handler[ev.type](&ev); /* call handler */
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

void killclient(Client *c) {
    if(!selmon->sel)
	return;
    if(!sendevent(selmon->sel, wmatom[WMDelete])) {
	XGrabServer(display);
	XSetErrorHandler(xerrordummy);
	XSetCloseDownMode(display, DestroyAll);
	XKillClient(display, selmon->sel->win);
	sync_display();
	XSetErrorHandler(xerror);
	XUngrabServer(display);
    }
}

void killfocused(const Arg *arg) {
    if(!selmon->sel)
	return;
    killclient(selmon->sel);
}

/* manage the new client */
void manage(Window w, XWindowAttributes *wa) {
    Client *c, *t = NULL;
    Window trans = None;
    XWindowChanges wc;
    XClassHint ch = { NULL, NULL };

    if (checkdock(&w)) {
      return;
    }

    if(!(c = calloc(1, sizeof(Client))))
	eprint("fatal: could not malloc() %u bytes\n", sizeof(Client));
    c->win = w;
    if(XGetTransientForHint(display, w, &trans) && (t = wintoclient(trans))) {
	c->mon = t->mon;
	c->tags = t->tags;
    }
    else {
      c->mon = selmon;

      /* rule matching */
      c->isfloating = 1, c->tags = 0;
      XGetClassHint(display, c->win, &ch);

      if(ch.res_class)
        XFree(ch.res_class);
      if(ch.res_name)
        XFree(ch.res_name);
      c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
        
    }
    /* geometry */
    c->x = c->oldx = wa->x;
    c->y = c->oldy = wa->y;
    c->w = c->oldw = wa->width;
    c->h = c->oldh = wa->height;
    c->oldbw = wa->border_width;

    //    c->isdock = False;

    if(c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
	c->x = c->mon->mx + c->mon->mw - WIDTH(c);
    if(c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
	c->y = c->mon->my + c->mon->mh - HEIGHT(c);
    c->x = MAX(c->x, c->mon->mx);
    /* only fix client y-offset, if the client center might cover the bar */
    c->y = MAX(c->y, ((c->x + (c->w / 2) >= c->mon->wx)
                      && (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? DOCK_SIZE : c->mon->my);
    c->bw = 1;

    wc.border_width = c->bw;
    XConfigureWindow(display, w, CWBorderWidth, &wc);
    XSetWindowBorder(display, w, win_focus);
    configure(c); /* propagates border_width, if size doesn't change */
    ewmh_updatewindowtype(c);
    updatesizehints(c);
    updatewmhints(c);
    XSelectInput(display, w, EVENT_MASK);
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
    ewmh_setclientstate(c, NormalState);
    if (c->mon == selmon)
	unfocus(selmon->sel, False);
    c->mon->sel = c;
    arrange(c->mon);
    XMapWindow(display, c->win); /* maps the window */
    focus(NULL);
    /* set clients tag as current desktop (_NET_WM_DESKTOP) */
    ewmh_updateclientdesktop(c);
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

    if(!XGetWindowAttributes(display, ev->window, &wa))
	return;
    if(wa.override_redirect)
	return;
    if(!wintoclient(ev->window))
	manage(ev->window, &wa);
}

void maximize(const Arg *arg) {
    if(!selmon->sel || selmon->sel->isfullscreen || !(selmon->sel->isfloating))
	return;
    resize(selmon->sel, selmon->wx, selmon->wy,	selmon->ww - 2 * selmon->sel->bw, selmon->wh - 2 * selmon->sel->bw, False);
    arrange(selmon);
}

void horizontalmax(const Arg *arg) {
    if(!selmon->sel || selmon->sel->isfullscreen || !(selmon->sel->isfloating))
	return;
    resize(selmon->sel, selmon->wx, selmon->sel->y, selmon->ww - 2 * selmon->sel->bw, selmon->sel->h, False);
    arrange(selmon);
}

void verticalmax(const Arg *arg) {
    if(!selmon->sel || selmon->sel->isfullscreen || !(selmon->sel->isfloating))
	return;
    resize(selmon->sel, selmon->sel->x, selmon->wy, selmon->sel->w, selmon->wh - 2 * selmon->sel->bw, False);
    arrange(selmon);
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
		if(abs(selmon->wx - nx) < SNAP)
		    nx = selmon->wx;
		else if(abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < SNAP)
		    nx = selmon->wx + selmon->ww - WIDTH(c);
		if(abs(selmon->wy - ny) < SNAP)
		    ny = selmon->wy;
		else if(abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < SNAP)
		    ny = selmon->wy + selmon->wh - HEIGHT(c);
                if(!c->isfloating && (abs(nx - c->x) > SNAP || abs(ny - c->y) > SNAP));
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

    if(ev->state == PropertyDelete)
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
	    break;
	}
	if(ev->atom == netatom[NetWMWindowType])
	    ewmh_updatewindowtype(c);
    }
}

void quit(const Arg *arg) {
    running = False;
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

void resize(Client *c, int x, int y, int w, int h, Bool interact) {
    if(applysizehints(c, &x, &y, &w, &h, interact))
	resizeclient(c, x, y, w, h);
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
    sync_display();
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
		    if(!c->isfloating && (abs(nw - c->w) > SNAP || abs(nh - c->h) > SNAP));
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

    if(!m->sel)
	return;
    XRaiseWindow(display, m->sel->win);
    sync_display();
    while(XCheckMaskEvent(display, EnterWindowMask, &ev));
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
	    if(wa.map_state == IsViewable || ewmh_getstate(wins[i]) == IconicState)
		manage(wins[i], &wa);
	}
	for(i = 0; i < num; i++) { /* now the transients */
	    if(!XGetWindowAttributes(display, wins[i], &wa))
		continue;
	    if(XGetTransientForHint(display, wins[i], &d1)
	       && (wa.map_state == IsViewable || ewmh_getstate(wins[i]) == IconicState))
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
    ewmh_updateclientdesktop(c);
    attach(c);
    attachstack(c);
    focus(NULL);
    arrange(NULL);
}

void ewmh_setclientstate(Client *c, long state) {
    long data[] = { state, None };

    XChangeProperty(display, c->win, wmatom[WMState], wmatom[WMState], 32,
		    PropModeReplace, (unsigned char *)data, 2);
}

static Bool sendevent(Client *c, Atom proto){
    int n;
    Atom *protocols;
    Bool exists = False;
    XEvent ev;

    if(XGetWMProtocols(display, c->win, &protocols, &n)) {
      while(!exists && n--)
        exists = protocols[n] == proto;
      XFree(protocols);

    }
    if(exists) {
    ev.type = ClientMessage;
    ev.xclient.window = c->win;
    ev.xclient.message_type = wmatom[WMProtocols];
	ev.xclient.format = 32;
    ev.xclient.data.l[0] = proto;
	ev.xclient.data.l[1] = CurrentTime;
    XSendEvent(display, c->win, False, NoEventMask, &ev);
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
    sendevent(c, wmatom[WMTakeFocus]);
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

void ewmh_setcurrentdesktop(void) {
    long data[] = { 0 };

    XChangeProperty(display, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32,
		    PropModeReplace, (unsigned char *)data, 1);
}

void ewmh_setnumbdesktops(void) {
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
    screen_w = DisplayWidth(display, screen);
    screen_h = DisplayHeight(display, screen);
    updategeom();

    /* Standard & EWMH atoms */
    ewmh_init();

    /* cursors */
    cursor[CurNormal] = XCreateFontCursor(display, XC_top_left_arrow);
    cursor[CurResize] = XCreateFontCursor(display, XC_bottom_right_corner);
    cursor[CurMove] = XCreateFontCursor(display, XC_fleur);
    cursor[CurCmd] = XCreateFontCursor(display, CURSOR_WAITKEY);
    /* border colors */
    win_unfocus = getcolor(UNFOCUS);
    win_focus = getcolor(FOCUS);

    XDeleteProperty(display, root, netatom[NetClientList]);
    XDeleteProperty(display, root, netatom[NetClientListStacking]);
    /* set EWMH NUMBER_OF_DESKTOPS */
    ewmh_setnumbdesktops();
    /* initialize EWMH CURRENT_DESKTOP */
    ewmh_setcurrentdesktop();
    ewmh_updatecurrenddesktop();
    /* select for events */
    wa.cursor = cursor[CurNormal];
    wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask|PointerMotionMask
	|EnterWindowMask|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
    XChangeWindowAttributes(display, root, CWEventMask|CWCursor, &wa);
    XSelectInput(display, root, wa.event_mask);
    updatenumlockmask();
    grabkeys(PrefixKey);
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
	fprintf(stderr, "calavera-wm: execvp %s", ((char **)arg->v)[0]);
	perror(" failed");
	exit(EXIT_SUCCESS);
    }
}

void sync_display(void) {
    XSync(display, False);
}

void moveto_workspace(const Arg *arg) {
    if(selmon->sel && arg->ui & TAGMASK) {
	selmon->sel->tags = arg->ui & TAGMASK;
        ewmh_updateclientdesktop(selmon->sel);
	focus(NULL);
	arrange(selmon);
    }
    ewmh_updatecurrenddesktop();
}

void moveresize(const Arg *arg) {
  XEvent ev;
  Monitor *m = selmon;

  if(!(m->sel && arg && arg->v && m->sel->isfloating)) 
    return;
  resize(m->sel, m->sel->x + ((int *)arg->v)[0], m->sel->y + ((int *)arg->v)[1], m->sel->w + ((int *)arg->v)[2],
         m->sel->h + ((int *)arg->v)[3], True);

  while(XCheckMaskEvent(display, EnterWindowMask, &ev));
}

void fullscreen(const Arg *arg) {

    if(!selmon->sel)
	return;
    setfullscreen(selmon->sel, !selmon->sel->isfullscreen);
}

void unfocus(Client *c, Bool setfocus) {
    if(!c)
	return;
    grabbuttons(c, False);
    XSetWindowBorder(display, c->win, win_unfocus);
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
	ewmh_setclientstate(c, WithdrawnState);
	sync_display();
	XSetErrorHandler(xerror);
	XUngrabServer(display);
    }
    free(c);
    focus(NULL);
    ewmh_updateclientlist();
    ewmh_updateclientlist_stacking();
    arrange(m);
}

void unmapnotify(XEvent *e) {
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if((c = wintoclient(ev->window))) {
	if(ev->send_event)
	    ewmh_setclientstate(c, WithdrawnState);
	else
	    unmanage(c, False);
    }
}

void set_padding(Monitor *m) {
    m->wy += DOCK_SIZE;
    m->wh -= DOCK_SIZE;
}

void ewmh_updateclientlist() {
    Client *c;
    Monitor *m;

    XDeleteProperty(display, root, netatom[NetClientList]);
    for(m = mons; m; m = m->next)
	for(c = m->clients; c; c = c->next)
	    XChangeProperty(display, root, netatom[NetClientList],
			    XA_WINDOW, 32, PropModeAppend,
			    (unsigned char *) &(c->win), 1);
}

void ewmh_updateclientlist_stacking() {
    Client *c;
    Monitor *m;

    XDeleteProperty(display, root, netatom[NetClientListStacking]);
    for(m = mons; m; m = m->next)
        for(c = m->stack; c; c = c->snext)
            XChangeProperty(display, root, netatom[NetClientListStacking],
                            XA_WINDOW, 32, PropModeAppend,
                            (unsigned char *) &(c->win), 1);
}

void ewmh_updateclientdesktop(Client *c) {
     long data[] = { c->tags };

     XChangeProperty(display, c->win, netatom[NetWMDesktop], XA_CARDINAL, 32,
		     PropModeReplace, (unsigned char *)data, 1);
}

void ewmh_updatecurrenddesktop() {
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
			set_padding(m);
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
		set_padding(mons);
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

void ewmh_updatewindowtype(Client *c) {
    Atom state = getatomprop(c, netatom[NetWMState]);
    Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

    if(state == netatom[NetWMFullscreen])
	setfullscreen(c, True);

    if(wtype == netatom[NetWMWindowTypeDialog]) {
	c->isfloating = True; 
    } else if(wtype == netatom[NetWMWindowTypeDock] 
              || wtype == netatom[NetWMWindowTypeNotification]
              || wtype == netatom[NetWMWindowTypeSplash]) {
      //      c->isdock = True;
      c->neverfocus = True; 
      c->isfloating = True; 
    }
}

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
    ewmh_updatecurrenddesktop();
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

    if(w == root && getrootptr(&x, &y))
      return recttomon(x, y, 1, 1);
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
       || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
       || (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
       || (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
       || (ee->request_code == X_GrabKey && ee->error_code == BadAccess))
	return 0;
    fprintf(stderr, "calavera-wm: fatal error: request code=%d, error code=%d\n",
	    ee->request_code, ee->error_code);
    return xerrorxlib(display, ee); /* may call exit */
}

int xerrordummy(Display *display, XErrorEvent *ee) {
    return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int xerrorstart(Display *display, XErrorEvent *ee) {
    eprint("calavera-wm: another window manager is already running\n");
    return -1;
}

void exec(const Arg *arg) {
    int  pos;
    char tmp[32];
    char buf[BUFSIZE];
    Bool grabbing = True;
    KeySym ks;
    XEvent ev;

    // Clear the array
    memset(tmp, 0, sizeof(tmp));
    memset(buf, 0, sizeof(buf));
    pos = 0;

    XGrabKeyboard(display, ROOT, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    sync_display();

    // grab keys
    while(grabbing){
	if(ev.type == KeyPress) {
	    XLookupString(&ev.xkey, tmp, sizeof(tmp), &ks, 0);

	    switch(ks){
	    case XK_Return:
		goto launch;
		grabbing = False;
		break;
	    case XK_BackSpace:
		if(pos) buf[--pos] = 0;
		break;
	    case XK_Escape:
              goto out;
		break;
	    default:
		strncat(buf, tmp, sizeof(tmp));
		++pos;
		break;
	    }
	    sync_display();
	}
	XNextEvent(display, &ev);
    }

 launch:
    if (pos) {
	char *termcmd[]  = { buf, NULL };
        Arg arg = {.v = termcmd };
	spawn (&arg);
    }
  
 out:
    XUngrabKeyboard(display, CurrentTime);
 
    return;
}

int main(int argc, char *argv[]) {
    if(argc == 2 && !strcmp("-v", argv[1]))
	eprint("calavera-wm-"VERSION", © 2006-2012 dwm engineers, see LICENSE for details\n");
    else if(argc != 1)
	eprint("usage: calavera-wm [-v]\n");
    if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
	fputs("warning: no locale support\n", stderr);
    if(!(display = XOpenDisplay(NULL)))
	eprint("calavera-wm: cannot open display\n");
    cargv = argv;
    checkotherwm();
    setup();
    scan();
    autorun();
    handle_events();
    cleanup();
    /* Close display */
    XCloseDisplay(display);
    return EXIT_SUCCESS;
}

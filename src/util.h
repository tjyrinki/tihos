#ifndef UTIL_H_
#define UTIL_H_

gboolean isLocked();
gboolean isMainVisible();
gboolean timeoutLock(gpointer data);
void removeTimeout();
gboolean undoClose (gpointer data);
void lockScreen(GtkWidget *widget, gpointer data);
void unLockScreen();
void updateBattery();
void refreshWindowList();
void handleX();
void switchWindow(GtkWidget *widget, gpointer data);
void launchCommand(gchar *command);
void menuLaunchCommand(GtkWidget *widget, gpointer data);
void *get_utf8_property(Window win, Atom atom);
void Xclimsg(Window win, long type, long l0, long l1, long l2, long l3, long l4);
void toggleTray(GtkWidget *widget, gpointer data);
void keyboardButton(GtkWidget *widget, gpointer data);
void appsButton(GtkWidget *widget, gpointer data);
void closeButton(GtkWidget *widget, gpointer data);
void switchButtons(GtkWidget *widget, gpointer data);
void createUI();
static void destroy(GtkWidget *widget, gpointer data);

static gint x_fd = 0;
static gboolean really_close = FALSE, screen_locked = FALSE, action_buttons = FALSE;
static Display *display = NULL;
static Window *windows = NULL;
static Window own_window;
static GdkWindow *own_gdk_window = NULL;
static gulong number_of_windows = 0, selected_item = 0;
static guint lock_timeout_source = 0;
static GtkWidget *main_window, *vbox, *lock_text, *table, *battery, *lockbutton, *hbox, *morebutton, *keyboardbutton, *vbox2, *appsbutton, *vbox3, *closebutton;
static GtkWidget *button[99];

#endif /* UTIL_H_ */


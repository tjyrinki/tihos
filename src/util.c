#include "tihos.h"
#include "util.h"
#include <omhacks/all.h>

gboolean isLocked() {
    if (GTK_WIDGET_VISIBLE(lock_text)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

gboolean isMainVisible() {
    if (GTK_WIDGET_VISIBLE(main_window)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

/* Callback to lock screen after timeout */
gboolean timeoutLock(gpointer data) {
    lockScreen(lockbutton, NULL);
    return FALSE;
}

/* Remove or postpone timeout when certain actions are done */
void removeTimeout() {
    if (lock_timeout_source != 0) {
        g_source_remove(lock_timeout_source);
        lock_timeout_source = 0;
    }
}

/* Restore non-highlighted close button after timeout */
gboolean undoClose (gpointer data) {
    gtk_widget_modify_bg(closebutton, GTK_STATE_NORMAL, NULL);
    gtk_widget_modify_bg(closebutton, GTK_STATE_SELECTED, NULL);
    really_close = FALSE;
    return FALSE;
}

/* Lock screen display */
void lockScreen(GtkWidget *widget, gpointer data) {
    removeTimeout();
    if (screen_locked == FALSE) {
        gtk_widget_hide(vbox);
        gtk_container_remove(GTK_CONTAINER(main_window), vbox);
        gtk_container_add(GTK_CONTAINER(main_window), lock_text);
        gtk_widget_show(lock_text);
        screen_locked = TRUE;
        removeTimeout();
        lock_timeout_source = g_timeout_add_seconds(15, timeoutLock, main_window);
        launchCommand("om screen power 0");
    }
    else {
        return;
    }
}

/* Unlock screen and postpone locking timeout */
void unLockScreen() {
    removeTimeout();
    lock_timeout_source = g_timeout_add_seconds(15, timeoutLock, main_window);
    if (screen_locked == TRUE) {
        screen_locked = FALSE;
        gtk_widget_hide(lock_text);
        gtk_container_remove(GTK_CONTAINER(main_window), lock_text);
        gtk_container_add(GTK_CONTAINER(main_window), vbox);
        gtk_widget_show(vbox);
        launchCommand("om screen power 1");
    }
    else {
        return;
    }
}

/* Read battery level */
void updateBattery() {
    gint charge;
    gchar charging;
    gchar batteryText[1024];
    FILE *file;
    file=fopen("/sys/class/power_supply/battery/capacity", "r");
    fscanf(file, "%d", &charge);
    fclose(file);
    file=fopen("/sys/class/power_supply/battery/status", "r");
    fscanf(file, "%c", &charging);
    fclose(file);
    if(charging == 'C') charge+=200;
    if (charge > 100) {
        charge-=200;
        sprintf(batteryText, _("Bat: %d%% +"), charge);
    }
    else {
        sprintf(batteryText, _("Bat: %d%%"), charge);
    }
    gtk_label_set_text(GTK_LABEL(battery), batteryText);
}

/* Refresh the window list */
void refreshWindowList() {
    Atom a_NET_WM_NAME = XInternAtom(GDK_DISPLAY(), "_NET_WM_NAME", False);
    Atom a_NET_CLIENT_LIST = XInternAtom(GDK_DISPLAY(), "_NET_CLIENT_LIST", False);
    Atom type_ret;
    gint format_ret;
    gint i = 0, x = 0, y = 0;
    gulong items_ret, after_ret;
    char selected[1024];
    char selected2[1024];
    char *name = NULL;

    action_buttons = FALSE;

    /* Clean previous */
    for (i = 0; i < number_of_windows; i++) { 
        gtk_container_remove(GTK_CONTAINER(table), button[i]);
    }

    gtk_table_resize(GTK_TABLE(table), 3, 3);

    XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), a_NET_CLIENT_LIST, 0, G_MAXLONG, False, XA_WINDOW, &type_ret, &format_ret, &items_ret, &after_ret, (guchar **)&windows);
    number_of_windows=items_ret;

    if (number_of_windows > 9) number_of_windows = 9;
    for (i = 0; i < number_of_windows; i++) { 
        name = get_utf8_property(windows[i],  a_NET_WM_NAME);
        if (strcmp (name, "neolockscreen") == 0) {
            own_gdk_window = gdk_window_foreign_new(windows[i]);
        }
        button[i] = gtk_button_new();
        snprintf(selected, 19, "\n\n[%d] %s             ", i, name);
        sprintf(selected2, "%s\n\n", selected);
        gtk_button_set_label(GTK_BUTTON(button[i]), selected2);
        gtk_table_attach(GTK_TABLE(table), button[i], x, x+1, y, y+1, GTK_FILL, GTK_FILL, 0, 0);
        gtk_widget_show(button[i]);
        g_signal_connect(G_OBJECT(button[i]), "clicked", G_CALLBACK(switchWindow), NULL);
        x++;
        if (x>2) { x=0; y++; }
    }
}

/* Handle X keyboard event (AUX button) */
void handleX() {
    XEvent event;
    int nfds;
    fd_set readset;
    struct timeval timeout;
    gettimeofday(&timeout, 0);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    FD_ZERO (&readset);
    FD_SET (x_fd, &readset);
    nfds = x_fd+1;

    if (-1 == select (nfds, &readset, NULL, NULL, &timeout)) {
        printf("Select returned error\n");
        return;
    }
    else  {
        if (FD_ISSET (x_fd, &readset)) {
            while (XPending (GDK_DISPLAY())) {
                XNextEvent (GDK_DISPLAY(), &event);
                if (event.type == KeyPress) {
                    if (event.xkey.keycode == XKeysymToKeycode(GDK_DISPLAY(), XStringToKeysym(AUXKEY_NAME))) {
                        if (GTK_WIDGET_VISIBLE(main_window)) {
                            /* Hide window */
                            unLockScreen();
                            gtk_widget_hide_all(main_window);
                           return;
                        }
                        else {
                            /* Make tihos visible */
                            if (screen_locked == TRUE) {
                                unLockScreen();
                            }
                            gtk_widget_show_all(main_window);
                            /* Delay locking since button was pressed */
                            removeTimeout();
                            lock_timeout_source = g_timeout_add_seconds(15, timeoutLock, main_window);
                            gtk_main_iteration();
                            updateBattery();
                            // HAXOR, main_window = non-NULL, switch to own window
                            switchWindow(main_window, main_window);
                            /* Refresh window list */
                            refreshWindowList();
//                            close (x_fd);
                            return;
                        }
                    }
                    if (event.xkey.keycode == XKeysymToKeycode(GDK_DISPLAY(), XStringToKeysym(POWERKEY_NAME))) {
                        if (screen_locked == FALSE) {
                            lockScreen(lockbutton, NULL);
                            launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/manualsuspend", NULL));
                        }
                    }
                }
            }
        }
    }
}

/* Switch to selected window from the window list */
void switchWindow(GtkWidget *widget, gpointer data) {
    Atom a_NET_CURRENT_DESKTOP = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
    Atom a_NET_ACTIVE_WINDOW = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    if (data == NULL && gdk_window_foreign_new != NULL) {
        selected_item = atoi(&gtk_button_get_label(GTK_BUTTON(widget))[3]);
        Xclimsg(windows[selected_item], a_NET_CURRENT_DESKTOP, 0, 0, 0, 0, 0);
        Xclimsg(windows[selected_item], a_NET_ACTIVE_WINDOW, 2L, CurrentTime, 0, 0, 0);
    }
    else {
        Xclimsg(GDK_WINDOW_XID(own_gdk_window), a_NET_CURRENT_DESKTOP, 0, 0, 0, 0, 0);
        Xclimsg(GDK_WINDOW_XID(own_gdk_window), a_NET_ACTIVE_WINDOW, 2L, CurrentTime, 0, 0, 0);
        return;
    }
    XSync (display, False);
    XFlush (display);
    removeTimeout();
    gtk_widget_hide_all(main_window);
}



void launchCommand(gchar *command) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp ("/bin/sh", "sh", "-c", command, NULL);
        fprintf (stderr, "Execlp error.\n");
        _exit(-1);
    }
}

/* Launch application and hide tihos */
void menuLaunchCommand(GtkWidget *widget, gpointer data) {
    launchCommand(data);
    /* Hide when selected */
    unLockScreen();
    gtk_widget_hide_all(main_window);
}

void *get_utf8_property(Window win, Atom atom) {
    Atom a_UTF8_STRING;
    a_UTF8_STRING = XInternAtom(GDK_DISPLAY(), "UTF8_STRING", False);
    Atom type;
    int format;
    gulong nitems;
    gulong bytes_after;
    gchar  *retval;
    int result;
    guchar *tmp = NULL;

    type = None;
    retval = NULL;
    result = XGetWindowProperty (GDK_DISPLAY(), win, atom, 0, G_MAXLONG, False,
          a_UTF8_STRING, &type, &format, &nitems,
          &bytes_after, &tmp);
    if (result != Success)
        return NULL;
    if (tmp) {
        if (type == a_UTF8_STRING && format == 8 && nitems != 0)
            retval = g_strndup ((gchar *)tmp, nitems);
        XFree (tmp);
    }
    return retval;
}

void Xclimsg(Window win, long type, long l0, long l1, long l2, long l3, long l4) {
    XClientMessageEvent xev;

    xev.type = ClientMessage;
    xev.window = win;
    xev.send_event = True;
    xev.display = GDK_DISPLAY();
    xev.message_type = type;
    xev.format = 32;
    xev.data.l[0] = l0;
    xev.data.l[1] = l1;
    xev.data.l[2] = l2;
    xev.data.l[3] = l3;
    xev.data.l[4] = l4;
    XSendEvent(GDK_DISPLAY(), XDefaultRootWindow(GDK_DISPLAY()), False,
          (SubstructureNotifyMask | SubstructureRedirectMask),
          (XEvent *) & xev);
}

/* Callbacks from buttons */
static void helperForceUSB(GtkWidget *widget, gpointer data) {
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/forceusbpower", NULL));
}
static void helperProfileMusic(GtkWidget *widget, gpointer data) {
    launchCommand("alsactl -f /usr/share/openmoko/scenarios/headset.state restore");
}
static void helperProfilePhone(GtkWidget *widget, gpointer data) {
    launchCommand("alsactl -f /usr/share/openmoko/scenarios/gsmhandset.state restore");
}
static void helperUSBHostMode(GtkWidget *widget, gpointer data) {
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/hostmode_switch", NULL));
}
static void helperRotate(GtkWidget *widget, gpointer data) {
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/rotate", NULL));
}
static void helperScreenon(GtkWidget *widget, gpointer data) {
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/screenonswitch", NULL));
}
static void helperBtNetFw(GtkWidget *widget, gpointer data) {
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/btnetfw", NULL));
}
static void helperGprsOff(GtkWidget *widget, gpointer data) {
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/gprsoff", NULL));
}
static void helperGprsOn(GtkWidget *widget, gpointer data) {
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/gprson", NULL));
}
static void helperGpsOff(GtkWidget *widget, gpointer data) {
    launchCommand("/etc/init.d/gpsd stop");
    sleep(1);
    om_gps_power_set(0);
}
static void helperGpsOn(GtkWidget *widget, gpointer data) {
    om_gps_power_set(1);
    sleep(1);
    launchCommand("/etc/init.d/gpsd start");
}
static void helperBluetoothOff(GtkWidget *widget, gpointer data) {
    om_bt_power_set(0);
    launchCommand("killall pand");
}
static void helperBluetoothOn(GtkWidget *widget, gpointer data) {
    om_bt_power_set(1);
    sleep(2);
    launchCommand("/sbin/hciconfig hci0 up");
    sleep(1);
    launchCommand("/sbin/hciconfig hci0 piscan");
    launchCommand("pand --listen --role NAP --master --autozap");
}
static void helperWlanOff(GtkWidget *widget, gpointer data) {
    om_wifi_power_set(0);
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/wlan stop", NULL));
    sleep(1);
    launchCommand("iwconfig eth1 txpower off");
}
static void helperWlanOn(GtkWidget *widget, gpointer data) {
    om_wifi_power_set(1);
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/wlan start", NULL));
    sleep(1);
    launchCommand("iwconfig eth1 txpower auto");
}
static void helper3GOn(GtkWidget *widget, gpointer data) {
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/3g", NULL));
}
static void helperPowerOff(GtkWidget *widget, gpointer data) {
    launchCommand("poweroff");
}
void toggleTray(GtkWidget *widget, gpointer data) {
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/toggle_tray", NULL));
}
void keyboardButton(GtkWidget *widget, gpointer data) {
    launchCommand(g_strjoin(NULL, PACKAGE_SCRIPTS_DIR, "/literki_switch", NULL));
    /* Hide when selected */
    unLockScreen();
    gtk_widget_hide_all(main_window);
}
void appsButton(GtkWidget *widget, gpointer data) {
    removeTimeout();
    lock_timeout_source = g_timeout_add_seconds(15, timeoutLock, main_window);
    GtkMenu *menu;
    menu = GTK_MENU (data);
    gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 0, 0);
}
void closeButton(GtkWidget *widget, gpointer data) {
    /* First set the background color of the button red */
    if (really_close == FALSE) {
        GdkColor my_red;
        my_red.red = 0xffff;
        my_red.green = 0x0000;
        my_red.blue = 0x0f00;
        /* For unknown reason red not visible until focus elsewhere */
        gtk_widget_modify_bg(closebutton, GTK_STATE_NORMAL, &my_red);
        gtk_widget_modify_bg(closebutton, GTK_STATE_SELECTED, &my_red);
        gtk_button_released(GTK_BUTTON(closebutton));
        gtk_button_leave(GTK_BUTTON(closebutton));
        really_close = TRUE;
        g_timeout_add_seconds(3, undoClose, NULL);
        gtk_widget_hide(closebutton);
        gtk_widget_show(closebutton);
        return;
    }

    really_close = FALSE;
    /* If already clicked once, proceed */
    gtk_widget_modify_bg(closebutton, GTK_STATE_NORMAL, NULL);
    gtk_widget_modify_bg(closebutton, GTK_STATE_SELECTED, NULL);

    /* Hide when selected */
    unLockScreen();
    gtk_widget_hide_all(main_window);

    /* Close window */
    unsigned int keycode = XKeysymToKeycode(display, XK_F4);
    unsigned int Alt_code = XKeysymToKeycode(display, XK_Alt_L);
    XTestFakeKeyEvent(display, Alt_code, True, 0);
    XTestFakeKeyEvent(display, keycode, True, 0);
    XTestFakeKeyEvent(display, keycode, False, 0);
    XTestFakeKeyEvent(display, Alt_code, False, 0);
}

/* Switch between special buttons and window list, ie. the "More..." button */
void switchButtons(GtkWidget *widget, gpointer data) {
    gint i = 0;
    /* Delay locking since button was pressed */
    removeTimeout();
    lock_timeout_source = g_timeout_add_seconds(15, timeoutLock, main_window);
    if (action_buttons == FALSE) {
        for (i = 0; i < number_of_windows; i++) { 
            gtk_container_remove(GTK_CONTAINER(table), button[i]);
        }
        action_buttons = TRUE;
        gtk_table_resize(GTK_TABLE(table), 5, 3);

        number_of_windows = 15;
        for (i = 0; i < number_of_windows; i++) {
            button[i] = gtk_button_new();
        }
        gtk_button_set_label(GTK_BUTTON(button[0]), _("\n  Force USB  \n"));
        g_signal_connect(G_OBJECT(button[0]), "clicked", G_CALLBACK(helperForceUSB), NULL);
        gtk_button_set_label(GTK_BUTTON(button[1]), _("\n  Profile: music  \n"));
        g_signal_connect(G_OBJECT(button[1]), "clicked", G_CALLBACK(helperProfileMusic), NULL);
        gtk_button_set_label(GTK_BUTTON(button[2]), _("\n  Profile: phone  \n"));
        g_signal_connect(G_OBJECT(button[2]), "clicked", G_CALLBACK(helperProfilePhone), NULL);
        gtk_button_set_label(GTK_BUTTON(button[3]), _("\n  USB host mode  \n"));
        g_signal_connect(G_OBJECT(button[3]), "clicked", G_CALLBACK(helperUSBHostMode), NULL);
        gtk_button_set_label(GTK_BUTTON(button[4]), _("\n  Rotate  \n"));
        g_signal_connect(G_OBJECT(button[4]), "clicked", G_CALLBACK(helperRotate), NULL);
        gtk_button_set_label(GTK_BUTTON(button[5]), _("\n Screen on switch \n"));
        g_signal_connect(G_OBJECT(button[5]), "clicked", G_CALLBACK(helperScreenon), NULL);
        gtk_button_set_label(GTK_BUTTON(button[6]), _("\n  BT net fw  \n"));
        g_signal_connect(G_OBJECT(button[6]), "clicked", G_CALLBACK(helperBtNetFw), NULL);
        gtk_button_set_label(GTK_BUTTON(button[7]), _("\n GPRS off \n"));
        g_signal_connect(G_OBJECT(button[7]), "clicked", G_CALLBACK(helperGprsOff), NULL);
        gtk_button_set_label(GTK_BUTTON(button[8]), _("\n GPRS on \n"));
        g_signal_connect(G_OBJECT(button[8]), "clicked", G_CALLBACK(helperGprsOn), NULL);
        if (om_gps_power_get() == 1) {
            gtk_button_set_label(GTK_BUTTON(button[9]), _("\n GPS off \n"));
            g_signal_connect(G_OBJECT(button[9]), "clicked", G_CALLBACK(helperGpsOff), NULL);
        }
        else {
            gtk_button_set_label(GTK_BUTTON(button[9]), _("\n GPS on \n"));
            g_signal_connect(G_OBJECT(button[9]), "clicked", G_CALLBACK(helperGpsOn), NULL);
        }
        if (om_bt_power_get() == 1) { 
            gtk_button_set_label(GTK_BUTTON(button[10]), _("\n Bluetooth off \n"));
            g_signal_connect(G_OBJECT(button[10]), "clicked", G_CALLBACK(helperBluetoothOff), NULL);
        }
        else {
            gtk_button_set_label(GTK_BUTTON(button[10]), _("\n Bluetooth on \n"));
            g_signal_connect(G_OBJECT(button[10]), "clicked", G_CALLBACK(helperBluetoothOn), NULL);
        }
        if (om_wifi_power_get() == 1) {
            gtk_button_set_label(GTK_BUTTON(button[11]), _("\n WLAN off \n"));
            g_signal_connect(G_OBJECT(button[11]), "clicked", G_CALLBACK(helperWlanOff), NULL);
        }
        else {
            gtk_button_set_label(GTK_BUTTON(button[11]), _("\n WLAN on \n"));
            g_signal_connect(G_OBJECT(button[11]), "clicked", G_CALLBACK(helperWlanOn), NULL);
        }
        gtk_button_set_label(GTK_BUTTON(button[12]), _("\n 3G On \n"));
        g_signal_connect(G_OBJECT(button[12]), "clicked", G_CALLBACK(helper3GOn), NULL);
        gtk_button_set_label(GTK_BUTTON(button[13]), _("\n POWER OFF \n"));
        g_signal_connect(G_OBJECT(button[13]), "clicked", G_CALLBACK(helperPowerOff), NULL);
        gtk_button_set_label(GTK_BUTTON(button[14]), _("\n Toggle tray \n"));
        g_signal_connect(G_OBJECT(button[14]), "clicked", G_CALLBACK(toggleTray), NULL);

        gint x = 0, y = 0;
        /* Add to table and show */
        for (i = 0; i < number_of_windows; i++) {
            gtk_table_attach(GTK_TABLE(table), button[i], x, x+1, y, y+1, GTK_FILL, GTK_FILL, 0, 0);
            gtk_widget_show(button[i]);
            x++;
            if (x>2) { x=0; y++; }
        }

    }
    else {
        refreshWindowList();
        updateBattery();
    }
}

void createUI() {
    GdkColor myblack;
    myblack.red = 0x0000;
    myblack.green = 0x0000;
    myblack.blue = 0x0000;

    /* Main windows and basis for task buttons */
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(main_window), 0);
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);
    gtk_widget_show(vbox);
    table = gtk_table_new(3,3,TRUE);
    gtk_container_add(GTK_CONTAINER(vbox), table);
    gtk_widget_show(table);
    gtk_widget_modify_bg(main_window, GTK_STATE_NORMAL, &myblack);
    gtk_widget_show(main_window);
    gtk_window_fullscreen(GTK_WINDOW(main_window));

    /* Lock screen and button widgets */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(vbox), hbox);
    gtk_widget_show(hbox);
    lockbutton = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(lockbutton), _("\n\n\nLock screen\n\n\n"));
    gtk_container_add(GTK_CONTAINER(hbox), lockbutton);
    gtk_widget_show(lockbutton);
    vbox2 = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(hbox), vbox2);
    gtk_widget_show(vbox2);
    keyboardbutton = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(keyboardbutton), _("\nKeyboard\n"));
    gtk_container_add(GTK_CONTAINER(vbox2), keyboardbutton);
    gtk_widget_show(keyboardbutton);
    appsbutton = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(appsbutton), _("\nApplications\n"));
    gtk_container_add(GTK_CONTAINER(vbox2), appsbutton);
    gtk_widget_show(appsbutton);
    vbox3 = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(hbox), vbox3);
    gtk_widget_show(vbox3);
    closebutton = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(closebutton), _("\nClose App\n(tap twice)\n"));
    gtk_container_add(GTK_CONTAINER(vbox3), closebutton);
    gtk_widget_show(closebutton);
    morebutton = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(morebutton), _("\n\n\nMore...\n\n\n"));
    gtk_container_add(GTK_CONTAINER(vbox3), morebutton);
    gtk_widget_show(morebutton);

    lock_text = GTK_WIDGET(gtk_label_new(NULL));
    gtk_label_set_markup(GTK_LABEL(lock_text), _("<span size=\"36000\" background=\"black\"><b>☻</b></span>"));

    battery = GTK_WIDGET(gtk_label_new(_("Bat: ? %")));
    gtk_container_add(GTK_CONTAINER(vbox), battery);
    gtk_widget_show(battery);

    display = GDK_DISPLAY();

    /* Nngggghh */
/*
    g_object_ref(main_window);
    g_object_ref(lockbutton);
    g_object_ref(table);
    g_object_ref(lock_text);
*/
    g_object_ref(vbox);
    g_object_ref(table);
    g_object_ref(table);
    g_object_ref(lock_text);

    /* Apps */
//    GtkWidget *apps = gtk_notebook_new();
    GtkWidget *apps_menu = gtk_menu_new(), *menu, *menuitem;
    FILE *file;
    char buf[1024];
    char command[128][255];
    gint command_nr = 0;
    file=fopen("/tmp/menu.lst", "r");
    menu = apps_menu;
    do {
        fgets(buf, sizeof(buf), file);
        if (strstr(buf, "MENU") != NULL) {
            menuitem = gtk_menu_item_new_with_label(buf+4);
            menu = gtk_menu_new();
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
            gtk_widget_show(menu);
            gtk_menu_shell_append(GTK_MENU_SHELL(apps_menu), menuitem);
            gtk_widget_show(menuitem);
        }
        if (strstr(buf, "ITEM") != NULL) {
            menuitem = gtk_menu_item_new_with_label(buf+4);
            gtk_menu_append(GTK_MENU(menu), menuitem);
            fgets(command[command_nr], sizeof(command[command_nr]), file);
            gtk_signal_connect(GTK_OBJECT(menuitem), "activate", GTK_SIGNAL_FUNC(menuLaunchCommand), command[command_nr]);
            gtk_widget_show(menuitem);
        }
        command_nr++;
    } while (!feof(file) && command_nr < 128);
    fclose(file);
    gtk_widget_show(apps_menu);

    x_fd=XConnectionNumber(GDK_DISPLAY());

   /* Signals */
    XGrabKey (display, XKeysymToKeycode (display, XStringToKeysym (AUXKEY_NAME)), 0, XDefaultRootWindow(GDK_DISPLAY()), False, GrabModeAsync, GrabModeAsync);
    XGrabKey (display, XKeysymToKeycode (display, XStringToKeysym (POWERKEY_NAME)), 0, XDefaultRootWindow(GDK_DISPLAY()), False, GrabModeAsync, GrabModeAsync);
    g_signal_connect(G_OBJECT(main_window), "destroy", G_CALLBACK(destroy), NULL);
//    g_signal_connect(G_OBJECT(main_window), "key-press-event", G_CALLBACK(gtkKeyPress), NULL);
    g_signal_connect(G_OBJECT(lockbutton), "clicked", G_CALLBACK(lockScreen), NULL);
    g_signal_connect(G_OBJECT(keyboardbutton), "clicked", G_CALLBACK(keyboardButton), NULL);
    g_signal_connect(G_OBJECT(appsbutton), "clicked", G_CALLBACK(appsButton), apps_menu);
    g_signal_connect(G_OBJECT(morebutton), "clicked", G_CALLBACK(switchButtons), NULL);
    g_signal_connect(G_OBJECT(closebutton), "clicked", G_CALLBACK(closeButton), NULL);
}

/* Quit program callback */
void destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}


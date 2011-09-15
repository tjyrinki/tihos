/* Tihos window switcher, home screen or something like that
 *
 * License GPLv3 or later, (c) Timo Jyrinki <timo.jyrinki@iki.fi>.
 * 
 *    get_utf8_property copy-pasted from fbpanel:
 *    BSD license, (c) Anatoly Asviyan, Peter Zelezn
 */

#include "tihos.h"
#include "util.h"

/* Main program and UI creation */
int main (int argc, char *argv[]) {
    g_type_init();
    gtk_init(&argc, &argv);

    bindtextdomain("tihos", PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset("tihos", "UTF-8");
    textdomain("tihos");

    createUI();
    updateBattery();
    refreshWindowList();

    /* Check if to start with screen lock */
    if (argc > 1) {
        if (strcmp(argv[1], "--lock") == 0) {
            lockScreen(NULL, NULL);
        }
    }
 
    /* Key grabbing and looping depending on where we are */
    while (1) {
        if (isLocked()) {
            while (gtk_events_pending()) {
                gtk_main_iteration();
            }
            handleX();
        }
        else if (isMainVisible()) {
            while (gtk_events_pending()) {
                gtk_main_iteration();
            }
        }
        else {
            handleX();
        }
    }
}


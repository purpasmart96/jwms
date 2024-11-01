#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>

#include <X11/Xlib.h>

static void SignalHandler(int signal)
{
    switch (signal)
    {
        case SIGINT:
        case SIGTERM:
            syslog(LOG_NOTICE, "JWMS: Received termination signal. Stopping session...");
            exit(0);
            break;
        default:
            break;
    }
}

static int CheckForOtherWM(void)
{
    // Open a connection to the X server
    Display *display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        syslog(LOG_ERR, "JWMS: Failed to open X display.");
        return -1;
    }

    // Get the atom for the selection "WM_S0" (window manager selection for screen 0)
    Atom wm_sel = XInternAtom(display, "WM_S0", False);

    // Get the owner of the WM_S0 selection
    Window owner = XGetSelectionOwner(display, wm_sel);
    if (owner != None)
    {
        printf("JWMS: A another Window manager is already running. Exiting...\n");
        syslog(LOG_ERR, "Another window manager is already running! Exiting...");
        XCloseDisplay(display);
        return -1;
    }

    // Close the display connection
    XCloseDisplay(display);

    return 0;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    syslog(LOG_INFO, "JWMS: Starting JWMS");

    if (CheckForOtherWM() != 0)
    {
        return EXIT_FAILURE;
    }

    char *jwm_helper_args[] = { "jwm-helper", "-a", NULL };

    pid_t jwm_helper_pid = fork();
    if (jwm_helper_pid < 0)
    {
        syslog(LOG_ERR, "JWMS: Failed to create child process for jwm-helper: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    if (jwm_helper_pid == 0)
    {
        //const char *username = getenv("USER");
        //const char *home = getenv("HOME");
        //syslog(LOG_INFO, "JWMS: Logged as user: %s", username);
        //syslog(LOG_INFO, "JWMS: Home dir at: %s", home);

        syslog(LOG_INFO, "JWMS: Running jwm-helper...");
        // Child process: Execute jwm-helper
        if (execvp("jwm-helper", jwm_helper_args) < 0)
        {
            syslog(LOG_ERR, "JWMS: jwm-helper failed to run: %s", strerror(errno));
            return EXIT_FAILURE;
        }
    }

    // Wait for jwm-helper to finish
    int jwm_helper_status;
    if (waitpid(jwm_helper_pid, &jwm_helper_status, 0) < 0)
    {
        syslog(LOG_ERR, "JWMS: Error waiting for jwm-helper: %s", strerror(errno));
    }
    else if (WIFEXITED(jwm_helper_status))
    {
        syslog(LOG_INFO, "JWMS: jwm-helper exited with status %d", WEXITSTATUS(jwm_helper_status));
    }

    // jwm-helper is done, let's now start JWM
    pid_t jwm_pid = fork();
    if (jwm_pid < 0)
    {
        syslog(LOG_ERR, "JWMS: Failed to create child process for jwm: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    if (jwm_pid == 0)
    {
        syslog(LOG_INFO, "JWMS: Starting jwm");
        // Child process: Execute JWM
        if (execlp("jwm", "jwm", NULL) < 0)
        {
            syslog(LOG_ERR, "JWMS: jwm failed to run: %s", strerror(errno));
            return EXIT_FAILURE;
        }
    }

    // Wait for JWM to finish
    int jwm_status;
    syslog(LOG_INFO, "JWMS: Session manager running...");

    if (waitpid(jwm_pid, &jwm_status, 0) < 0)
    {
        syslog(LOG_ERR, "JWMS: Error waiting for jwm: %s", strerror(errno));
    }
    else if (WIFSIGNALED(jwm_status))
    {
        syslog(LOG_INFO, "JWMS:jwm was terminated by signal %d", WTERMSIG(jwm_status));
    }

    return EXIT_SUCCESS;
}

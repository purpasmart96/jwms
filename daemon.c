
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

static int running = 1;

static void SignalHandler(int signal)
{
    if (signal == SIGKILL || signal == SIGTERM || signal == SIGINT)
    {
        running = 0;
        //syslog(LOG_NOTICE, "My daemon was killed.");
        //exit(0);
    }
}
   
static void jwms_daemon()
{
    pid_t pid;
    
    // Fork off the parent process
    pid = fork();
    
    // An error occurred
    if (pid < 0)
        exit(EXIT_FAILURE);
    
    // Success: Let the parent terminate
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    // On success: The child process becomes session leader
    if (setsid() < 0)
        exit(EXIT_FAILURE);
    
    // Catch, ignore and handle signals
    //signal(SIGCHLD, SIG_IGN);
    //signal(SIGHUP, SIG_IGN);

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGKILL, SignalHandler);
    
    // Fork off for the second time
    pid = fork();
    
    // An error occurred
    if (pid < 0)
        exit(EXIT_FAILURE);
    
    // Success: Let the parent terminate
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    // Set new file permissions
    umask(0);
    
    // Change the working directory to the root directory
    chdir("/");
    
    // Close all open file descriptors
    for (int x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }
    
    // Open the log file
    openlog("jwms_daemon", LOG_PID, LOG_DAEMON);
}

int main()
{
    jwms_daemon();
    syslog(LOG_NOTICE, "jwms daemon started.");

    // Call the child
    //const char *home = getenv("HOME");
    //execl("/home/matt/Documents/jwms/jwms", "jwms", NULL);
    // This is obvously just temporary...
    int ret = system("/home/matt/Documents/jwms/jwms");
    if (ret != 0)
        syslog(LOG_NOTICE, "jwms daemon failed to run other program");

    while (running)
    {
        //TODO: Insert daemon code here.
        syslog(LOG_NOTICE, "jwms daemon is running");
        sleep(30);
    }
   
    syslog (LOG_NOTICE, "jwms daemon terminated.");
    closelog();
    
    return EXIT_SUCCESS;
}

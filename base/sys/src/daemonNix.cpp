#include <string>
#include <cstring>
#include <system_error>

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "config.h"
#include "logger.h"
#include "daemonNix.h"

void signalAction(int sig, siginfo_t* siginfo, void*)
{
    switch (sig)
    {
        case SIGCHLD:
        {
            /* Wait for all dead processes.
             * We use a non-blocking call to be sure this signal handler
             * will not block if a child was cleaned up in another part of
             * the program.
             */
            while (waitpid(-1, nullptr, WNOHANG) > 0)
            {
            }
        }
        break;

        case SIGINT:
        case SIGTERM:
        {
            // Do shutdown.
            parrot::DaemonNix::getInstance().shutdownDaemon();
        }
        break;

        case SIGUSR1:
        {
            // Logger level + 1.
        }
        break;

        case SIGUSR2:
        {
            // Logger level -1.
        }
        break;

        default:
        {
            LOG_WARN("Global::signalAction: Received signal: "
                     << sig << ". User is " << (long)siginfo->si_pid
                     << ". Group is " << (long)siginfo->si_uid << ".");
        }
        break;
    }
}

namespace parrot
{
DaemonNix::DaemonNix()
    : _lockFd(0), _isShutdown(false), _shutdownCb(), _config(nullptr)
{
}

DaemonNix& DaemonNix::getInstance()
{
    static DaemonNix daemon;
    return daemon;
}

void DaemonNix::setConfig(const Config* cfg)
{
    _config = cfg;
}

void DaemonNix::registerShutdownCb(std::function<void()> cb)
{
    _shutdownCb = std::move(cb);
}

void DaemonNix::beforeThreadsStart()
{
    blockAllSignals();
    handleSignal();
}

void DaemonNix::afterThreadsStart()
{
    unblockAllSignals();
}

void DaemonNix::blockAllSignals()
{
    sigset_t set;
    sigfillset(&set); // Fill all signals.

    int s = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (s != 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::blockAllSignals");
    }
}

void DaemonNix::unblockAllSignals()
{
    sigset_t set;
    sigfillset(&set); // Fill all signals.

    int s = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    if (s != 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::unblockAllSignals");
    }
}

void DaemonNix::handleSignal()
{
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_sigaction = signalAction;

    if (sigaction(SIGINT, &sa, nullptr) == -1)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::handleSignal: SIGHUP");
    }

    if (sigaction(SIGTERM, &sa, nullptr) == -1)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::handleSignal: SIGTERM");
    }
    
    if (sigaction(SIGUSR1, &sa, nullptr) == -1)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::handleSignal: SIGUSR1");
    }

    if (sigaction(SIGUSR2, &sa, nullptr) == -1)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::handleSignal: SIGUSR2");
    }

    if (sigaction(SIGCHLD, &sa, nullptr) == -1)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::handleSignal: SIGCHLD");
    }

    if (sigaction(SIGTSTP, &sa, nullptr) == -1)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::handleSignal: SIGTSTP");
    }

    if (sigaction(SIGTTOU, &sa, nullptr) == -1)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::handleSignal: SIGTTOU");
    }

    if (sigaction(SIGTTIN, &sa, nullptr) == -1)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::handleSignal: SIGTTIN");
    }
}

bool DaemonNix::isShutdown() const
{
    return _isShutdown;
}

void DaemonNix::shutdownDaemon()
{
    _isShutdown = true;
    _shutdownCb();
}

void DaemonNix::daemonize()
{
    // This function is referenced from:
    // http://www.enderunix.org/docs/eng/daemon.php

    if (getppid() == 1)
    {
        return; /* Already a daemon. */
    }

    int i = fork();

    if (i < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::daemonize: fork");
    }

    if (i > 0)
    {
        exit(0); /* Parent exits */
    }

    /* Child (daemon) continues here ...*/

    /**
     * A process receives signals from the terminal that it is connected to,
     * and each process inherits its parent's controlling tty. A server
     * should not receive signals from the process that started it, so it
     * must detach itself from its controlling tty.
     * In Unix systems, processes operates within a process group, so that
     * all processes within a group is treated as a single entity. Process
     * group or session is also inherited. A server should operate
     * independently from other processes.
     * This call will place the server in a new process group and session and
     * detach its controlling terminal. (setpgrp() is an alternative for this)
     */
    setsid();

    /**
     * Open descriptors are inherited to child process, this may cause the
     * use of resources unneccessarily. Unneccesarry descriptors should be
     * closed before fork() system call (so that they are not inherited)
     * or close all open descriptors as soon as the child process starts
     * running.
     */
    for (i = getdtablesize(); i >= 0; --i)
    {
        ::close(i);
    }

    /**
     * There are three standart I/O descriptors: standart input 'stdin' (0),
     * standart output 'stdout' (1), standart error 'stderr' (2). A standard
     * library routine may read or write to standart I/O and it may occur to
     * a terminal or file. For safety, these descriptors should be opened
     * and connectedthem to a harmless I/O device (such as /dev/null).
     */
    i = open("/dev/null", O_RDWR);
    dup(i);
    dup(i);

    /**
     * Most servers runs as super-user, for security reasons they should
     * protect files that they create. Setting user mask will pre vent
     * unsecure file priviliges that may occur on file creation.
     */
    umask(122); // This will restrict file creation mode to 644.

    // Create lock file.
    _lockFd = open(_config->_lockFilePath.c_str(), O_RDWR | O_CREAT, 0640);
    if (_lockFd < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::daemonize: open");
    }

    /**
     * Most services require running only one copy of a server at a time.
     * File locking method is a good solution for mutual exclusion. The
     * first instance of the server locks the file so that other instances
     * understand that an instance is already running. If server terminates
     * lock will be automatically released so that a new instance can run.
     * Recording the pid of the running instance is a good idea. It will
     * surely be efficient to make 'cat mydaamon.lock' instead of
     * 'ps -ef|grep mydaemon'
     */
    if (lockf(_lockFd, F_TLOCK, 0) < 0)
    {
        close(_lockFd);
        throw std::system_error(errno, std::system_category(),
                                "DaemonNix::daemonize: lockf");
    }

    char buff[16];
    snprintf(buff, sizeof(buff), "%d\n", getpid());
    write(_lockFd, buff, strlen(buff)); /* Record pid to lockfile */
}

void DaemonNix::shutdown()
{
    // Unlock and remove lock file.
    ::lockf(_lockFd, F_ULOCK, 0);
    ::close(_lockFd);
    ::remove(_config->_lockFilePath.c_str());
}
}

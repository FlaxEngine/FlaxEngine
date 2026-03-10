#!/bin/sh
# https://gist.github.com/ongardie/aa15f1f0d0e6b59890a9
gdb -q --batch \
               -ex 'handle SIGHUP nostop pass' \
               -ex 'handle SIGQUIT nostop pass' \
               -ex 'handle SIGPIPE nostop pass' \
               -ex 'handle SIGALRM nostop pass' \
               -ex 'handle SIGTERM nostop pass' \
               -ex 'handle SIGUSR1 nostop pass' \
               -ex 'handle SIGUSR2 nostop pass' \
               -ex 'handle SIGCHLD nostop pass' \
               -ex 'set print thread-events off' \
               -return-child-result \
               -ex 'run' \
               -ex 'thread apply all bt' \
               --tty=/dev/stdout \
               --args $*

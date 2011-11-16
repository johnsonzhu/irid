gcc -g -Wall main.c epoll_socket.c epoll_thread.c buff.c mlist.c lrumc.c mc_protocol.c config.c pary.c serialize.c rbtree.c assignment.c cli.c -lm  -o Irid  -l pthread && ./Irid >./too/1.txt
cd tool
./view_leak.sh


#ifndef NETBLOCKS_POSIX_IMPL_H
#define NETBLOCKS_POSIX_IMPL_H
#include <sys/socket.h>
// This header implements all the network functionality required by 
// posix style applications
// this calls into the NetBlocks generated code

// All the functions here have the nbp_ prefix
// We will use symbol renaming so that these don't conflict with existing 
// implementations


// these two functions are similar, except for the fact that 
// they assume different roles for the underlying communication interface
int nbp_init_server(const char* ip);
int nbp_init_client(const char* ip);

int nbp_socket (int domain, int type, int protocol);

int nbp_connect (int sockfd, const struct sockaddr* addr, socklen_t addrLen);

int nbp_accept(int sockfd, struct sockaddr *addr, socklen_t * addrlen);

int nbp_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int nbp_listen(int sockfd, int backlog);

ssize_t nbp_recv(int sockfd, char buf[], size_t len, int flags);

ssize_t nbp_send(int sockfd, const char buf[], size_t len, int flags);

int nbp_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

int nbp_ioctl(int fd, unsigned long request, char* arpg);

int nbp_close(int fd);

int nbp_pause(void);

int nbp_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

#endif
/* Darren Chauvet 12205390
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */

#ifndef USER_H
#define USER_H
#include <netinet/in.h>
#include <sys/socket.h>

struct user {
  char nickname[16];
  struct sockaddr *address;
  socklen_t addr_len;
  int sock;
  /* autres champs éventuels */
};

struct user *user_accept(int sl);
void user_free(struct user *user);

#endif /* ifndef USER_H */

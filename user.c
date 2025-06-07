/* Darren Chauvet 12205390
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */

#include "user.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h> // pour close()

/** accepter une connection TCP depuis la socket d'écoute sl et retourner un
 * pointeur vers un struct user, dynamiquement alloué et convenablement
 * initialisé */

struct user *user_accept(int sl) {
  struct user *usr = malloc(sizeof(struct user));
  if (!usr) {
    perror("malloc user");
    exit(EXIT_FAILURE);
  }

  usr->address = malloc(sizeof(struct sockaddr_in));
  if (!usr->address) {
    perror("malloc address");
    free(usr);
    exit(EXIT_FAILURE);
  }

  usr->addr_len =
      sizeof(struct sockaddr_in); // ici tu avais écrit `usr>addr_len` (faute de
                                  // syntaxe)

  int s = accept(sl, (struct sockaddr *)usr->address, &usr->addr_len);
  if (s < 0) {
    perror("accept");
    free(usr->address);
    free(usr);
    exit(EXIT_FAILURE);
  }

  usr->sock = s;

  return usr;
}

/** libérer toute la mémoire associée à user */
void user_free(struct user *user) {
  if (!user)
    return;

  if (user->sock >= 0) {
    close(user->sock); // Ferme la socket proprement
    user->sock = -1;
  }
  free(user->address);
  user->address = NULL;
  free(user);
}

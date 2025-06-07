/* Darren Chauvet 12205390
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */

#ifndef SERVEUR_H
#define SERVEUR_H

#include "list/list.h"
#include "user.h"
#include <pthread.h>
#include <stdint.h>

#define MAX_NICKNAME 16
#define PORT_FREESCORD 4321
#define MSG_SIZE 512
#define SELF_PREFIX "[vous] "
#define LOG_FILE "freescord.log"

extern int pipefd[2];
extern struct list *l;
extern pthread_mutex_t mut;
extern pthread_mutex_t log_mutex;

void log_message(const char *message);
void send_welcome_message(struct user *usr);
int handle_nickname(struct user *usr);
void *handle_client(void *user);
int create_listening_sock(uint16_t port);
void *repeater_thread(void *arg);

#endif // SERVEUR_H

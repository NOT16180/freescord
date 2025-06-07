/* Darren Chauvet 12205390
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */

#ifndef POINT_H
#define POINT_H

#include "buffer/buffer.h"
#include <stdint.h>
#include <sys/socket.h>

#define PORT_FREESCORD 4321
#define MSG_SIZE 512
#define CLEAR_LINE "\033[1A\033[K"
#define MAX_NICKNAME 16
#define COLOR_GRAY "\033[90m"
#define COLOR_RESET "\033[0m"
#define TABLE_FLIP "(╯°□°)╯︵ ┻━┻\n"
#define UNFLIP "┬─┬ノ( º _ ºノ)\n"
#define SHRUG "¯\\_(ツ)_/¯\n"

void process_special_commands(char *msj);
int handle_nickname_exchange(int sock, buffer *sock_buff);

int connect_serveur_tcp(char *adresse, uint16_t port);

int main(int argc, char *argv[]);

#endif /* POINT_H */

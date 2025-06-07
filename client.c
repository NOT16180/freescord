/* Darren Chauvet 12205390
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */

#include "client.h"
#include "utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

void process_special_commands(char *msj) {
  if (msj[0] != '/')
    return; // Pas une commande spéciale

  // Remplacement de commandes spéciales
  if (strncmp(msj, "/tableflip", 10) == 0) {
    strncpy(msj, TABLE_FLIP, MSG_SIZE);
  } else if (strncmp(msj, "/unflip", 7) == 0) {
    strncpy(msj, UNFLIP, MSG_SIZE);
  } else if (strncmp(msj, "/shrug", 6) == 0) {
    strncpy(msj, SHRUG, MSG_SIZE);
  }
}

int handle_nickname_exchange(int sock, buffer *sock_buff) {
  char command[MSG_SIZE];
  char response[MSG_SIZE];
  int valid = 0;

  while (!valid) {

    sock_buff->end = 0;

    if (fgets(command, sizeof(command), stdin) == NULL) {
      perror("Erreur lecture pseudonyme");

      return 0;
    }

    size_t len = strlen(command);
    if (len > 0 && command[len - 1] == '\n')
      command[len - 1] = '\0'; // Supprimer \n

    if (strchr(command, ':') != NULL) {
      printf("Erreur: ':' interdit\n");
      continue;
    }

    if (strlen(command) == 0) {
      printf("Erreur: vide\n");
      continue;
    }

    // Envoi du pseudo
    size_t total_sent = 0;
    while (total_sent < strlen(command)) {
      ssize_t sent =
          send(sock, command + total_sent, strlen(command) - total_sent, 0);
      if (sent == -1) {
        perror("send");
        return 0;
      }
      total_sent += sent;
    }

    // Lecture réponse serveur
    if (!buff_fgets_crlf(sock_buff, response, sizeof(response))) {
      if (buff_eof(sock_buff))
        fprintf(stderr, "Serveur fermé\n");
      else
        perror("réponse serveur");
      close(sock);
      buff_free(sock_buff);
      return EXIT_FAILURE;
    }

    printf("\033[2J\033[999H"); // Nettoyer écran
    fflush(stdout);

    switch (response[0]) {
    case '0':
      printf("Pseudonyme '%s' accepté\nVous pouvez commencer a parler\n",
             command + 10);
      valid = 1;
      return 1;
    case '1':
      printf("Erreur: pseudo déjà utilisé\n");
      break;
    case '2':
      printf("Erreur: pseudo interdit\n");
      break;
    case '3':
      fprintf(stderr, "Erreur format commande\n");
      break;
    default:
      fprintf(stderr, "Réponse inconnue: %s\n", response);
      break;
    }
  }

  printf("Trop d'échecs. Abandon.\n");
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <IP>\n", argv[0]);
    return EXIT_FAILURE;
  }

  printf("Connexion à %s:%d...\n", argv[1], PORT_FREESCORD);
  int sock = connect_serveur_tcp(argv[1], PORT_FREESCORD);
  if (sock < 0)
    perror_exit("connect_serveur_tcp");

  printf("Connecté.\n");

  int std = STDIN_FILENO;
  char msj[MSG_SIZE];
  buffer *buff = buff_create(std, MSG_SIZE);
  buffer *sock_buff = buff_create(sock, MSG_SIZE);

  // Lecture du message de bienvenue
  char welcome_msg[2048];
  if (!buff_fgets_crlf(sock_buff, welcome_msg, sizeof(welcome_msg))) {
    fprintf(stderr, "Pas de message de bienvenue\n");
    exit(EXIT_FAILURE);
  }

  char *clean_msg = crlf_to_lf(welcome_msg);
  if (clean_msg) {
    printf("%s\n", clean_msg);
    free(clean_msg);
  } else {
    printf("%s\n", welcome_msg);
  }

  if (!handle_nickname_exchange(sock, sock_buff)) {
    close(sock);
    buff_free(buff);
    buff_free(sock_buff);
    return EXIT_FAILURE;
  }

  struct pollfd fds[2] = {{.fd = std, .events = POLLIN},
                          {.fd = sock, .events = POLLIN}};

  int nb_ouverts = 2;

  while (nb_ouverts > 0 && poll(fds, 2, -1) > 0) {
    if (fds[0].revents & (POLLIN | POLLHUP)) {
      if (buff_fgets(buff, msj, sizeof(msj)) == NULL) {
        printf("\nEntrée fermée (Ctrl-D). Fin.\n");
        break;
      }

      printf(CLEAR_LINE); // Nettoyer ligne terminal
      fflush(stdout);

      process_special_commands(msj); // Emojis spéciaux
      char *crlfmsj = lf_to_crlf(msj);
      if (crlfmsj) {
        size_t total_sent = 0;
        while (total_sent < strlen(crlfmsj)) {
          ssize_t sent = send(sock, crlfmsj + total_sent,
                              strlen(crlfmsj) - total_sent, MSG_NOSIGNAL);
          if (sent == -1) {
            perror("send");
            break;
          }
          total_sent += sent;
        }
        free(crlfmsj);
      } else {
        fprintf(stderr, "Erreur LF->CRLF\n");
      }
    }

    if (fds[1].revents & (POLLIN | POLLHUP)) {
      char rep[MSG_SIZE];
      if (!buff_fgets_crlf(sock_buff, rep, sizeof(rep))) {
        if (buff_eof(sock_buff))
          printf("Serveur fermé.\n");
        else
          perror_exit("erreur lecture");
        nb_ouverts--;
        continue;
      }

      if (strlen(rep) == 0 ||
          (strlen(rep) <= 2 && (rep[0] == '\r' || rep[0] == '\n')))
        continue; // Ignorer lignes vides

      char *lfrep = crlf_to_lf(rep);
      if (lfrep) {
        if (strstr(lfrep, "[vous]") != NULL)
          printf(COLOR_GRAY "%s" COLOR_RESET "\n", lfrep); // Message personnel
        else
          printf("%s\n", lfrep); // Message normal
        free(lfrep);
        fflush(stdout);
      } else {
        fprintf(stderr, "Erreur CRLF->LF\n");
      }
    }
  }

  printf("Déconnexion...\n");
  close(sock);
  buff_free(buff);
  buff_free(sock_buff);

  return EXIT_SUCCESS;
}

int connect_serveur_tcp(char *adresse, uint16_t port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0); // Crée socket TCP
  if (sock < 0) {
    perror("socket");
    return -1;
  }

  // Timeout de 5 secondes
  struct timeval timeout = {.tv_sec = 5, .tv_usec = 0};

  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  struct sockaddr_in sa = {.sin_family = AF_INET, .sin_port = htons(port)};
  if (inet_pton(AF_INET, adresse, &sa.sin_addr) != 1) {
    perror("inet_pton");
    close(sock);
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
    perror("connect");
    close(sock);
    return -1;
  }

  return sock;
}

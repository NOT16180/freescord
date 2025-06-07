
/* Darren Chauvet 12205390
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */
#include "serveur.h"
#include "utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

int pipefd[2];             // Pipe pour communication inter-threads
struct list *l;            // Liste des utilisateurs connectés
pthread_mutex_t mut;       // Mutex pour accès liste utilisateurs
pthread_mutex_t log_mutex; // Mutex pour accès fichier log

// Log thread-safe
void log_message(const char *message) {
  time_t now;
  time(&now);
  char *time_str = ctime(&now);
  time_str[strlen(time_str) - 1] = '\0'; // Supprime '\n'

  pthread_mutex_lock(&log_mutex);

  FILE *log_file = fopen(LOG_FILE, "a");
  if (log_file) {
    fprintf(log_file, "[%s] %s\n", time_str, message);
    fclose(log_file);
  }

  printf("[%s] %s\n", time_str, message);
  pthread_mutex_unlock(&log_mutex);
}

void send_welcome_message(struct user *usr);
int handle_nickname(struct user *usr);
void *handle_client(void *user);
int create_listening_sock(uint16_t port);
void *repeater_thread(void *user);

int main() {
  pthread_mutex_init(&log_mutex, NULL);
  log_message("[SERVER] Démarrage du serveur Freescord");

  int sock_l = create_listening_sock(PORT_FREESCORD); // Création socket écoute
  if (pipe(pipefd)) {
    perror_exit("pipe");
  }

  pthread_mutex_init(&mut, NULL);
  l = list_create(); // Initialisation liste utilisateurs

  pthread_t rptr;
  pthread_create(&rptr, NULL, repeater_thread, NULL); // Thread de diffusion
  pthread_detach(rptr);

  char log_msg[100];
  snprintf(log_msg, sizeof(log_msg), "[SERVER] En écoute sur le port %d",
           PORT_FREESCORD);
  log_message(log_msg);

  while (1) {
    struct user *usr = user_accept(sock_l); // Acceptation client
    if (!usr)
      continue;

    pthread_t th;
    if (pthread_create(&th, NULL, (void *(*)(void *))handle_client, usr)) {
      log_message("[ERREUR] Échec de création du thread client");
      user_free(usr);
    }
    pthread_detach(th);
  }

  close(sock_l);
  list_free(l, (void (*)(void *))user_free);
  pthread_mutex_destroy(&log_mutex);
  log_message("[SERVER] Arrêt du serveur");
  return 0;
}

// Gestion client dans un thread
void *handle_client(void *user) {
  struct user *usr = (struct user *)user;
  char rep[MSG_SIZE];
  ssize_t rd;
  char log_msg[MSG_SIZE + 50];

  send_welcome_message(usr);

  if (!handle_nickname(usr)) { // Attribution pseudo
    snprintf(log_msg, sizeof(log_msg),
             "[CLIENT] Échec de l'attribution du pseudo pour ");
    log_message(log_msg);
    close(usr->sock);
    user_free(usr);
    return NULL;
  }

  list_add(l, usr); // Ajout à la liste
  snprintf(log_msg, sizeof(log_msg), "[CLIENT] %s connecté ", usr->nickname);
  log_message(log_msg);

  char connect_msg[MSG_SIZE];
  snprintf(connect_msg, sizeof(connect_msg), "Utilisateur %s a rejoint!\r\n",
           usr->nickname);
  write(pipefd[1], connect_msg, strlen(connect_msg)); // Notification

  while (1) {
    rd = recv(usr->sock, rep, sizeof(rep) - 1, 0);
    if (rd <= 0) {
      if (rd == 0) {
        snprintf(log_msg, sizeof(log_msg), "[CLIENT] %s déconnecté",
                 usr->nickname);
        log_message(log_msg);
      } else if (errno != EAGAIN) {
        snprintf(log_msg, sizeof(log_msg), "[ERREUR] recv: %s",
                 strerror(errno));
        log_message(log_msg);
      }
      break;
    }
    rep[rd] = '\0';

    if (strcmp(rep, "\r\n") == 0)
      continue;

    snprintf(log_msg, sizeof(log_msg), "[MESSAGE] [%s] %s", usr->nickname, rep);
    log_message(log_msg);

    // Envoi message à l'expéditeur
    char self_msg[MSG_SIZE + sizeof(SELF_PREFIX)];
    snprintf(self_msg, sizeof(self_msg), "%s%s", SELF_PREFIX, rep);
    send(usr->sock, self_msg, strlen(self_msg), MSG_NOSIGNAL);

    // Diffusion message aux autres
    char broadcast_msg[MSG_SIZE + MAX_NICKNAME + 4];
    snprintf(broadcast_msg, sizeof(broadcast_msg), "[%s] %s", usr->nickname,
             rep);
    pthread_mutex_lock(&mut);
    write(pipefd[1], broadcast_msg, strlen(broadcast_msg));
    pthread_mutex_unlock(&mut);
  }

  // Déconnexion
  char disconnect_msg[MSG_SIZE];
  snprintf(disconnect_msg, sizeof(disconnect_msg),
           "Utilisateur %s a quitté!\r\n", usr->nickname);
  list_remove_element(l, usr);
  write(pipefd[1], disconnect_msg, strlen(disconnect_msg));

  close(usr->sock);
  user_free(usr);
  return NULL;
}

// Création socket d'écoute
int create_listening_sock(uint16_t port) {
  int sock_l = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_l < 0)
    perror_exit("socket");

  int opt = 1;
  setsockopt(sock_l, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in sa = {.sin_family = AF_INET,
                           .sin_port = htons(port),
                           .sin_addr.s_addr = htonl(INADDR_ANY)};

  if (bind(sock_l, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    perror_exit("bind");

  listen(sock_l, 128);
  return sock_l;
}

// Thread de diffusion des messages
void *repeater_thread(void *arg) {
  char buf[MSG_SIZE];
  char log_msg[MSG_SIZE + 20];

  while (1) {
    ssize_t rd = read(pipefd[0], buf, sizeof(buf) - 1);
    if (rd <= 0)
      continue;
    buf[rd] = '\0';

    pthread_mutex_lock(&mut);
    struct node *curr = l->first;
    while (curr) {
      struct user *usr = curr->elt;

      char prefix[MAX_NICKNAME + 3]; // "[pseudo] "
      snprintf(prefix, sizeof(prefix), "[%s]", usr->nickname);

      if (strncmp(buf, prefix, strlen(prefix)) != 0) {
        if (send(usr->sock, buf, strlen(buf), MSG_NOSIGNAL) < 0) {
          snprintf(log_msg, sizeof(log_msg), "[ERREUR] send: %s",
                   strerror(errno));
          log_message(log_msg);
        }
      }

      curr = curr->next;
    }
    pthread_mutex_unlock(&mut);
  }
  return NULL;
}

// Message d'accueil envoyé au client
void send_welcome_message(struct user *usr) {
  const char *welcome_msg =
      " \n\n"
      "  ╔═════════════════════════════════════════════╗\n"
      "  ║  ██████ ╗                                   ║\n"
      "  ║  ██╔════╝                                   ║\n"
      "  ║  ██████╗   ██████╗  ██████╗                 ║\n"
      "  ║  ██╔═══╝   ╚════██╗ ╚════██╗                ║\n"
      "  ║  ██        ██████╔╝ ██████╔╝                ║\n"
      "  ║  ╚═╝       ╚═════╝  ╚═════╝                 ║\n"
      "  ║  Freescord en ligne.                        ║\n"
      "  ║  Ton lien vers le flux néon du net.         ║\n"
      "  ║  Branche-toi. Prends ton pseudo.            ║\n"
      "  ║  Pas de corps, pas de nom. Juste des ombres ║\n"
      "  ║  et du code.                                ║\n"
      "  ╚═════════════════════════════════════════════╝\n"
      "  /nickname <pseudo> pour t'3r3gistr3r\n"
      "\n\r\n";
  size_t total = 0;
  size_t len = strlen(welcome_msg);

  while (total < len) {
    ssize_t sent =
        send(usr->sock, welcome_msg + total, len - total, MSG_NOSIGNAL);
    if (sent <= 0) {
      char log_msg[100];
      snprintf(log_msg, sizeof(log_msg),
               "[ERREUR] Échec d'envoi du message de bienvenue ");
      log_message(log_msg);
      return;
    }
    total += sent;
  }
}

// Gestion du pseudo utilisateur
int handle_nickname(struct user *usr) {
  char input[MSG_SIZE];
  char nickname[MAX_NICKNAME + 1];
  int valid = 0;
  char log_msg[100];

  while (!valid) {
    ssize_t received = recv(usr->sock, input, sizeof(input) - 1, 0);
    if (received <= 0) {
      snprintf(log_msg, sizeof(log_msg),
               "[CLIENT] Déconnexion pendant la sélection du pseudo ");
      log_message(log_msg);
      return 0;
    }
    input[received] = '\0';

    if (strncmp(input, "/nickname ", 10) != 0) {
      const char *msg = "3\r\n"; // commande invalide
      send(usr->sock, msg, strlen(msg), 0);
      continue;
    }

    strncpy(nickname, input + 10, MAX_NICKNAME);
    nickname[MAX_NICKNAME] = '\0';

    // Trim fin de chaîne
    char *end = nickname + strlen(nickname) - 1;
    while (end >= nickname && (*end == ' ' || *end == '\t'))
      end--;
    *(end + 1) = '\0';

    char msg[4] = "0\r\n";

    if (strlen(nickname) == 0 || strlen(nickname) > MAX_NICKNAME ||
        strchr(nickname, ':')) {
      strcpy(msg, "2\r\n"); // pseudo invalide
      send(usr->sock, msg, strlen(msg), 0);
      continue;
    }

    pthread_mutex_lock(&mut);
    struct node *curr = l->first;
    int exists = 0;
    while (curr && !exists) {
      exists = (strcmp(((struct user *)curr->elt)->nickname, nickname) == 0);
      curr = curr->next;
    }
    pthread_mutex_unlock(&mut);

    if (exists) {
      strcpy(msg, "1\r\n"); // pseudo déjà pris
      send(usr->sock, msg, strlen(msg), 0);
      continue;
    }

    strcpy(usr->nickname, nickname);
    send(usr->sock, msg, strlen(msg), 0);
    valid = 1;
  }

  snprintf(log_msg, sizeof(log_msg), "[CLIENT] Pseudo attribué: %s ",
           usr->nickname);
  log_message(log_msg);
  return 1;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>

#define TAILLEBUF 1024
#define SIZE 1024
#define MAX_CLIENTS 100
#define FIFO_SEND "/tmp/myfifo"
#define FIFO_RECV "/tmp/fifo_recv"

typedef struct {
    int socket;
    char username[50];
} User; //cette structure sert à identifier l'utilisateur par le username et la socket
typedef struct {
    User users[MAX_CLIENTS]; // MAX_CLIENTS est le nombre maximum d'utilisateurs que vous voulez stocker
    sem_t *sem;
} SharedMemory; // cette socket nous
int sock_udp;
SharedMemory *sharedMemory;

void displayUsers();

static int udp_port;
static char *nom;

// Thread pour envoyer des messages via UDP
void *udp_sender(void *arg) {
    char buffer[TAILLEBUF];
    int nb_octets;
    int fd_read = open(FIFO_SEND, O_RDONLY);
    struct sockaddr_in addr_serveur_udp;
    struct hostent *serveur_host = gethostbyname(nom);
    addr_serveur_udp.sin_family = AF_INET;
    addr_serveur_udp.sin_port = htons(udp_port);
    memcpy(&addr_serveur_udp.sin_addr, serveur_host->h_addr, serveur_host->h_length);
    while (1) {

        if ((nb_octets = read(fd_read, buffer, TAILLEBUF)) > 0) {
            printf("envoyer d'un client: %s\n", buffer);
            sendto(sock_udp, buffer, nb_octets, 0, (struct sockaddr *) &addr_serveur_udp, sizeof(addr_serveur_udp));
        }
    }
    return NULL;
}

// Thread pour gérer les réponses UDP
void *udp_response_handler() {

    fprintf(stderr, "udp_response_handler\n");

    char buffer[TAILLEBUF];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int nb_octets, i = 0;

    char *tokens[3];  // Corrected declaration

    char client_message[TAILLEBUF];

    int fd_read = open(FIFO_RECV, O_WRONLY);


    while (1) {
        fprintf(stderr, "jaii\n");
        nb_octets = recvfrom(sock_udp, buffer, SIZE, 0, (struct sockaddr *) &client_addr, &addr_len);
        printf("Reçu d'un client: %s\n", buffer);
        if (nb_octets < 0) {
            perror("Erreur recvfrom UDP");
            continue;
        }
        write(fd_read, buffer, nb_octets); // Write response to the response pipe
        char *token = strtok(buffer, ";");
        if (token != NULL) {
            // Boucle pour extraire et stocker tous les tokens
            while (token != NULL && i < 3) {  // Limite à 3 tokens pour éviter un débordement de tableau
                tokens[i++] = token;
                printf("sad %s:\n", tokens[i]);
                token = strtok(NULL, ";");
            }

            if (strcmp(tokens[1], "user est connecte") == 0) {
                sem_wait(sharedMemory->sem);
                if (sharedMemory == NULL) {
                    printf("sharedMemory is NULL\n");
                }
                if (sharedMemory->sem == SEM_FAILED) {
                    printf("sem_open failed\n");
                }

                i = 0;
                while (i < MAX_CLIENTS && sharedMemory->users[i].socket != 0) {
                    i++;
                }
                if (i < MAX_CLIENTS) {
                    sharedMemory->users[i].socket = atoi(tokens[0]);
                    strcpy(sharedMemory->users[i].username, tokens[2]);
                }
                sem_post(sharedMemory->sem);
            }
        }
        i = 0;
        //displayUsers();
    }

}

void displayUsers() {
    if (sharedMemory == NULL) {
        printf("sharedMemory is NULL\n");
        return; // or handle error as appropriate
    }
    if (sharedMemory->sem == SEM_FAILED) {
        printf("sem_open failed\n");
        return; // or handle error as appropriate
    }

    sem_wait(sharedMemory->sem);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sharedMemory->users[i].socket != 0) {
            printf("User %d:\n", i + 1);
            printf("Socket: %d\n", sharedMemory->users[i].socket);
            printf("Username: %s\n", sharedMemory->users[i].username);
        }
    }
    sem_post(sharedMemory->sem);
}


int main(int argc, char *argv[]) {
    struct sockaddr_in addr_client, addr_serveur;
    int socket_ecoute, socket_service;
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <Hostname> <Port>\n", argv[0]);
        exit(1);
    }
    udp_port = atoi(argv[2]);
    nom = argv[1];

    socklen_t lg_addr;
    if ((sock_udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ||
        (socket_ecoute = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erreur création socket");
        exit(EXIT_FAILURE);
    }
    mkfifo(FIFO_RECV, 0666);
    if (mkfifo(FIFO_RECV, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Erreur lors de la création de FIFO_RECV");
            exit(EXIT_FAILURE);
        }
    }
    int shmid;
    key_t key = 12134;  // Clé arbitraire, doit être le même pour tous les processus
    size_t shm_size = sizeof(SharedMemory);  // Taille du segment, dépend de votre structure

    shmid = shmget(key, shm_size, IPC_CREAT | 0666);
    if (shmid == -1) {
        if (errno == EEXIST) {
            shmid = shmget(key, shm_size, 0666);
            if (shmid == -1) {
                perror("shmget");
                exit(1);
            }
        } else {
            perror("shmget");
            exit(1);
        }
    }

    // Attacher la mémoire partagée
    sharedMemory = (SharedMemory *) shmat(shmid, NULL, 0);
    memset(sharedMemory, 0, sizeof(SharedMemory));
    if (sharedMemory == (SharedMemory *) -1) {
        perror("shmat");
        exit(1);
    }

    sharedMemory->sem = sem_open("smp", O_CREAT, 0666, 1);
    if (sharedMemory->sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    pthread_t thread_id, thread_udp, response_thread, tcp_response;
    // Démarrage des threads pour gérer UDP et TCP
    pthread_create(&response_thread, NULL, udp_response_handler, NULL);
    //pthread_create(&tcp_response, NULL, tcp_response_sender, NULL);
    pthread_create(&thread_udp, NULL, (void *) udp_sender, NULL);
    // Thread pour udp_sender // Attente de la fin des threads (ne se terminera pas naturellement)
    pthread_join(response_thread, NULL);
    //pthread_join(tcp_response, NULL);
    pthread_join(thread_udp, NULL);
    close(sock_udp);
    close(socket_ecoute);
    unlink(FIFO_SEND);
    unlink(FIFO_RECV);
    return 0;
}
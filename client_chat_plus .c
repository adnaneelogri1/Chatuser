#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include<stdbool.h>

#define BUFFER_SIZE 1024
#define FIFO_PATH "/tmp/chat_fifo"
#define FIFO_PATH2 "/tmp/myfifo2"
#define PROGRAMME "gestion"
static int udp_port;
static char *nom;
bool estConnecte = false;
// Fonction pour créer un compte
void creerCompte(int sock, int *choix) {
    char username[100];
    char password[100];
    int nb_octets;

    printf("\n\tVeuillez insérer le UserName: ");
    fgets(username, sizeof(username), stdin);  // Lire le nom d'utilisateur
    username[strcspn(username, "\n")] = 0;  // Enlever le newline

    printf("\n\tVeuillez insérer le Mot de Passe: ");
    fgets(password, sizeof(password), stdin);  // Lire le mot de passe
    password[strcspn(password, "\n")] = 0;  // Enlever le newline

    // Préparer le message à envoyer
    char message[256];
    if ((strlen(username) > 0 && strlen(username) <= 10) && (strlen(password) > 0 && strlen(password) <= 10)) {
        snprintf(message, sizeof(message), "%d;%s;%s", *choix, username, password);  // Format pour le serveur

        printf("\n\t\tLes informations de connexion sont valide !\n");
        // Envoyer le message au serveur
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Échec de l'envoi des données de création de compte");
        }
        //nb_octets = read(sock, message, BUFFER_SIZE);
        // printf(" reponse recue : %s\n", message);
        //  printf(" click sur entrer pour retourner  : ");

        // getchar(); // wait for ENTER
        sleep(2);
        printf(" \n\n \t\t **** Clique sur Entrer pour retourner à la table d'acceuil  : \n\n");

        getchar(); // wait for ENTER
        printf("\033c");//remplace 'estConnectear' en terminal
    } else
        printf("\n\n\t\tLes informations de connexion ne sont pas valides \n\t\t( Le mot de passe et le UserName doivent etre  \e[1minferieur ou égale à 10 \t et Supérieur à 0 !)\e[0m\n");
    printf("\033c");//remplace 'estConnectear' en terminal
    *choix = 6;

}
// Fonction pour recevoir les messages du serveur
void *receiveMessages(void *socket_desc) {
    int sock = *(int *) socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;
    mkfifo(FIFO_PATH, 0666);
    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    //int fifo_fd_sys = open(FIFO_PATH2, O_WRONLY);

    if (fifo_fd == -1) {
        perror("Could not open FIFO for writing");
        return NULL;
    }

    while ((read_size = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        char delim[] = ";";     // Délimiteur utilisé pour séparer les tokens
        buffer[read_size] = '\0';
        // Extraire le premier token en utilisant le délimiteur ";"
        char *token = strtok(buffer, delim);

        // Vérifier si le premier token est "M"
        if (token != NULL && strcmp(token, "M") == 0) {
            token = strtok(NULL, delim);
            write(fifo_fd, token, strlen(token)); // Envoyer au processus d'affichage
        } else {
            if (strcmp(buffer, "user est connecte") == 0) {
                estConnecte = true;
            } else if (strcmp(buffer, "user est deconnecte") == 0) {
                estConnecte = false;
            }
            printf(" reponse recue : %s\n", buffer);

        }
        //printf("Received: %s\n", buffer);
    }

    close(fifo_fd);
    //close(sock);
    //free(socket_desc);
    return NULL;
}

int main(int argc, char *argv[]) {
    int sock, choice;
    struct sockaddr_in server;
    char message[BUFFER_SIZE];
    pthread_t thread_id;
    char *fifo_path = FIFO_PATH, *fifo_path2 = FIFO_PATH2, *programme = PROGRAMME;
    int choix, pid_;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <Hostname> <Port>\n", argv[0]);
        exit(1);
    }
    udp_port = atoi(argv[2]);
    nom = argv[1];
// identification socket d'écoute du serveur
    static struct sockaddr_in addr_serveur;
// identifiants de la machine où tourne le serveur
    struct hostent *host_serveur;
    //sprintf(fifo_path, "/tmp/chat_fifo");
    //sprintf(fifo_path2, "/tmp/myfifo2");
    pid_ = getpid();
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        return 1;
    }
// récupération identifiants de la machine serveur
//host_serveur = gethostbyname("scinfe059");
    host_serveur = gethostbyname(nom);

    if (host_serveur == NULL) {
        perror("erreur récupération adresse serveur\n");
        exit(1);
    }
// création de l'identifiant de la socket d'écoute du

    bzero((char *) &addr_serveur, sizeof(addr_serveur));
    addr_serveur.sin_family = AF_INET;
    addr_serveur.sin_port = htons(udp_port);
    memcpy(&addr_serveur.sin_addr.s_addr,
           host_serveur->h_addr,
           host_serveur->h_length);
// connexion de la socket client locale à la socket coté

    if (connect(sock, (struct sockaddr *) &addr_serveur,
                sizeof(struct sockaddr_in)) == -1) {
        perror("erreur connexion serveur");
        exit(1);
    }


    // Forking to launch affichage_message
    pid_t pid = fork();
    if (pthread_create(&thread_id, NULL, receiveMessages, (void *) &sock) < 0) {
        perror("could not create thread");
        return 1;
    }
    if (pid == 0) {
        // Child process
        close(STDOUT_FILENO);  // Close the standard output
        execlp("gnome-terminal", "gnome-terminal", "--", "./affiche", fifo_path, NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
    }
    /* pid_t pid1 = fork();
      if (pthread_create(&thread_id, NULL, receiveMessages, (void*)&sock) < 0) {
         perror("could not create thread");
         return 1;
     }
     if (pid1 == 0) {
         // Child process
         close(STDOUT_FILENO);  // Close the standard output
         execlp("gnome-terminal", "gnome-terminal", "--", "./affichage_plus",fifo_path2, NULL);
         perror("execl failed");
         exit(EXIT_FAILURE);
     }
     pid_t pid2= fork();
     if (pid2 == 0) {
         // Child process
         close(STDOUT_FILENO);  // Close the standard output
         execlp("gnome-terminal", "gnome-terminal", "--", programme, NULL);
         perror("execl failed");
         exit(EXIT_FAILURE);
     }*/


    printf("\033c");
    do {
        printf("\n\t1. Se connecter\n\n\t2. Se déconnecter\n\n\t3. Afficher la liste de tous les utilisateurs\n\n\t4. Créer un nouveau compte\n\n\t5. Supprimer un compte\n\n\t6. Envoyer un message\n\n\t7. Quitter\n\n\t\t |- C’est à vous de choisir maintenant: ");
        scanf("%d", &choice);
        getchar(); // catch newline

        switch (choice) {
            case 1:
                if (estConnecte == true) {
                    printf("\n\n\n\t\t\t| Deja connecte: |\n\n\n");
                    sleep(1);

                } else {
                    printf("\n\n\n\t\t\t| Veuillez enter vos informations: |\n\n\n");
                    creerCompte(sock, &choice);
                }
                printf("\033c");//remplace 'estConnectear' en terminal
                choix = 6;
                break;
            case 2:
                if (estConnecte == false) {
                    printf("\n\n\n\t\t\t| Deja Deconnecte: |\n\n\n");
                    sleep(2);

                } else {
                 char number_str[10];  // Assurez-vous que le buffer est assez grand
                sprintf(number_str, "%d", 2);  // Convertit l'entier en chaîne de caractères
                send(sock, number_str, strlen(number_str), 0);  // Envoie la chaîne
                printf("\n\n\n\t\t\ttDeconnexion avec succes \n");
                }
                printf("\033c");//remplace 'estConnectear' en terminal
                choix = 6;
                break;

            case 3:
                printf("Voici la liste de tous les utilisateurs :\n");
                choix = 3;
                char number_str[10];  // Assurez-vous que le buffer est assez grand
                sprintf(number_str, "%d", 3);  // Convertit l'entier en chaîne de caractères
                int send_result = send(sock, number_str, strlen(number_str), 0);  // Envoie la chaîne
                //   printf(" reponse recue : %s\n", message);
                sleep(2);
                printf(" click sur entrer pour retourner  : ");
                getchar(); // wait for ENTER
                printf("\033c");//remplace 'estConnectear' en terminal
                choix = 6;
                break;
            case 4:
                if (estConnecte == true) {
                    printf("\n\n\n\t\t\t| Deja connecte: |\n\n\n");
                    sleep(1);


                } else {
                    printf("\n\n\n\t\t\t| Veuillez se connecter d'abord : |\n\n\n");
                    choix = 4;
                    creerCompte(sock, &choix);
                }
                printf("\033c");//remplace 'estConnectear' en terminal
                choix = 6;
                break;
            case 5:
                if (estConnecte == false) {
                    printf("\n\n\n\t\t\t| Veuillez se connecter d'abord : |\n\n\n");
                    sleep(1);

                } else {
                    printf("\n\n\n\t\t\t| Veuillez enter vos informations: |\n\n\n");
                    choix = 5;
                    creerCompte(sock, &choix);
                }
                printf("\033c");//remplace 'estConnectear' en terminal
                choix = 6;
                break;
            case 6:
                if (estConnecte == false) {
                    printf("\n\n\n\t\t\t| Veuillez se connecter d'abord  |\n\n\n");
                    sleep(1);


                } else {
                    printf("\t Enter message: ");
                    fgets(message, BUFFER_SIZE, stdin);
                    //send(sock, &pid_, sizeof(int), 0);
                    printf("content : %s \n", message);
                    send(sock, message, strlen(message), 0);
                }
                printf("\033c");//remplace 'estConnectear' en terminal
                choix = 6;
                break;
            case 7:
                number_str[10];  // Assurez-vous que le buffer est assez grand
                sprintf(number_str, "%d", 2);  // Convertit l'entier en chaîne de caractères
                send_result = send(sock, number_str, strlen(number_str), 0);  // Envoie la chaîne
                printf("\n\n\n\t\t\ttDeconnexion avec succes \n");
                return 0;
            default:
                // Handle other choices
                break;
        }
    } while (choix == 6);

    pthread_join(thread_id, NULL);
    return 0;
}

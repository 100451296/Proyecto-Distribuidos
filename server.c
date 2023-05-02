#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "manage.h"

pthread_mutex_t mutex_socket = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_users = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_copied = PTHREAD_COND_INITIALIZER;
int copied = 0;
int num_users;
User **users;
struct sockaddr_in client_addr;

void manage_client(int *sc){
        int sc_copied;
        char buffer[MAX_LINE_LENGTH];
        char **peticion;
        char client_ip[INET_ADDRSTRLEN];

        pthread_mutex_lock(&mutex_socket);
        sc_copied = *sc; 
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN); 
        copied = 1;
        pthread_cond_signal(&cond_copied);
        pthread_mutex_unlock(&mutex_socket);

        printf("Cliente conectado\n");
        // Comprobacion socket
	if (sc_copied == -1){
                perror("El socket no se pudo abrir correctamente\n");
	}

        printf("La dirección del cliente es %s\n", client_ip);
	// Pone la zona de memoria del buffer todo a 0
	memset(buffer, 0, sizeof(buffer));

	// Realiza la recepción de la petición en el formato esperado
	recv(sc_copied, buffer, MAX_LINE_LENGTH, 0);

        // Trocea la peticion y reinicia buffer
        peticion = split_fields(buffer);

        // Inicio seccion critica para acceder a users
        pthread_mutex_lock(&mutex_users);

        //Tratamiento peticion REGISTER
        if (strcmp(peticion[0],"REGISTER") == 0){
                
                if (registered(peticion[REGISTER_USERNAME], peticion[REGISTER_ALIAS], users, num_users) == 1){
                        sprintf(buffer, "%d", 1);
                } // Usuario registrado
        
               else{
                        add_user(peticion[REGISTER_USERNAME],
                                peticion[REGISTER_ALIAS],
                                peticion[REGISTER_DATE], users, &num_users);

                        sprintf(buffer, "%d", 0);
                } // No se encontro un usuario igual

                send(sc_copied, buffer, MAX_LINE_LENGTH, MSG_WAITALL);
        }

        else if (strcmp(peticion[0], "UNREGISTER") == 0){
                if (registered(peticion[REGISTER_USERNAME], peticion[REGISTER_ALIAS], users, num_users) == 0){
                        sprintf(buffer, "%d", 1);
                } // Usuario no registrado
        
               else{
                        remove_user(peticion[REGISTER_USERNAME], users, &num_users);
                        sprintf(buffer, "%d", 0);
                } // Usuario borrado

                send(sc_copied, buffer, MAX_LINE_LENGTH, MSG_WAITALL);
        }

        else if (strcmp(peticion[0], "CONNECT") == 0){
               sprintf(buffer, "%d", 
                fill_connected(peticion[CONNECTED_ALIAS], client_ip, peticion[CONNECTED_PORT], users, num_users));
                send(sc_copied, buffer, MAX_LINE_LENGTH, MSG_WAITALL);
        }

        pthread_mutex_unlock(&mutex_users); // Fin sección critica users

        close(sc_copied);
        pthread_exit(0);
}

int main(int argc, char *argv[])
{
        int val;
        int err;
        int sd, sc;
        socklen_t size;
        struct sockaddr_in server_addr;
        pthread_t thid;
        pthread_attr_t t_attr;

        users = read_users_from_file(&num_users);

        // Inicializa el socket del servidor con el tipo de conexion
        sd =  socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sd < 0) {
                perror("Error in socket");
                exit(1);
        }

        // Configura opciones del socket
        val = 1;
        err = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(int));
        if (err < 0) {
                perror("Error in opction");
                exit(1);
        }

        // Inicializa la estrcutura de direccion para el servidor
        bzero((char *)&server_addr, sizeof(server_addr));
        server_addr.sin_family      = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port        = htons(4200);

        // Asigna la dirección y puerto al socket del servidor
        err = bind(sd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
        if (err == -1) {
                printf("Error en bind\n");
                return -1;
        }

        // Se conifguran los atirbutos del hilo para que no tengan que esperar
        pthread_attr_init(&t_attr);
        pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);
        while (1){
                // Pone al socket en espera de recibir conexiones
                err = listen(sd, SOMAXCONN);
                if (err == -1) {
                        printf("Error en listen\n");
                        return -1;
                }

                size = sizeof(client_addr);
                
                // Realiza la conexion antes de entrar al bucle
                printf("esperando conexion\n");
                sc = accept(sd, (struct sockaddr *)&client_addr, (socklen_t *)&size);
                if (sc == -1) {
                        printf("Error en accept\n");
                        return -1;
                }
                printf("***********\nconexión aceptada de IP: %s   Puerto: %d\n***********\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                if (pthread_create(&thid, &t_attr, (void *)manage_client, (void *)&sc) == 0){
                        pthread_mutex_lock(&mutex_socket);
                        while (copied == 0){
                                pthread_cond_wait(&cond_copied, &mutex_socket);
                        }
                        copied = 0;
                        pthread_mutex_unlock(&mutex_socket);

                }
        }

        close (sd);

        free_user_array(users, num_users);

        return (0);        
}


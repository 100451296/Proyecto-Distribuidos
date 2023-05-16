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
#include <netdb.h>

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
        char **peticion = NULL;
        int num_peticion = 0;
        int i = 0;
        unsigned int id;
        char send_buffer[MAX_LINE_LENGTH];
        char *ip = NULL;
        char *port = NULL;
        int err;
        char *client_ip;
        int num_connected;
        char** connected_alias = NULL;

        pthread_mutex_lock(&mutex_socket);
        sc_copied = *sc; 
        client_ip = strdup(inet_ntoa(client_addr.sin_addr));
        copied = 1;
        pthread_cond_signal(&cond_copied);
        pthread_mutex_unlock(&mutex_socket);

        printf("Cliente conectado\n");
        // Comprobacion socket
	if (sc_copied == -1){
                perror("El socket no se pudo abrir correctamente\n");
	}

	// Pone la zona de memoria del buffer todo a 0
	memset(buffer, 0, sizeof(buffer));

	// Realiza la recepción del codigo de operacion
	recv(sc_copied, buffer, MAX_LINE_LENGTH, 0);

        // Envía una confirmación al cliente
       send(sc_copied, "OK\0", strlen("OK\0"), MSG_WAITALL);

        //TO DO: Funcion para comprobar que el formato de la peticion sea valido

        // Trocea la peticion y reinicia buffer
        agregar_string(&peticion, &num_peticion, buffer);

        if (peticion == NULL){
                printf("Se envió un petición vacía");
                close(sc_copied);
                pthread_exit(0);
        }

        // Inicio seccion critica para acceder a users
        pthread_mutex_lock(&mutex_users);

        //Tratamiento peticion REGISTER
        if (strcmp(peticion[0],"REGISTER") == 0){
                // Pone la zona de memoria del buffer todo a 0 y recibe
                for (i = 0; i < NUM_REGISTER; i++){
                        // Reinicia buffer para recibir
                        memset(buffer, 0, sizeof(buffer));

                        // Recibe dato
                        recv(sc_copied, buffer, MAX_LINE_LENGTH, 0);
                        // Agrega el dato a la peticion (lista de strings)
                        agregar_string(&peticion, &num_peticion, buffer);
                        // Manda la confirmacion
                        send(sc_copied, "OK\0", strlen("OK\0"), MSG_NOSIGNAL);
                }
                
                if (registered(peticion[REGISTER_ALIAS], users, num_users) == 1){
                        sprintf(buffer, "%d", 1);
                } // Usuario registrado
        
               else{
                        add_user(peticion[REGISTER_USERNAME],
                                peticion[REGISTER_ALIAS],
                                peticion[REGISTER_DATE], users, &num_users);

                        createPendingFile(peticion[REGISTER_ALIAS]);
                        
                        sprintf(buffer, "%d", 0);
                } // No se encontro un usuario igual
                memset(send_buffer, 0, sizeof(buffer));
                recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);

                buffer[strlen(buffer)] = '\0';
                send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);
        }

        else if (strcmp(peticion[0], "UNREGISTER") == 0){

                for (i = 0; i < NUM_UNREGISTER; i++){
                        // Reinicia buffer para recibir
                        memset(buffer, 0, sizeof(buffer));

                        // Recibe dato
                        recv(sc_copied, buffer, MAX_LINE_LENGTH, 0);
                        // Agrega el dato a la peticion (lista de strings)
                        agregar_string(&peticion, &num_peticion, buffer);
                        // Manda la confirmacion
                       send(sc_copied, "OK\0", strlen("OK\0"), MSG_WAITALL);
                }

                if (registered(peticion[UNREGISTER_ALIAS], users, num_users) == 0){
                        sprintf(buffer, "%d", 1);
                } // Usuario no registrado
        
               else{
                        remove_user(peticion[UNREGISTER_ALIAS], users, &num_users);
                        deletePendingFile(peticion[UNREGISTER_ALIAS]);
                        sprintf(buffer, "%d", 0);

                        //deletePendingFile(peticion[REGISTER_ALIAS]);
                } // Usuario borrado
                memset(send_buffer, 0, sizeof(buffer));
                recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);

                buffer[strlen(buffer)] = '\0';
                send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);
        }

        else if (strcmp(peticion[0], "CONNECT") == 0){
                for (i = 0; i < NUM_CONNECT; i++){
                        // Reinicia buffer para recibir
                        memset(buffer, 0, sizeof(buffer));

                        // Recibe dato
                        recv(sc_copied, buffer, MAX_LINE_LENGTH, 0);
                        // Agrega el dato a la peticion (lista de strings)
                        agregar_string(&peticion, &num_peticion, buffer);
                        // Manda la confirmacion
                       send(sc_copied, "OK\0", strlen("OK\0"), MSG_WAITALL);
                }
                err = fill_connection(peticion[CONNECTED_ALIAS], client_ip, peticion[CONNECTED_PORT], 
                                users, num_users, CONNECTED);
                sprintf(buffer, "%d", err);

                switch(err){
                        case 0: 
                                printf("s> CONNECT %s OK\n", peticion[CONNECTED_ALIAS]);
                                
                                err = 0;
                                while(err == 0){
                                        getUserPortIP(peticion[CONNECTED_ALIAS], &ip, &port, users, num_users);
                                        sendMessage(ip, port, peticion[CONNECTED_ALIAS]);
                                        err = borrarUltimaLinea(peticion[CONNECTED_ALIAS]);
                                }
                                break;
                        case 1:
                                printf("s> CONNECT %s FAIL\n", peticion[CONNECTED_ALIAS]);
                                break;
                        default:
                                break;
                }
                memset(send_buffer, 0, sizeof(buffer));
                recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);

                buffer[strlen(buffer)] = '\0';
                send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);

                // TO DO: Enviar todos los mensajes pendientes 
        }

        else if (strcmp(peticion[0], "DISCONNECT") == 0){
                for (i = 0; i < NUM_DISCONNECT; i++){
                        // Reinicia buffer para recibir
                        memset(buffer, 0, sizeof(buffer));
                        // Recibe dato
                        recv(sc_copied, buffer, MAX_LINE_LENGTH, 0);
                        // Agrega el dato a la peticion (lista de strings)
                        agregar_string(&peticion, &num_peticion, buffer);
                        // Manda la confirmacion
                       send(sc_copied, "OK\0", strlen("OK\0"), MSG_WAITALL);
                }
                err = fill_connection(peticion[CONNECTED_ALIAS], NULL, NULL, 
                                users, num_users, DISCONNECTED);
                sprintf(buffer, "%d", err);

                switch(err){
                        case 0: 
                                printf("s> DISCONNECT %s OK\n", peticion[CONNECTED_ALIAS]);
                                break;
                        case 1:
                                printf("s> DISCONNECT %s FAIL\n", peticion[CONNECTED_ALIAS]);
                                break;
                        default:
                                break;
                }
                memset(send_buffer, 0, sizeof(buffer));
                recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);

                buffer[strlen(buffer)] = '\0';
                send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);
        }

        else if (strcmp(peticion[0], "SEND") == 0){
                for (i = 0; i < NUM_SEND; i++){
                        // Reinicia buffer para recibir
                        memset(buffer, 0, sizeof(buffer));

                        // Recibe dato
                        recv(sc_copied, buffer, MAX_LINE_LENGTH, 0);
                        // Agrega el dato a la peticion (lista de strings)
                        agregar_string(&peticion, &num_peticion, buffer);
                        // Manda la confirmacion
                       send(sc_copied, "OK\0", strlen("OK\0"), MSG_WAITALL);
                }
                if (registered(peticion[SEND_REMI], users, num_users) == 0 ||
                    registered(peticion[SEND_DEST], users, num_users) == 0){
                        sprintf(buffer, "%d", -1); // Alguno de los dos usuarios no está registrado
                        printf("No estanb registrados %s , %s\n", peticion[SEND_DEST], peticion[SEND_REMI]);
                    }
                else{
                        printf("Actualizando id\n");
                        id = updateID(peticion[SEND_REMI], users, num_users);
                        writePendingMessage(peticion[SEND_DEST], peticion[SEND_REMI], id, peticion[SEND_CONTENT]);
                        sprintf(buffer, "%d", 0);
                }

                switch(atoi(buffer)){
                        case 0:
                                memset(send_buffer, 0, sizeof(buffer));
                                recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);

                                buffer[strlen(buffer)] = '\0';
                                send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);
                                
                                memset(buffer, 0, sizeof(buffer));
                                recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);
                                
                                // Envio de id
                                sprintf(send_buffer, "%u", id);
                                send_buffer[strlen(send_buffer)] = '\0';
                                send(sc_copied, send_buffer, strlen(send_buffer), 0);
                                
                                // Manda mensaje si está conectado
                                if (connected(peticion[SEND_DEST], users, num_users) == CONNECTED){
                                        // Obtiene IP y puerto e inicializa hp con información del host
                                        getUserPortIP(peticion[SEND_DEST], &ip, &port, users, num_users);
                                        sendMessage(ip, port, peticion[SEND_DEST]);
                                        borrarUltimaLinea(peticion[SEND_DEST]);
                }
                                break;

                        default:
                                memset(send_buffer, 0, sizeof(buffer));
                                recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);

                                buffer[strlen(buffer)] = '\0';
                                send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);
                                break;
                }
        }
        else if (strcmp(peticion[0], "CONNECTEDUSERS") == 0){
                printf("connectedUsers\n");

                connected_alias = connected_users(users, num_users, &num_connected);
                if (connected_alias == NULL){
                        sprintf(buffer, "%d", 2);
                } 
                else{
                        

                        memset(send_buffer, 0, sizeof(buffer));
                        recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);
                        
                        sprintf(buffer, "%d", 0);
                        buffer[strlen(buffer)] = '\0';
                        send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);

                        memset(send_buffer, 0, sizeof(buffer));
                        recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);
                        
                        sprintf(buffer, "%d", num_connected);
                        buffer[strlen(buffer)] = '\0';
                        send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);

                }

                
                
        }
        else{
               printf("Comando desconcoido\n");
               sprintf(buffer, "%d", -1); 
        }

        pthread_mutex_unlock(&mutex_users); // Fin sección critica users

        free(client_ip);
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
        server_addr.sin_port        = htons(PORT);

        printf("server port: %d\n", server_addr.sin_port);

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
                sc = accept(sd, (struct sockaddr *)&client_addr, (socklen_t *)&size);
                if (sc == -1) {
                        printf("Error en accept\n");
                        return -1;
                }

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


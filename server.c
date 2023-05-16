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
#include <netinet/in.h>

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
        int err_conn;

        // Copia el socket e IP asociados al cliente conectado
        pthread_mutex_lock(&mutex_socket);
        sc_copied = *sc; 
        client_ip = strdup(inet_ntoa(client_addr.sin_addr));
        copied = 1;
        pthread_cond_signal(&cond_copied);
        pthread_mutex_unlock(&mutex_socket);

        // Comprobacion socket
	if (sc_copied == -1){
                err = -1;
                printf("Error al asignar el socket\n");
	}

        // Recibe codigo de operacion y comprueba
	err = recvField(sc_copied, &peticion, &num_peticion);
        if (err == -1){
                printf("Error al recibir peticion inicial\n");
        }
        // Comprueba peticion
        if (peticion == NULL){
                printf("Se envió un petición vacía");
                close(sc_copied);
                pthread_exit(0);
        }

        // Inicio seccion critica para acceder a users
        pthread_mutex_lock(&mutex_users);

        //Tratamiento peticion REGISTER
        if (strcmp(peticion[0],"REGISTER") == 0){
                for (i = 0; i < NUM_REGISTER; i++){ 
                        // Recibe campo y copia en peticion
                        err = recvField(sc_copied, &peticion, &num_peticion);   
                        if (err == -1){
                                printf("Error al recibir algún campo\n");
                        }
                }
                
                if (err == 0){
                        if (registered(peticion[REGISTER_ALIAS], users, num_users) == 1){
                                sprintf(buffer, "%d", 1);
                                printf("s> REGISTER %s FAIL, USER ALREADY EXIST\n", peticion[REGISTER_ALIAS]);
                        } // Usuario registrado
                
                        else{
                                err = add_user(peticion[REGISTER_USERNAME],
                                        peticion[REGISTER_ALIAS],
                                        peticion[REGISTER_DATE], users, &num_users); // Actualiza el fichero users y array dinamico users
                                if (err == 0){
                                        err = createPendingFile(peticion[REGISTER_ALIAS]);
                                        if (err < 0){
                                                remove_user(peticion[REGISTER_ALIAS], users, &num_users); // Borra los cambios de add_user
                                                sprintf(buffer, "%d", 2);
                                                printf("s> REGISTER %s FAIL, archivo recepcion\n", peticion[REGISTER_ALIAS]);   
                                        } // Error al crear su archivo de mensajes pendientes
                                        else{
                                                sprintf(buffer, "%d", 0);
                                                printf("s> REGISTER %s OK\n", peticion[REGISTER_ALIAS]);
                                        } // Operacion realizada con exito
                                        
                                }
                                else{ // Error al escribir
                                        sprintf(buffer, "%d", 2);
                                        printf("s> REGISTER %s FAIL, al escribir\n", peticion[REGISTER_ALIAS]);
                                }
                        } // No se encontro un usuario igual
                }
                else{
                        sprintf(buffer, "%d", 2);
                } // Error conexion o recepcion

                // Envia resultado final
                sendResponse(sc_copied, buffer);
        }

        else if (strcmp(peticion[0], "UNREGISTER") == 0){

                for (i = 0; i < NUM_UNREGISTER; i++){
                        // Recibe campo y copia en peticion
                        err = recvField(sc_copied, &peticion, &num_peticion);    
                        if (err == -1){
                                printf("Error al recibir algún campo\n");
                        }            
                }

                if (err == 0){ // No hubo error en la comunicacion

                        if (registered(peticion[UNREGISTER_ALIAS], users, num_users) == 0){
                                sprintf(buffer, "%d", 1);
                                printf("s> UNREGISTER %s FAIL, USER DOESNT EXIST\n", peticion[UNREGISTER_ALIAS]);
                        } // Usuario no registrado
        
                        else{   
                                if (deletePendingFile(peticion[UNREGISTER_ALIAS]) < 0){
                                        sprintf(buffer, "%d", 2);
                                        printf("s> UNREGISTER %s FAIL\n", peticion[UNREGISTER_ALIAS]);
                                } // Fallo a borrar su archivo de mensajes pendientes

                                else if (remove_user(peticion[UNREGISTER_ALIAS], users, &num_users) < 0){
                                        createPendingFile(peticion[UNREGISTER_ALIAS]);
                                        sprintf(buffer, "%d", 2);
                                        printf("s> UNREGISTER %s FAIL\n", peticion[UNREGISTER_ALIAS]);
                                } // Fallo al borrar de users.txt o array dinamico

                                else{
                                        sprintf(buffer, "%d", 0);
                                        printf("s> UNREGISTER %s OK\n", peticion[UNREGISTER_ALIAS]);
                                } // Borrado con exito
                               
                               
                        } // Usuario registrado
                }
                else{
                        sprintf(buffer, "%d", 2);
                } // Error de recepcion
                
                sendResponse(sc_copied, buffer);
        }

        else if (strcmp(peticion[0], "CONNECT") == 0){
                for (i = 0; i < NUM_CONNECT; i++){
                        // Recibe campo y copia en peticion
                        err = recvField(sc_copied, &peticion, &num_peticion);    
                        if (err == -1){
                                printf("Error al recibir algún campo\n");
                        }                
                }

                if (err == 0){
                        err_conn = fill_connection(peticion[CONNECTED_ALIAS], client_ip, peticion[CONNECTED_PORT], 
                                                   users, num_users, CONNECTED); // Cambia estado a conectado y guarda ip y port para mandar mensajes
                        sprintf(buffer, "%d", err_conn);

                        switch(err_conn){
                        case 0: // Operacion realizada con exito
                                printf("s> CONNECT %s OK\n", peticion[CONNECTED_ALIAS]); 
                                err = 0;
                                while(err == 0){ // Manda mensajes pendientes
                                        getUserPortIP(peticion[CONNECTED_ALIAS], &ip, &port, users, num_users);
                                        sendMessage(ip, port, peticion[CONNECTED_ALIAS]);
                                        err = borrarUltimaLinea(peticion[CONNECTED_ALIAS]);
                                }
                                break;
                        case 1: // Usuario no registrado
                                printf("s> CONNECT %s FAIL, USER NOT REGISTERED\n", peticion[CONNECTED_ALIAS]);
                                break;
                        case 2: // Usuario ya conectado
                                printf("s> CONNECT %s FAIL, USER ALREADY CONNECTED\n", peticion[CONNECTED_ALIAS]);
                                break;
                        default: // Fallo desconocido
                                printf("s> CONNECT %s FAIL\n", peticion[CONNECTED_ALIAS]);
                                break;
                }
                } //Recepcion exitosa
                else{
                        printf("s> CONNECT %s FAIL\n", peticion[CONNECTED_ALIAS]);
                        sprintf(buffer, "%d", 3);
                } // Fallo recepcion
                
                // Envio de resulado 
                sendResponse(sc_copied, buffer);
        }

        else if (strcmp(peticion[0], "DISCONNECT") == 0){
                for (i = 0; i < NUM_DISCONNECT; i++){
                        // Recibe campo y copia en peticion
                        err = recvField(sc_copied, &peticion, &num_peticion);    
                        if (err == -1){
                                printf("Error al recibir algún campo\n");
                        }           
                }
                if (err == 0){
                        err_conn = fill_connection(peticion[CONNECTED_ALIAS], NULL, NULL, 
                                                   users, num_users, DISCONNECTED); // Cambia estado a desconectado
                        sprintf(buffer, "%d", err_conn);

                        switch(err_conn){
                        case 0: // Operacion realizada con exito
                                printf("s> DISCONNECT %s OK\n", peticion[CONNECTED_ALIAS]); 
                                break;
                        case 1: // Usuario no registrado
                                printf("s> DISCONNECT %s FAIL, USER NOT REGISTERED\n", peticion[CONNECTED_ALIAS]);
                                break;
                        case 2: // Usuario ya conectado
                                printf("s> DISCONNECT %s FAIL, USER ALREADY CONNECTED\n", peticion[CONNECTED_ALIAS]);
                                break;
                        default: // Fallo desconocido
                                printf("s> DISCONNECT %s FAIL\n", peticion[CONNECTED_ALIAS]);
                                break;
                }
                } // Recepcion exitosa

                else{
                        sprintf(buffer, "%d", 3);
                } // Fallo recepcion
                
                sendResponse(sc_copied, buffer);
        }

        else if (strcmp(peticion[0], "SEND") == 0){
                for (i = 0; i < NUM_SEND; i++){
                        // Recibe campo y copia en peticion
                        err = recvField(sc_copied, &peticion, &num_peticion);    
                        if (err == -1){
                                printf("Error al recibir algún campo\n");
                        }                 
                }

                if (err == 0){
                        if (registered(peticion[SEND_REMI], users, num_users) == 0 ||
                            registered(peticion[SEND_DEST], users, num_users) == 0) 
                        {
                                sprintf(buffer, "%d", 1); 
                        } // Alguno de los usuarios no esta registrado
                        else{
                                id = updateID(peticion[SEND_REMI], users, num_users);
                                if (writePendingMessage(peticion[SEND_DEST], peticion[SEND_REMI], id, peticion[SEND_CONTENT]) < 0) // Error de escritura en archivo
                                        sprintf(buffer, "%d", 2);
                                else
                                        sprintf(buffer, "%d", 0); // Mensaje escrito con exito
                        } // Intenta escribir mensajes en archivo

                        switch(atoi(buffer)){ // Comprueba que sucedio en la operacion

                                case 0: // Comprobaciones exitosas, procede enviar
                                        sendResponse(sc_copied, buffer); // Envio de resultado
                                
                                       
                                        sprintf(send_buffer, "%u", id);
                                        sendResponse(sc_copied, send_buffer);  // Envio de id
                                        
                                        // Manda mensaje a destinatario si está conectado
                                        if (connected(peticion[SEND_DEST], users, num_users) == CONNECTED){

                                                // Obtiene IP y puerto e inicializa hp con información del host
                                                getUserPortIP(peticion[SEND_DEST], &ip, &port, users, num_users);

                                                if (sendMessage(ip, port, peticion[SEND_DEST]) < 0){
                                                        // No se pudo mandar pero se almacena
                                                        printf("s> SEND MESSAGE %u %s TO %s STORED\n", id, peticion[SEND_REMI], peticion[SEND_DEST]); 
                                                } // Intenta mandar mensaje
                                                else{
                                                        borrarUltimaLinea(peticion[SEND_DEST]); // Borra mensaje del archivo
                                                } // Envio exitoso
                                                
                                        }
                                        else{ 
                                                break;
                                        }
                                                                              
                                        break;

                                case 1:
                                        printf("s> SEND MESSAGE %s OR %s IS NOT REGISTERED\n", peticion[SEND_REMI], peticion[SEND_DEST]);
                                        break;
                                
                                case 2:
                                        printf("s> SEND MESSAGE %u %s TO %s FAIL\n", id, peticion[SEND_REMI], peticion[SEND_DEST]); 
                                        break;
                                default:
                                        sprintf(buffer, "%d", 2);
                                        printf("s> SEND MESSAGE %u %s TO %s FAIL\n", id, peticion[SEND_REMI], peticion[SEND_DEST]); 
                                        break;
                        }
                }// Recepcion exitosa

                else{
                        sprintf(buffer, "%d", 2);
                        printf("s> SEND MESSAGE FAIL\n"); 
                } // Fallo en la recepcion

                sendResponse(sc_copied, buffer);
        } 

                
        else if (strcmp(peticion[0], "CONNECTEDUSERS") == 0){
                for (i = 0; i < NUM_CONNECTEDUSERS; i++){
                        err = recvField(sc_copied, &peticion, &num_peticion);
                }

                // Comprobamos que usuario está conectado
                // Primero si esta registrado
                if (connected(peticion[CONNECTEDUSERS_ALIAS], users, num_users) == 0 ||
                    registered(peticion[CONNECTEDUSERS_ALIAS], users, num_users) == 0){
                        printf("s> CONNECTEDUSERS FAIL, NOT REGISTERED OR CONNECTED\n");
                        // Envío resultado 
                        memset(send_buffer, 0, sizeof(buffer));
                        recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);
                        
                        sprintf(buffer, "%d", 1);
                        buffer[strlen(buffer)] = '\0';
                        send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);
                }
                else{
                        connected_alias = connected_users(users, num_users, &num_connected);
                        if (connected_alias == NULL){
                                printf("s> CONNECTEDUSERS FAIL\n");
                                
                                // Envío resultado 
                                memset(send_buffer, 0, sizeof(buffer));
                                recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);
                                
                                sprintf(buffer, "%d", 0);
                                buffer[strlen(buffer)] = '\0';
                                send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);
                        } 
                        else{
                                printf("s> CONNECTEDUSERS OK\n");

                                // Envío resultado 
                                memset(send_buffer, 0, sizeof(buffer));
                                recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);
                                
                                sprintf(buffer, "%d", 0);
                                buffer[strlen(buffer)] = '\0';
                                send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);

                                // Envío de num_connected
                                memset(send_buffer, 0, sizeof(buffer));
                                recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);
                                
                                sprintf(buffer, "%d", num_connected);
                                buffer[strlen(buffer)] = '\0';
                                send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);

                                for (i = 0; i < num_connected; i++){
                                        // Envío de num_connected
                                        memset(send_buffer, 0, sizeof(buffer));
                                        recv(sc_copied, send_buffer, MAX_LINE_LENGTH, 0);

                                        sprintf(buffer, "%s", connected_alias[i]);
                                        buffer[strlen(buffer)] = '\0';
                                        send(sc_copied, buffer, strlen(buffer), MSG_WAITALL);
                                }




                        }

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
        char host[256];
        char *IP;
        struct hostent *host_entry;

        // Verificar si se proporcionó el argumento -p y su valor
        if (argc < 3 || strcmp(argv[1], "-p") != 0 || !isNumber(argv[2])) {
                printf("Uso: ./server -p <numero>\n");
                return 1;
        }

        // Obtener el valor del número asociado a -p
        int port = atoi(argv[2]);
        printf("Número de puerto: %d\n", port);

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
        server_addr.sin_port        = htons(port);

      

        // Asigna la dirección y puerto al socket del servidor
        err = bind(sd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
        if (err == -1) {
                printf("Error en bind\n");
                return -1;
        }

        gethostname(host, sizeof(host));
        host_entry = gethostbyname(host);
        IP = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));

        // Se conifguran los atirbutos del hilo para que no tengan que esperar
        pthread_attr_init(&t_attr);
        pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);
        printf("s> init server %s:%d\n", IP, port);
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


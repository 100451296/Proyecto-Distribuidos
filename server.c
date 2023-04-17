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
#include <manage.h>

#define MAX_LINE 	256

pthread_mutex_t mutex_socket = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_copied = PTHREAD_COND_INITIALIZER;
int copied = 0;

void manage_client(int *sc){
        int sc_copied;
        char buffer[MAX_LINE];
        int err_thread;

        pthread_mutex_lock(&mutex_socket);
        sc_copied = *sc; 
        copied = 1;
        pthread_cond_signal(&cond_copied);
        pthread_mutex_unlock(&mutex_socket);

        

        printf("Cliente conectado\n");

        close(sc_copied);
        pthread_exit(0);
}

int main(int argc, char *argv[])
{
	int val;
	int err;
    int sd, sc;
    socklen_t size;
    struct sockaddr_in server_addr,  client_addr;
    pthread_t thid;
    pthread_attr_t t_attr;


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
        return (0);        
}


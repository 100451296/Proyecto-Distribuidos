#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "manage.h"
#include <limits.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <ctype.h>

int isNumber(const char* str) {
    // Verificar si el string es un número válido
    int i = 0;
    if (str[0] == '-') {
        i = 1;
    }
    for (; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

User **read_users_from_file(int *num_users) {
    FILE *fp = fopen(USERS_PATH, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: could not open file '%s'\n", USERS_PATH);
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    char *user_str;
    User **user_arr = malloc(MAX_USERS * sizeof(User *));
    int i = 0;

    while (fgets(line, MAX_LINE_LENGTH, fp) != NULL && i < MAX_USERS) {
        user_str = strtok(line, ";");
        while (user_str != NULL && i < MAX_USERS) {
            char *username, *alias, *date, *id;
            username = strtok(user_str, ",");
            alias = strtok(NULL, ",");
            date = strtok(NULL, ",");
            id = strtok(NULL, ",");
            if (username != NULL && alias != NULL && date != NULL) {
                User *new_user = malloc(sizeof(User));
                new_user->username = strdup(username);
                new_user->alias = strdup(alias);
                new_user->date = strdup(date);
                new_user->connected = DISCONNECTED;
                sscanf(id, "%u", &new_user->id);
                user_arr[i] = new_user;
                i++;
            }
            user_str = strtok(NULL, ";");
        }
    }

    fclose(fp);

    *num_users = i;
    return user_arr;
}

int recvField(int sc_copied, char ***peticion, int *num_peticion){
    
    char buffer[MAX_LINE_LENGTH];
    
    // Reinicia buffer para recibir
    memset(buffer, 0, sizeof(buffer));

    // Recibe dato
    if (recv(sc_copied, buffer, MAX_LINE_LENGTH, 0) < 0){
        printf("Error al recibir\n");
        return -1;
    }
    // Agrega el dato a la peticion (lista de strings)
    if (agregar_string(peticion, num_peticion, buffer) < 0){
        printf("Error al añadir cadena a peticion\n");
        return -1;
    }
    // Manda la confirmacion
    if (send(sc_copied, "OK\0", strlen("OK\0"), MSG_WAITALL) < 0){
        printf("Error al enviar\n");
        return -1;
    }

    return 0;
    
}

int sendResponse(int sc_copied, char *buffer){

    char recv_buffer[MAX_LINE_LENGTH];

    // Reinicia buffer
    memset(recv_buffer, 0, sizeof(recv_buffer));

    // Recibe confirmacion para enviar
    if (recv(sc_copied, recv_buffer, MAX_LINE_LENGTH, 0) < 0){
        printf("Error al recibir\n");
        return -1;
    }

    // Envia repuesta
    buffer[strlen(buffer)] = '\0';
    if (send(sc_copied, buffer, strlen(buffer), MSG_WAITALL) < 0){
        printf("Error al enviar\n");
        return -1;
    }
    
    return 0;
}

int add_user(const char *new_username, const char *new_alias, const char *new_date, User **user_arr, int *num_users) {
    // Actualiza el archivo de usuarios
    FILE *fp = fopen(USERS_PATH, "a+");
    if (fp == NULL) {
        fprintf(stderr, "Error: could not open file '%s'\n", USERS_PATH);
        return -1;
    }

    // Write new user to file
    fprintf(fp, "%s,%s,%s,%d;\n", new_username, new_alias, new_date, 0);

    fclose(fp);

    // Actualiza el array de usuarios
    User *new_user = malloc(sizeof(User));
    new_user->username = strdup(new_username);
    new_user->alias = strdup(new_alias);
    new_user->date = strdup(new_date);
    new_user->connected = DISCONNECTED;
    new_user->id = 0;

    if (*num_users < MAX_USERS) {
        user_arr[*num_users] = new_user;
        (*num_users)++;
        return 0;
    } else {
        // Si se excede el numero maximo de usuarios, libera la memoria del usuario nuevo
        free(new_user);
        return -1;
    }
}

int remove_user(char* username, User** user_arr, int* num_users) {
    int found = 0;
    // Buscar el usuario y eliminarlo de la lista dinámica
    for (int i = 0; i < *num_users; i++) {
        if (strcmp(username, user_arr[i]->alias) == 0) {
            free(user_arr[i]->username);
            free(user_arr[i]->alias);
            free(user_arr[i]->date);
            free(user_arr[i]->ip);
            free(user_arr[i]->port);
            
            // Mover los elementos restantes de la lista un espacio hacia atrás
            for (int j = i + 1; j < *num_users; j++) {
                user_arr[j - 1] = user_arr[j];
            }
            
            free(user_arr[*num_users - 1]); // Liberar la memoria para el último elemento
            (*num_users)--;
            found = 1;
            break;
        }
    }
    
    // Si se encontró el usuario, actualizar el archivo
    if (found) {
        FILE* f = fopen(USERS_PATH, "w");
        if (f == NULL) {
            printf("Error opening file!\n");
            return -1;
        }
        
        // Escribir los usuarios restantes en el archivo
        for (int i = 0; i < *num_users; i++) {
            fprintf(f, "%s,%s,%s,%d;\n", user_arr[i]->username, user_arr[i]->alias, user_arr[i]->date, user_arr[i]->id);
        }
        
        fclose(f);
        return 0;
    }
    else {
        printf("User not found.\n");
        return -1;
    }

}

int updateUsers(User **user_arr, int num_users){
    FILE* f = fopen(USERS_PATH, "w");
        if (f == NULL) {
            printf("Error opening file!\n");
            return -1;
        }
        
        // Escribir los usuarios restantes en el archivo
        for (int i = 0; i < num_users; i++) {
            fprintf(f, "%s,%s,%s,%d;\n", user_arr[i]->username, user_arr[i]->alias, user_arr[i]->date, user_arr[i]->id);
        }
        
        fclose(f);
        return 0;
}

int updateID(char *alias, User **user_arr, int num_users){
    // Buscar el usuario y eliminarlo de la lista dinámica
    for (int i = 0; i < num_users; i++) {
        if (strcmp(alias, user_arr[i]->alias) == 0) {
            user_arr[i]->id = incrementAndReset(user_arr[i]->id);
            updateUsers(user_arr, num_users);
            return user_arr[i]->id;
        }
    }

    return -1;
}

int registered(const char *alias, User **users, int num_users){
    // Devuelve 1 si existe 0 si no existe
    int i;

    for(i = 0; i < num_users; i++){
            if (strcmp(users[i]->alias, alias) == 0){
                    return 1; // REGISTADO
            } 
    } 
    
    return 0; // NO REGISTRADO
}

int connected(char *alias, User **users, int num_users){

    // Devuelve 1 si el alias esta conectado y  0 en caso contrario
    int i;

    for(i = 0; i < num_users; i++){
            if (users[i]->connected == CONNECTED && strcmp(users[i]->alias, alias) == 0){
                    return 1; // connected
            } 
    } 
    
    return 0; // no connected
}

int getUserPortIP(char *alias, char **ip, char **port, User **users, int num_users){
    // Devuelve 1 si existe 0 si no existe
    int i;

    for(i = 0; i < num_users; i++){
            if (users[i]->connected == CONNECTED && strcmp(users[i]->alias, alias) == 0){
                    *ip = strdup(users[i]->ip);
                    *port = strdup(users[i]->port);
                    return 0; // found and connected
            } 
    }

    return -1;
}

int fill_connection(char *alias, char* ip, char *port, User **users, int num_users, int mode){
    int i;

    for(i = 0; i < num_users; i++){
            if (strcmp(users[i]->alias, alias) == 0){ // Usuario encontrado
                if (users[i]->connected == mode)
                    return 2; // Usuario ya esta (conectado/desconectado)
                
                else{
                    if (mode == CONNECTED){ // Queremos conectar al usuario
                        users[i]->ip = strdup(ip);
                        users[i]->port = strdup(port);
                    } // Asginamos ip y port 

                    else{ // Queremos desconectar al usuario
                    
                        free(users[i]->ip);
                        users[i]->ip = NULL;
                        free(users[i]->port);
                        users[i]->port = NULL;
                    
                    } // Borramos ip y port

                    users[i]->connected = mode; // Le conectamos/desconectamos
                    
                    return 0; // Caso exito (asigna ip, port y pone a conectado)
                }

            } 
    } 
    
    return 1; // Usuario no registrado
}

char** connected_users(User** users, int num_users, int* num_connected) {
    int i;
    int connectedCount = 0;
    char** connectedAliases = NULL;

    // Contar la cantidad de usuarios conectados
    for (i = 0; i < num_users; i++) {
        if (users[i]->connected == CONNECTED) {
            connectedCount++;
        }
    }

    // Asignar memoria inicial para el array de alias conectados
    connectedAliases = (char**)malloc(connectedCount * sizeof(char*));
    if (connectedAliases == NULL) {
        // Error de asignación de memoria
        return NULL;
    }

    int index = 0;
    for (i = 0; i < num_users; i++) {
        if (users[i]->connected == CONNECTED) {
            // Copiar el alias en el array de aliases conectados
            connectedAliases[index] = strdup(users[i]->alias);
            if (connectedAliases[index] == NULL) {
                // Error de asignación de memoria al copiar el alias
                // Liberar memoria previamente asignada
                for (int j = 0; j < index; j++) {
                    free(connectedAliases[j]);
                }
                free(connectedAliases);
                return NULL;
            }
            index++;
        }
    }

    *num_connected = connectedCount;
    return connectedAliases;
}


unsigned int incrementAndReset(unsigned int value) {
    if (value == UINT_MAX) {
        return 1;
    } else {
        return value + 1;
    }
}

void free_user_array(User **user_arr, int num_users) {
    for (int i = 0; i < num_users; i++) {
        free(user_arr[i]->username);
        free(user_arr[i]->alias);
        free(user_arr[i]->date);
        free(user_arr[i]);
    }
    free(user_arr);
}

char **split_fields(char *message) {
    // Devuelve un array dinámico con los campos de la peticion (separados por DELIM)
    int num_fields = 0;
    char **fields = NULL;

    if (message == NULL)
        return NULL;

    char *token = strtok(message, DELIM);
    while (token != NULL) {
        num_fields++;
        fields = realloc(fields, num_fields * sizeof(char *));
        fields[num_fields-1] = token;

        token = strtok(NULL, DELIM);
    }

    return fields;
}

int agregar_string(char ***array, int *num_elementos, char *nuevo_string) {
    // Primero, reservamos memoria para un nuevo elemento en el array
    char **nuevo_array = realloc(*array, (*num_elementos + 1) * sizeof(char *));
    if (nuevo_array == NULL) {
        // En caso de que realloc falle, no se modifica el array original
        return -1;
    }

    // Añadimos el nuevo string al final del array
    nuevo_array[*num_elementos] = strdup(nuevo_string);

    // Actualizamos el puntero del array original
    *array = nuevo_array;
    *num_elementos = *num_elementos + 1;

    return 0;
}

int createPendingFile(const char *alias) {
    char filename[256];
    sprintf(filename, "%s%s.txt", PENDINGS_PATH, alias);
    
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error: Failed to create pending file\n");
        return -1;
    }
    
    fclose(file);
    return 0;
}

int deletePendingFile(const char *alias) {
    char filename[256];
    sprintf(filename, "%s%s.txt", PENDINGS_PATH, alias);
    
    if (remove(filename) != 0) {
        printf("Error: Failed to delete pending file\n");
        return -1;
    }
    
    return 0;
}

int writePendingMessage(const char* dest, const char* remitente, int id, const char* content) {
    /*Escribe en el archivo pendings/dest.txt el mensaje remi,id,content*/
    char filename[256];
    sprintf(filename, "%s%s.txt", PENDINGS_PATH, dest);

    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        printf("Error opening file for writing: %s\n", filename);
        return -1;
    }

    fprintf(file, "%s,%d,%s\n", remitente, id, content);

    fclose(file);
    return 0;
}

int extraerUltimaLinea(char *dest, int* id, char** remi, char** content) {
    char archivo[100];
    snprintf(archivo, sizeof(archivo), "%s%s.txt", PENDINGS_PATH, dest);

    FILE* fp = fopen(archivo, "r+");
    if (fp == NULL) {
        printf("Error al abrir el archivo.\n");
        return -1;
    }

    char linea[1024];
    long ultimaPos = -1;
    long posActual;

    while (fgets(linea, sizeof(linea), fp) != NULL) {
        posActual = ftell(fp);
        if (linea[0] != '\n') {
            ultimaPos = posActual - strlen(linea);
        }
    }

    if (ultimaPos >= 0) {
        fseek(fp, ultimaPos, SEEK_SET);
        if (fgets(linea, sizeof(linea), fp) != NULL) {
            char tempRemi[100];
            char tempContent[100];
            if (sscanf(linea, "%99[^,],%d,%99[^\n]", tempRemi, id, tempContent) == 3) {
                *remi = strdup(tempRemi);
                *content = strdup(tempContent);
                fseek(fp, ultimaPos, SEEK_SET);
                fputs("", fp);  // Borra la última línea escribiendo una cadena vacía
                fclose(fp);
                return 0;  // Éxito
            }
        }
    }

    fclose(fp);
    return -1;  // Error
}

int sendMessage(char *ip, char *port, char *dest){

    int sd_send;
    int err;
    struct sockaddr_in server_addr;
    struct hostent *hp;
    int id;
    char *remi;
    char *content;
    //char buffer[MAX_ALIAS_LENGTH];
    char buffer_alias[1024];
    char buffer_id[1024];
    char buffer_content[1024];
    char buffer[MAX_LINE_LENGTH];

    // Inicializa el descriptor de socket
    sd_send = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd_send < 0){
            perror("Error in socket");
            return -1;
    }
    
    // Inicializa estructura de servidor
    bzero((char *)&server_addr, sizeof(server_addr));

   

    
    hp = gethostbyname(ip);
    if (hp == NULL) {
            perror("Error en gethostbyname");
            exit(1);
    }
    
    // Copia en estructura del servidor información del host
    memcpy(&(server_addr.sin_addr), hp->h_addr, hp->h_length);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    
    if (isEmpty(dest) == -1){
        return -1;
    }
    
    extraerUltimaLinea(dest, &id, &remi, &content);

    sprintf(buffer_alias, "%s", remi);
    sprintf(buffer_id, "%d", id);
    sprintf(buffer_content, "%s", content);

    // Intenta conectarse 
    err = connect(sd_send, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (err == -1) {
            printf("Error en connect\n");
            return -1;
    }
    send(sd_send, "SEND_MESSAGE\0", strlen("SEND_MESSAGE\0"), 0);

    memset(buffer, 0, sizeof(buffer));
    recv(sd_send, buffer, MAX_LINE_LENGTH, 0);
    if (strcmp(buffer, "OK") != 0)
        return -1;

    buffer_alias[strlen(buffer_alias)] = '\0';
    send(sd_send, buffer_alias, strlen(buffer_alias), 0);

    memset(buffer, 0, sizeof(buffer));
    recv(sd_send, buffer, MAX_LINE_LENGTH, 0);
    if (strcmp(buffer, "OK") != 0)
        return -1;

    buffer_id[strlen(buffer_id)] = '\0';
    send(sd_send, buffer_id, strlen(buffer_id), 0);

    memset(buffer, 0, sizeof(buffer));
    recv(sd_send, buffer, MAX_LINE_LENGTH, 0);
    if (strcmp(buffer, "OK") != 0)
        return -1;

    buffer_content[strlen(buffer_content)] = '\0';
    send(sd_send, buffer_content, strlen(buffer_content), 0);

    memset(buffer, 0, sizeof(buffer));
    recv(sd_send, buffer, MAX_LINE_LENGTH, 0);
    if (strcmp(buffer, "OK") != 0)
        return -1;



    close(sd_send);
    printf("s> SEND MESSAGE %d FROM %s TO %s\n", id, remi, dest);
    //borrarUltimaLinea(dest);
    return 0;
}

int borrarUltimaLinea(char* dest) {
    char nombreArchivo[100];
    snprintf(nombreArchivo, sizeof(nombreArchivo), "%s%s.txt", PENDINGS_PATH, dest);

    FILE* archivo = fopen(nombreArchivo, "r");
    if (archivo == NULL) {
        printf("Error al abrir el archivo.\n");
        return -1;
    }

    // Verificar si el archivo está vacío
    fseek(archivo, 0, SEEK_END);
    if (ftell(archivo) == 0) {
        fclose(archivo);
        return -1;
    }

    // Crear un archivo temporal para escribir el contenido sin la última línea
    FILE* archivoTemp = fopen("temp.txt", "w");
    if (archivoTemp == NULL) {
        printf("Error al abrir el archivo temporal.\n");
        fclose(archivo);
        return -1;
    }

    fseek(archivo, 0, SEEK_SET); // Mover el puntero al inicio del archivo

    char buffer[1024];
    char ultimaLinea[1024];
    ultimaLinea[0] = '\0';
    int primeraLinea = 1;

    // Leer el archivo línea por línea y guardar la última línea en un buffer separado
    while (fgets(buffer, sizeof(buffer), archivo) != NULL) {
        if (!primeraLinea) {
            fputs(ultimaLinea, archivoTemp);
        }
        strcpy(ultimaLinea, buffer);
        primeraLinea = 0;
    }

    fclose(archivo);
    fclose(archivoTemp);

    // Si solo había una línea en el archivo, se elimina el archivo temporal sin cambios
    if (primeraLinea) {
        remove("temp.txt");
        return -1;
    }

    // Renombrar el archivo temporal como el archivo original
    remove(nombreArchivo);
    rename("temp.txt", nombreArchivo);

    return 0;
}

int isEmpty(char* dest){

    char nombreArchivo[100];
    snprintf(nombreArchivo, sizeof(nombreArchivo), "%s%s.txt", PENDINGS_PATH, dest);

    FILE* archivo = fopen(nombreArchivo, "r");
    if (archivo == NULL) {
        printf("Error al abrir el archivo.\n");
        return -1;
    }

    // Verificar si el archivo está vacío
    fseek(archivo, 0, SEEK_END);
    if (ftell(archivo) == 0) {
        fclose(archivo);
        return -1;
    }

    fclose(archivo);
    return 0;
}
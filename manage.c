#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "manage.h"
#include <limits.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        return 1;
    } else {
        // Si se excede el numero maximo de usuarios, libera la memoria del usuario nuevo
        free(new_user);
        return 0;
    }
}

int remove_user(char* username, User** user_arr, int* num_users) {
    int found = 0;
    // Buscar el usuario y eliminarlo de la lista dinámica
    for (int i = 0; i < *num_users; i++) {
        if (strcmp(username, user_arr[i]->username) == 0) {
            free(user_arr[i]->username);
            free(user_arr[i]->alias);
            free(user_arr[i]->date);
            free(user_arr[i]->ip);
            free(user_arr[i]->port);
            free(user_arr[i]);
            
            // Mover los elementos restantes de la lista un espacio hacia atrás
            for (int j = i + 1; j < *num_users; j++) {
                user_arr[j - 1] = user_arr[j];
            }
            
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
            exit(1);
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
    // Devuelve 1 si existe 0 si no existe
    int i;

    for(i = 0; i < num_users; i++){
            if (users[i]->connected == CONNECTED){
                    return 1;
            } 
    } 
    
    return 0;
}

int fill_connection(char *alias, char* ip, char *port, User **users, int num_users, int mode){
    int i;

    for(i = 0; i < num_users; i++){
            if (strcmp(users[i]->alias, alias) == 0){ // Usuario encontrado
                if (users[i]->connected == mode)
                    return 2; // Usuario ya esta (conectado/desconectado)
                
                else{
                    if (mode == CONNECTED){ // Queremos conectar al usuario
                        users[i]->ip = strdup(alias);
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
    printf("Pending file created: %s\n", filename);
    return 0;
}

int deletePendingFile(const char *alias) {
    char filename[256];
    sprintf(filename, "%s%s.txt", PENDINGS_PATH, alias);
    
    if (remove(filename) != 0) {
        printf("Error: Failed to delete pending file\n");
        return -1;
    }
    
    printf("Pending file deleted: %s\n", filename);
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
    printf("Message written to file: %s\n", filename);
    return 0;
}
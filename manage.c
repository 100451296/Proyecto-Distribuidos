#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "manage.h"


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
            char *username, *alias, *date;
            username = strtok(user_str, ",");
            alias = strtok(NULL, ",");
            date = strtok(NULL, ",");
            if (username != NULL && alias != NULL && date != NULL) {
                User *new_user = malloc(sizeof(User));
                new_user->username = strdup(username);
                new_user->alias = strdup(alias);
                new_user->date = strdup(date);
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
    fprintf(fp, "%s,%s,%s;\n", new_username, new_alias, new_date);

    fclose(fp);

    // Actualiza el array de usuarios
    User *new_user = malloc(sizeof(User));
    new_user->username = strdup(new_username);
    new_user->alias = strdup(new_alias);
    new_user->date = strdup(new_date);

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
            free(user_arr[i]);
            // Mover los elementos restantes de la lista un espacio hacia atrás
            for (int j = i; j < *num_users - 1; j++) {
                user_arr[j] = user_arr[j + 1];
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
            fprintf(f, "%s,%s,%s", user_arr[i]->username, user_arr[i]->alias, user_arr[i]->date);
            if (i < *num_users) {
                fprintf(f, ";");
            }
            fprintf(f, "\n");
        }
        fclose(f);
        return 0;
    }
    else {
        printf("User not found.\n");
        return -1;
    }
}



int registered(const char *username, const char *alias, User **users, int num_users){
    int i;

    for(i = 0; i < num_users; i++){
            if (strcmp(users[i]->username, username) == 0 ||
                strcmp(users[i]->alias, alias) == 0){
                    return 1;
            } 
    } 
    
    return 0;
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

    char *token = strtok(message, DELIM);
    while (token != NULL) {
        num_fields++;
        fields = realloc(fields, num_fields * sizeof(char *));
        fields[num_fields-1] = token;

        token = strtok(NULL, DELIM);
    }

    return fields;
}
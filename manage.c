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
    fprintf(fp, "%s,%s,%s;", new_username, new_alias, new_date);

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "manage.h"


char **split_fields(char *message) {
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
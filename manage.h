#ifndef MANAGE
#define MANAGE

#define USERS_PATH "./files/users.txt"

#define DELIM ":;"

#define MAX_LINE 	256

#define MAX_LINE_LENGTH 1024
#define MAX_USERS 1000
#define MAX_USERNAME_LENGTH 256
#define MAX_ALIAS_LENGTH 256
#define MAX_DATE_LENGTH 256

// INDICES PARA PETICION
#define REGISTER_USERNAME 1
#define REGISTER_ALIAS 2
#define REGISTER_DATE 3

typedef struct User {
    char *username;
    char *alias;
    char *date;
    int conected;
    char *ip;
    char *port;
} User;

char **split_fields(char *message);
User **read_users_from_file(int *num_users);
int add_user(const char *new_username, const char *new_alias, const char *new_date, User **user_arr, int *num_users);
void free_user_array(User **user_arr, int num_users);

#endif
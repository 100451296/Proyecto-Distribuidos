#ifndef MANAGE
#define MANAGE

#define USERS_PATH "./files/users.txt"
#define USERS_TEMP_PATH "./files/tmp_users.txt"

#define DELIM ":;"

#define MAX_LINE 	256

#define MAX_LINE_LENGTH 1024
#define MAX_USERS 1000
#define MAX_USERNAME_LENGTH 256
#define MAX_ALIAS_LENGTH 256
#define MAX_DATE_LENGTH 256

#define CONNECTED 1
#define DISCONNECTED 0

// INDICES PARA PETICION
#define REGISTER_USERNAME 1
#define REGISTER_ALIAS 2
#define REGISTER_DATE 3

#define CONNECTED_ALIAS 1
#define CONNECTED_PORT 2

typedef struct User {
    char *username;
    char *alias;
    char *date;
    int connected; // 0 disconnected 1 connected
    char *ip;
    char *port;
    char **pending;
} User;

char **split_fields(char *message);
User **read_users_from_file(int *num_users);
int add_user(const char *new_username, const char *new_alias, const char *new_date, User **user_arr, int *num_users);
void free_user_array(User **user_arr, int num_users);
int registered(const char *username, const char *alias, User **users, int num_users);
int remove_user(char *username, User **user_arr, int *num_users);
int connected(char *alias, User **users, int num_users);
int fill_connected(char *alias, char* ip, char *port, User **users, int num_users);

#endif
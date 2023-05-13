#ifndef MANAGE
#define MANAGE

#define USERS_PATH "./files/users.txt"
#define USERS_TEMP_PATH "./files/tmp_users.txt"

#define PENDINGS_PATH "./files/pendings/"

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

// NUMERO PARAMETROS POR PETICION
#define NUM_REGISTER 3
#define NUM_UNREGISTER 3
#define NUM_CONNECT 2
#define NUM_DISCONNECT 1
#define NUM_SEND 3


typedef struct User {
    char *username;
    char *alias;
    char *date;
    int connected; // 0 disconnected 1 connected
    char *ip;
    char *port;
} User;

char **split_fields(char *message);
User **read_users_from_file(int *num_users);
int add_user(const char *new_username, const char *new_alias, const char *new_date, User **user_arr, int *num_users);
void free_user_array(User **user_arr, int num_users);
int registered(const char *username, const char *alias, User **users, int num_users);
int remove_user(char *username, User **user_arr, int *num_users);
int connected(char *alias, User **users, int num_users);
int fill_connection(char *alias, char* ip, char *port, User **users, int num_users, int mode);
int agregar_string(char ***array, int *num_elementos, char *nuevo_string);
int createPendingFile(const char *alias);
int deletePendingFile(const char *alias);

#endif
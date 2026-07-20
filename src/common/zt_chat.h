#ifndef _ZT_CHAT_H_
#define _ZT_CHAT_H_

#include <time.h>

#define USER_ID_MAX_LEN 16
#define USER_NAME_MAX_LEN 16
#define PASSWORD_MAX_LEN 16
#define MDN_MAX_LEN 16
#define MESSAGE_MAX_LEN 256
#define SESSION_ID_MAX_LEN 16
#define USER_POOL_MAX_COUNT 64

typedef enum {
    USER_TYPE_NORMAL = 0,
    USER_TYPE_ADMIN = 1,
} user_type_e;

typedef struct User {
    char user_id[USER_ID_MAX_LEN];
    char name[USER_NAME_MAX_LEN];
    char password[PASSWORD_MAX_LEN];
    char mdn[MDN_MAX_LEN];
} user_t;

typedef struct Message {
    unsigned int from_id;
    unsigned int to_id;
    time_t msg_time;
    char text[MESSAGE_MAX_LEN];
} message_t;

typedef struct Session {
    user_t user;
    char session_id[SESSION_ID_MAX_LEN];
} session_t;

#define ROOM_ID_MAX_LEN 64
#define ROOM_TYPE_MAX_LEN 8
#define ROOM_MEMBERS_MAX_LEN 256
#define FRIEND_MAX_COUNT 64

typedef struct FriendUser {
    char user_id[USER_ID_MAX_LEN];
    char name[USER_NAME_MAX_LEN];
    char mdn[MDN_MAX_LEN];
} friend_t;

typedef struct {
    char room_id[ROOM_ID_MAX_LEN];
    char password[PASSWORD_MAX_LEN];
    char creator_id[USER_ID_MAX_LEN];
    char room_type[ROOM_TYPE_MAX_LEN];
    char members[ROOM_MEMBERS_MAX_LEN];
    char channel_id[ROOM_ID_MAX_LEN];
} room_t;

int chat_join(int socket, const user_t *user);
int chat_login(int socket, const char *user_id, const char *password);
int chat_create_room(int socket, const char *room_id, const char *password, const char *user_id,
                     const char *room_type, const char *members);
int chat_search_room(int socket, const char *room_id);
int chat_list_rooms(int socket);
int chat_join_room(int socket, const char *room_id, const char *password);
int chat_list_friends(int socket, const char *user_id);
int chat_invite_room(int socket, const char *room_id, const char *user_id, const char *members);

#endif

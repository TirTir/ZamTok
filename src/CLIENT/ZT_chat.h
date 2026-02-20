#ifndef _ZT_CHAT_H_
#define _ZT_CHAT_H_

#define USER_ID_MAX_LEN 16
#define USER_NAME_MAX_LEN 16
#define PHONE_MAX_LEN 11
#define EMAIL_MAX_LEN 64
#define MESSAGE_MAX_LEN 256
#define SESSION_ID_MAX_LEN 16

typedef enum {
    USER_TYPE_NORMAL = 0,
    USER_TYPE_ADMIN = 1,
} user_type_e;

typedef struct User {
    // unsigned int i_id; // server will auto increment id
    char str_user_id[USER_ID_MAX_LEN];
    char str_name[USER_NAME_MAX_LEN];
    char str_phone[PHONE_MAX_LEN];
    char str_email[EMAIL_MAX_LEN];
} user_t;

typedef struct Message {
    unsigned int u_from_id;
    unsigned int u_to_id;
    time_t t_time;
    char str_message[MESSAGE_MAX_LEN];
} message_t;

typedef struct Session {
    user_t user;
    char session_id[SESSION_ID_MAX_LEN];
} session_t;

#endif
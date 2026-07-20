#ifndef _ZT_REDIS_H_
#define _ZT_REDIS_H_

#define USER_KEY_FMT "user:%s"
#define MDN_KEY_FMT "mdn:%s"
#define ROOM_KEY_FMT "room:%s"
#define ROOM_KEY_PATTERN "room:*"
#define FRIEND_KEY_FMT "friends:%s"
#define ROOM_MEMBERS_KEY_FMT "room_members:%s"
#define USER_ROOMS_KEY_FMT "user_rooms:%s"

#define KEY_MAX_LEN 128
#define CREATED_MAX_LEN 32

int redis_connect(const char *str_host, int i_port);
void redis_disconnect(void);

int redis_user_save(const user_t *user);
int redis_user_exists(const char *user_id);
int redis_mdn_exists(const char *mdn);
int redis_user_get(const char *user_id, user_t *out_user);
int redis_friend_list(const char *user_id, friend_t *out_friends, size_t max_friends);
int redis_user_is_friend(const char *user_id, const char *friend_id);

int redis_room_save(const room_t *room);
int redis_room_get(const char *room_id, room_t *out_room);
int redis_room_list(room_t *out_rooms, size_t max_rooms);
int redis_room_invite(const char *room_id, const char *user_id, const char *members);

#define REDIS_USER_SAVE_CMD "HSET %s name %s pwd %s mdn %s created_at %s"
#define REDIS_USER_GET_CMD "HMGET %s pwd name mdn"
#define REDIS_MDN_SAVE_CMD "SET %s %s"
#define REDIS_EXISTS_CMD "EXISTS %s"
#define REDIS_ROOM_SAVE_CMD "HSET %s pwd %s created_at %s user_id %s room_type %s members %s channel_id %s"
#define REDIS_ROOM_GET_CMD "HMGET %s pwd user_id room_type members channel_id"
#define REDIS_ROOM_LIST_CMD "SCAN %s MATCH " ROOM_KEY_PATTERN " COUNT 50"
#define REDIS_ROOM_MEMBER_ADD_CMD "SADD %s %s"
#define REDIS_USER_ROOM_ADD_CMD "SADD %s %s"
#define REDIS_FRIEND_LIST_CMD "SMEMBERS %s"
#define REDIS_IS_FRIEND_CMD "SISMEMBER %s %s"

#endif

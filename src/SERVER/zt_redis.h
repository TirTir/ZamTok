#ifndef _ZT_REDIS_H_
#define _ZT_REDIS_H_

#define USER_KEY_FMT "user:%s"
#define ROOM_KEY_FMT "room:%s"
#define ROOM_KEY_PATTERN "room:*"

#define KEY_MAX_LEN 64
#define CREATED_MAX_LEN 32

/* Redis Connect (서버 기동 시 1회 호출 권장) */
int redis_connect(const char *str_host, int i_port);

void redis_disconnect(void);

/* User Save */
int redis_user_save(const user_t *user);

/* User Get */
int redis_user_get(const char *user_id, user_t *out_user);

/* Room Save */
int redis_room_save(const room_t *room);

/* Room Get */
int redis_room_get(const char *room_id, room_t *out_room);

/* Room List */
int redis_room_list(room_t *out_rooms, size_t max_rooms);

#define REDIS_USER_SAVE_CMD "HSET %s name %s pwd %s created_at %s"
#define REDIS_USER_GET_CMD "HMGET %s pwd name"
#define REDIS_ROOM_SAVE_CMD "HSET %s pwd %s created_at %s creator_id %s"
#define REDIS_ROOM_GET_CMD "HMGET %s pwd creator_id"
#define REDIS_ROOM_LIST_CMD "SCAN 0 MATCH " ROOM_KEY_PATTERN " COUNT 50"

#endif

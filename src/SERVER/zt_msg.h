#ifndef _ZT_MSG_H_
#define _ZT_MSG_H_

#include <stddef.h>

/* Stored/published payload: {"type":"message","id","room_id","from","text","ts"} */
#define ROOM_MSGS_KEY_FMT "room_msgs:%s"
#define ROOM_MSG_SEQ_KEY_FMT "room_msg_seq:%s"
#define ROOM_MSGS_KEEP 200
#define ROOM_MSGS_HISTORY 50
#define MSG_TEXT_MAX_LEN 1024 /* bytes */
#define TEXT_KEY "\"text\""

/* 1: member, 0: not, <0: error */
int room_is_member(const char *room_id, const char *user_id);
int room_member_add(const char *room_id, const char *user_id);
int room_members_json(const char *room_id, char *out, size_t out_size);

#endif

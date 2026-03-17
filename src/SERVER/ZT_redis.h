#ifndef _ZT_REDIS_H_
#define _ZT_REDIS_H_

#include <stddef.h>
#include "ZT_chat.h"

/* Redis 키 설계
 * - user:{user_id}  : HASH { name, pwd, created_at }
 *   Join 시 저장, Login 시 pwd 비교용 조회
 *
 * - room:{room_id}  : HASH { pwd, created_at, creator_id }
 *   CreateRoom 시 저장, 방 입장 시 pwd 비교용 조회
 */

/** Redis 연결 (서버 기동 시 1회 호출 권장). 0 성공, 음수 실패 */
int ZT_REDIS_Connect(const char *host, int port);

/** 연결 해제 */
void ZT_REDIS_Disconnect(void);

/** 회원가입: user_id 중복이면 -1, 성공 0, 기타 -2 */
int ZT_REDIS_UserSave(const user_t *pt_user);

/** 로그인 검증용: 유저 정보 조회. 없으면 -1, 있으면 0. 조회 결과는 out_user에 채워 반환 */
int ZT_REDIS_UserGet(const char *user_id, user_t *out_user);

/** 채팅방 생성: room_id 중복이면 -1, 성공 0, 기타 -2 */
int ZT_REDIS_RoomSave(const room_t *pt_room);

/** 채팅방 조회: 없으면 -1, 있으면 0. 조회 결과는 out_room에 채워 반환 */
int ZT_REDIS_RoomGet(const char *room_id, room_t *out_room);

#endif

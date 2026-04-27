#ifndef _ZT_HDL_H_
#define _ZT_HDL_H_

#include "ZT_ctx.h"
#include "ZT_Inc.h"

void HDL_400 ( int socket );
void HDL_500 ( int socket );

static int HDL_Join( ReqType_t *t_request );
static int HDL_Login( ReqType_t *t_request );
static int HDL_CreateRoom( ReqType_t *t_request );
static int HDL_SearchRoom( ReqType_t *t_request );
static int HDL_JoinRoom( ReqType_t *t_request );
static int HDL_ListRooms( ReqType_t *t_request );

#endif

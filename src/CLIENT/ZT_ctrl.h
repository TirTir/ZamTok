#ifndef _ZT_CTRL_H_
#define _ZT_CTRL_H_

#include "ZT_chat.h"

#define MAX_CMD_LEN  512
#define MAX_ARGC     32

typedef struct {
	char *name;
	int cmd;
	char *doc;
} st_commands_t;

typedef enum Type
{
	type_user	= 0x0001,
	type_message	= 0x0002,
	type_session	= 0x0004,
} type_t;


void CTRL_proc(int argc, char **argv);
int CTRL_start(int socket);

#endif

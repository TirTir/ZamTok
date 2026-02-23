#include "ZT_Inc.h"
#include "ZT_ctrl.h"
#include <stdio.h>
#include <strings.h>

void CTRL_proc(int argc, char **argv)
{
	if (argc < 1)
		return;

	if (!strcasecmp(argv[0], "signup")) 
	{
		printf("[1] USER ID: ");
		printf("[2] PASSWORD: ");
		printf("[3] EMAIL: ");
	} 
	
	if (!strcasecmp(argv[0], "help")) 
	{
		printf("Commands: help, member, quit\n");
	}
	
	if (!strcasecmp(argv[0], "quit") || !strcasecmp(argv[0], "exit")) 
	{
		printf("[CTRL] quit\n");
		exit(0);
	} 
	else {
		printf("[CTRL] unknown command: %s (try 'help')\n", argv[0]);
	}
}

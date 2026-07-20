#include "zt_common.h"
#include "zt_ctrl.h"
#include "zt_sock.h"
#include "zt_log.h"
#include "zt_log_fmt.h"
#include <stdio.h>
#include <strings.h>
#include <pthread.h>

#define MAX_MEMBER_INPUT 16

char                gstr_read_line [128] = {'\0',};
extern int          gi_pcap_open;
static int          g_socket_fd = -1;
static char         g_login_user_id[USER_ID_MAX_LEN] = {0};

st_commands_t       gp_commands [] = {
    {"[common]",0,""},
    {"menu",0,"Interactive menu"},
    {"help",0,"This Screen"},
    {"signup",4,"Interactive user signup"},
    {"login",2,"Interactive user login"},
    {"friends",0,"Show friend list"},
    {"create",4,"Interactive room create"},
    {"invite",2,"Interactive room invite"},
    {"enter",2,"Enter room"},
    {"list",0,"Show room list"},
    {"history",0,"History"},
    {"log",1,"on/off : Log (default off)"},
    {"quit",1,"Program quit"},
    {NULL, 0, NULL}
};

static int ctrl_is_logged_in(void)
{
	return g_login_user_id[0] != '\0';
}

static void ctrl_show_menu(void)
{
	LOG_FMT_SEP();
	LOG_FMT_LINE("No", "Command");
	LOG_FMT_SEP();
	LOG_FMT_LINE("1", "signup");
	LOG_FMT_LINE("2", "login");
	LOG_FMT_LINE("3", "friends");
	LOG_FMT_LINE("4", "create room");
	LOG_FMT_LINE("5", "invite room");
	LOG_FMT_LINE("6", "enter room");
	LOG_FMT_LINE("7", "list rooms");
	LOG_FMT_LINE("8", "help");
	LOG_FMT_LINE("9", "quit");
	LOG_FMT_SEP();
}

static void ctrl_request_friend_list(void)
{
	if (g_socket_fd >= 0 && ctrl_is_logged_in()) {
		if (chat_list_friends(g_socket_fd, g_login_user_id) == 0)
			LOG_MSG("[friends] Friend list request sent\n");
	}
}

static int ctrl_read_line(const char *prompt, char *out, size_t out_size)
{
	char *p;

	if (prompt == NULL || out == NULL || out_size == 0)
		return -1;

	printf("%s", prompt);
	fflush(stdout);

	if (!fgets(out, out_size, stdin))
		return -1;

	p = strchr(out, '\n');
	if (p)
		*p = '\0';

	return out[0] == '\0' ? -1 : 0;
}

static int ctrl_read_room_type(char *out, size_t out_size)
{
	char buf[32];

	while (1) {
		if (ctrl_read_line("room type (1=1:1, 2=1:N): ", buf, sizeof(buf)) < 0)
			return -1;

		if (!strcmp(buf, "1") || !strcmp(buf, "1:1")) {
			snprintf(out, out_size, "%s", "1:1");
			return 0;
		}

		if (!strcmp(buf, "2") || !strcmp(buf, "1:N")) {
			snprintf(out, out_size, "%s", "1:N");
			return 0;
		}

		LOG_MSG("[create] Invalid room type\n");
	}
}

static int ctrl_read_count(const char *prompt, int min, int max)
{
	char buf[32];
	int count;

	while (1) {
		if (ctrl_read_line(prompt, buf, sizeof(buf)) < 0)
			return -1;

		count = atoi(buf);
		if (count >= min && count <= max)
			return count;

		LOG_MSG("[input] Count must be %d..%d\n", min, max);
	}
}

static int ctrl_append_member(char *members, size_t members_size, const char *member)
{
	int n;
	size_t len;

	if (members == NULL || member == NULL || member[0] == '\0')
		return -1;

	len = strlen(members);
	n = snprintf(members + len, members_size - len, "%s%s", len > 0 ? "," : "", member);
	if (n < 0 || (size_t)n >= members_size - len)
		return -1;

	return 0;
}

static int ctrl_read_members(const char *room_type, char *members, size_t members_size)
{
	char member[USER_ID_MAX_LEN];
	int count;
	int i;

	if (room_type == NULL || members == NULL || members_size == 0)
		return -1;

	members[0] = '\0';
	ctrl_request_friend_list();

	if (!strcmp(room_type, "1:1")) {
		count = 1;
	} else {
		count = ctrl_read_count("member count: ", 1, MAX_MEMBER_INPUT);
		if (count < 0)
			return -1;
	}

	for (i = 0; i < count; i++) {
		if (ctrl_read_line("friend user_id: ", member, sizeof(member)) < 0)
			return -1;
		if (ctrl_append_member(members, members_size, member) < 0)
			return -1;
	}

	return 0;
}

static int ctrl_read_invite_members(char *members, size_t members_size)
{
	char member[USER_ID_MAX_LEN];
	int count;
	int i;

	if (members == NULL || members_size == 0)
		return -1;

	members[0] = '\0';
	ctrl_request_friend_list();

	count = ctrl_read_count("invite count: ", 1, MAX_MEMBER_INPUT);
	if (count < 0)
		return -1;

	for (i = 0; i < count; i++) {
		if (ctrl_read_line("friend user_id: ", member, sizeof(member)) < 0)
			return -1;
		if (ctrl_append_member(members, members_size, member) < 0)
			return -1;
	}

	return 0;
}

static const char *ctrl_menu_to_command(const char *cmd)
{
	if (!strcmp(cmd, "1")) return "signup";
	if (!strcmp(cmd, "2")) return "login";
	if (!strcmp(cmd, "3")) return "friends";
	if (!strcmp(cmd, "4")) return "create";
	if (!strcmp(cmd, "5")) return "invite";
	if (!strcmp(cmd, "6")) return "enter";
	if (!strcmp(cmd, "7")) return "list";
	if (!strcmp(cmd, "8")) return "help";
	if (!strcmp(cmd, "9")) return "quit";
	return cmd;
}

static int ctrl_signup_interactive(void)
{
	user_t t_user = {0};

	if (ctrl_read_line("user_id: ", t_user.user_id, sizeof(t_user.user_id)) < 0 ||
	    ctrl_read_line("name: ", t_user.name, sizeof(t_user.name)) < 0 ||
	    ctrl_read_line("password: ", t_user.password, sizeof(t_user.password)) < 0 ||
	    ctrl_read_line("mdn: ", t_user.mdn, sizeof(t_user.mdn)) < 0) {
		LOG_MSG("[signup] Input canceled\n");
		return -1;
	}

	return chat_join(g_socket_fd, &t_user);
}

static int ctrl_login_interactive(void)
{
	char user_id[USER_ID_MAX_LEN] = {0};
	char password[PASSWORD_MAX_LEN] = {0};
	int rc;

	if (ctrl_read_line("user_id: ", user_id, sizeof(user_id)) < 0 ||
	    ctrl_read_line("password: ", password, sizeof(password)) < 0) {
		LOG_MSG("[login] Input canceled\n");
		return -1;
	}

	rc = chat_login(g_socket_fd, user_id, password);
	if (rc == 0)
		snprintf(g_login_user_id, sizeof(g_login_user_id), "%s", user_id);

	return rc;
}

static int ctrl_create_interactive(void)
{
	char room_id[ROOM_ID_MAX_LEN] = {0};
	char password[PASSWORD_MAX_LEN] = {0};
	char room_type[ROOM_TYPE_MAX_LEN] = {0};
	char members[ROOM_MEMBERS_MAX_LEN] = {0};

	if (ctrl_read_line("room_id: ", room_id, sizeof(room_id)) < 0 ||
	    ctrl_read_line("password: ", password, sizeof(password)) < 0 ||
	    ctrl_read_room_type(room_type, sizeof(room_type)) < 0 ||
	    ctrl_read_members(room_type, members, sizeof(members)) < 0) {
		LOG_MSG("[create] Input canceled\n");
		return -1;
	}

	return chat_create_room(g_socket_fd, room_id, password, g_login_user_id, room_type, members);
}

static int ctrl_invite_interactive(void)
{
	char room_id[ROOM_ID_MAX_LEN] = {0};
	char members[ROOM_MEMBERS_MAX_LEN] = {0};

	if (ctrl_read_line("room_id: ", room_id, sizeof(room_id)) < 0 ||
	    ctrl_read_invite_members(members, sizeof(members)) < 0) {
		LOG_MSG("[invite] Input canceled\n");
		return -1;
	}

	return chat_invite_room(g_socket_fd, room_id, g_login_user_id, members);
}

static int ctrl_enter_interactive(void)
{
	char room_id[ROOM_ID_MAX_LEN] = {0};
	char password[PASSWORD_MAX_LEN] = {0};

	if (ctrl_read_line("room_id: ", room_id, sizeof(room_id)) < 0 ||
	    ctrl_read_line("password: ", password, sizeof(password)) < 0) {
		LOG_MSG("[enter] Input canceled\n");
		return -1;
	}

	return chat_join_room(g_socket_fd, room_id, password);
}

void CTRL_proc(int argc, char **argv)
{
	const char *cmd;
	int rc;

	if (argc < 1)
		return;

	cmd = ctrl_menu_to_command(argv[0]);

	if (!strcasecmp(cmd, "menu")) {
		ctrl_show_menu();
		return;
	}

	if (!strcasecmp(cmd, "signup"))
	{
		if (g_socket_fd < 0) {
			LOG_MSG("[signup] Socket not connected\n");
			return;
		}

		rc = ctrl_signup_interactive();
		LOG_MSG(rc == 0 ? "[signup] Join request sent\n" : "[signup] Join send fail\n");
		return;
	}

	if (!strcasecmp(cmd, "login"))
	{
		if (g_socket_fd < 0) {
			LOG_MSG("[login] Socket not connected\n");
			return;
		}

		rc = ctrl_login_interactive();
		LOG_MSG(rc == 0 ? "[login] Login request sent\n" : "[login] Login send fail\n");
		return;
	}

	if (!strcasecmp(cmd, "friends"))
	{
		if (g_socket_fd < 0) {
			LOG_MSG("[friends] Socket not connected\n");
			return;
		}

		if (!ctrl_is_logged_in()) {
			LOG_MSG("[friends] Please login first\n");
			return;
		}

		ctrl_request_friend_list();
		return;
	}

	if (!strcasecmp(cmd, "create"))
	{
		if (g_socket_fd < 0) {
			LOG_MSG("[create] Socket not connected\n");
			return;
		}

		if (!ctrl_is_logged_in()) {
			LOG_MSG("[create] Please login first\n");
			return;
		}

		rc = ctrl_create_interactive();
		LOG_MSG(rc == 0 ? "[create] Create room request sent\n" : "[create] Create room send fail\n");
		return;
	}

	if (!strcasecmp(cmd, "invite"))
	{
		if (g_socket_fd < 0) {
			LOG_MSG("[invite] Socket not connected\n");
			return;
		}

		if (!ctrl_is_logged_in()) {
			LOG_MSG("[invite] Please login first\n");
			return;
		}

		rc = ctrl_invite_interactive();
		LOG_MSG(rc == 0 ? "[invite] Invite request sent\n" : "[invite] Invite send fail\n");
		return;
	}

	if (!strcasecmp(cmd, "enter"))
	{
		if (g_socket_fd < 0) {
			LOG_MSG("[enter] Socket not connected\n");
			return;
		}

		rc = ctrl_enter_interactive();
		LOG_MSG(rc == 0 ? "[enter] Enter room request sent\n" : "[enter] Enter room send fail\n");
		return;
	}

	if (!strcasecmp(cmd, "list"))
	{
		if (g_socket_fd < 0) {
			LOG_MSG("[list] Socket not connected\n");
			return;
		}

		rc = chat_list_rooms(g_socket_fd);
		LOG_MSG(rc == 0 ? "[list] Room list request sent\n" : "[list] Room list send fail\n");
		return;
	}

	if (!strcasecmp(cmd, "help"))
	{
		int i = 0;
		LOG_FMT_SEP();
		LOG_FMT_LINE("Command", "Description");
		LOG_FMT_SEP();
		while (gp_commands[i].name) {
			LOG_FMT_LINE(gp_commands[i].name, gp_commands[i].doc);
			i++;
		}
		return;
	}

	if (!strcasecmp(cmd, "quit") || !strcasecmp(cmd, "exit")) {
		LOG_MSG("[CMD_LINE] quit\n");
		exit(0);
	}

	LOG_MSG("[CMD_LINE] unknown command: %s (try 'menu' or 'help')\n", argv[0]);
}

static int parse_cmd_line(char *line, char **argv, int max_argc)
{
	int argc = 0;
	char *p = line;

	while (argc < max_argc) {
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
			*p++ = '\0';

		if (!*p)
			break;

		argv[argc++] = p;

		while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
			p++;

		if (*p)
			*p++ = '\0';
	}

	return argc;
}

static void *CTRL_handler(void *ctx)
{
	static char line_buf[MAX_CMD_LEN];
	char *argv[MAX_ARGC];
	int nargs;

	(void)ctx;

	ctrl_show_menu();

	while (1)
	{
		printf("select> ");
		fflush(stdout);

		if (!fgets(line_buf, sizeof(line_buf), stdin))
			break;

		nargs = parse_cmd_line(line_buf, argv, MAX_ARGC);
		if (nargs < 1) {
			ctrl_show_menu();
			continue;
		}

		CTRL_proc(nargs, argv);
	}

	return NULL;
}

int CTRL_start(int socket)
{
	pthread_t t_id;

	g_socket_fd = socket;

	if (pthread_create(&t_id, NULL, CTRL_handler, (void*)NULL) < 0)
	{
		printf("[ERROR] pthread_create Fail\n");
		return -1;
	}

	return 0;
}

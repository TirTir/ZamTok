#include "zt_inc.h"

extern zt_server_ctx_t g_server_ctx;
extern unsigned char g_client_fd[MAX_CLIENTS / 8];

static int hdl_send_http_wire(int client_fd, res_t *response)
{
	char wire[HEADER_MAX_LEN + DATA_MAX_LEN + 128];
	size_t blen;
	int n, rc;

	if (client_fd < 0 || response == NULL)
		return ZT_RC_ARG_INVALID;

	blen = strlen(response->t_body.str_data);
	n = snprintf(wire, sizeof(wire),
		     "HTTP/1.1 %u %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s",
		     response->i_status_code, response->str_reason, blen, response->t_body.str_data);
	if (n < 0 || (size_t)n >= sizeof(wire))
		return ZT_RC_ARG_INVALID;

	rc = socket_send(client_fd, wire, n);
	return (rc < 0) ? ZT_RC_SOCKET : ZT_RC_OK;
}

int hdl_send_http_json(int client_fd, res_t *response)
{
	return hdl_send_http_wire(client_fd, response);
}

void hdl_bad_request(int client_fd)
{
	static const char msg[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";

	if (client_fd >= 0)
		(void)socket_send(client_fd, msg, (int)(sizeof(msg) - 1));
}

int hdl_header(char *header_buf, char *request_buf, int status, req_t *msg)
{
	char *temp = NULL;
	char *line;

	if (!header_buf || !request_buf || status < 100 || msg == NULL)
		return ZT_RC_ARG_INVALID;

	line = strtok(request_buf, "\r\n\r\n");

	msg->i_content_header_count = 0;

	while (line && msg->i_content_header_count < HEADER_MAX_COUNT) {
		if (!strncmp(line, "Content-Type:", 13)) {
			temp = strtok(line + 13, " ");
			snprintf(msg->t_content_headers[msg->i_content_header_count].str_name, NAME_MAX_LEN,
				 "%s", "Content-Type");
			snprintf(msg->t_content_headers[msg->i_content_header_count].str_value, VALUE_MAX_LEN,
				 "%s", temp ? temp : "");
			msg->i_content_header_count++;
		} else if (!strncmp(line, "Content-Length:", 15)) {
			temp = strtok(line + 15, " ");
			snprintf(msg->t_content_headers[msg->i_content_header_count].str_name, NAME_MAX_LEN,
				 "%s", "Content-Length");
			snprintf(msg->t_content_headers[msg->i_content_header_count].str_value, VALUE_MAX_LEN,
				 "%s", temp ? temp : "");
			msg->i_content_header_count++;
		}

		line = strtok(NULL, "\r\n");
	}

	{
		const char *clen =
			(msg->i_content_header_count > 1) ? msg->t_content_headers[1].str_value : "0";
		const char *ctype =
			(msg->i_content_header_count > 0) ? msg->t_content_headers[0].str_value : "text/plain";
		snprintf(header_buf, HEADER_MAX_LEN, HEADER_FMT, msg->str_version, status,
			 zt_http_status_to_reason(status), clen, ctype);
	}

	return ZT_RC_OK;
}

int hdl_header_mime(char *content_type, int size, const char *uri)
{
	char *ext;

	if (content_type == NULL) {
		LOG_MSG("[hdl_header_mime] Content Type is NULL\n");
		return ZT_RC_ARG_INVALID;
	}

	if (uri == NULL) {
		LOG_MSG("[hdl_header_mime] URI is NULL\n");
		return ZT_RC_ARG_INVALID;
	}

	ext = strrchr(uri, '.');

	if (ext == NULL) {
		snprintf(content_type, (size_t)size, "%s", "application/json");
		return ZT_RC_OK;
	}

	if (!strcmp(ext, ".html")) {
		snprintf(content_type, (size_t)size, "%s", "text/html");
	} else if (!strcmp(ext, ".jpg") || !strcmp(ext, ".jpeg")) {
		snprintf(content_type, (size_t)size, "%s", "image/jpeg");
	} else if (!strcmp(ext, ".png")) {
		snprintf(content_type, (size_t)size, "%s", "image/png");
	} else if (!strcmp(ext, ".CSS")) {
		snprintf(content_type, (size_t)size, "%s", "text/CSS");
	} else if (!strcmp(ext, ".js")) {
		snprintf(content_type, (size_t)size, "%s", "text/javascript");
	} else {
		snprintf(content_type, (size_t)size, "%s", "text/plain");
	}

	return ZT_RC_OK;
}

int hdl_socket(int epfd, int client_fd)
{
	req_t request = {0};
	char parse_buf[BUF_MAX_LEN] = {0};
	char read_buf[BUF_MAX_LEN] = {0};

	int n, rc, status;
	int retry_cnt = 0;

	(void)epfd;

	if (client_fd < 0) {
		LOG_ERR("[hdl_socket] invalid client fd <%d:%s>\n", errno, strerror(errno));
		return ZT_RC_ARG_INVALID;
	}

	while (retry_cnt < RETRY_MAX_CNT) {
		char header[HEADER_MAX_LEN] = "";

		errno = 0;
		n = read(client_fd, read_buf, sizeof(read_buf));

		if (n < 0 && (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)) {
			LOG_MSG("[hdl_socket] read retry FD=%d <%d:%s>\n", client_fd, errno,
				strerror(errno));
			retry_cnt++;
			continue;
		}

		if (n <= 0) {
			if (n == 0)
				LOG_MSG("[hdl_socket] FD=%d closed (client disconnect)\n", client_fd);
			else
				LOG_MSG("[hdl_socket] read error FD=%d <%d:%s>\n", client_fd, errno,
					strerror(errno));
			return ZT_RC_FAIL;
		}

		read_buf[n] = '\0';

		LOG_FMT_CENTER("hdl_socket_request");
		LOG_MSG("%s\n", read_buf);
		fflush(stdout);

		snprintf(request.str_body, sizeof(request.str_body), "%s", read_buf);
		snprintf(parse_buf, sizeof(parse_buf), "%s", read_buf);

		char *method = strtok(parse_buf, " ");
		char *uri = strtok(NULL, " ");
		char *version = strtok(NULL, "\r\n");

		if (!method || !uri || !version) {
			LOG_MSG("[hdl_socket] invalid request line FD=%d\n", client_fd);
			hdl_bad_request(client_fd);
			return ZT_RC_FAIL;
		}

		snprintf(request.str_method, sizeof(request.str_method), "%s", method);
		snprintf(request.str_uri, sizeof(request.str_uri), "%s", uri);
		snprintf(request.str_version, sizeof(request.str_version), "%s", version);

		if (!strcmp(request.str_uri, "/join")) {
			rc = http_join(client_fd, read_buf, &request);
			if (rc < 0)
				return ZT_RC_SOCKET;
			break;
		}

		if (!strcmp(request.str_uri, "/login")) {
			rc = http_login(client_fd, read_buf, &request);
			if (rc < 0)
				return ZT_RC_SOCKET;
			break;
		}

		if (!strcmp(request.str_uri, "/room") && !strncmp(request.str_method, "POST", 4)) {
			rc = http_create_room(client_fd, read_buf, &request);
			if (rc < 0)
				return ZT_RC_SOCKET;
			break;
		}

		if (!strncmp(request.str_uri, "/room?id=", 9) && !strncmp(request.str_method, "GET", 3)) {
			rc = http_search_room(client_fd, read_buf, &request);
			if (rc < 0)
				return ZT_RC_SOCKET;
			break;
		}

		if (!strcmp(request.str_uri, "/room/join") && !strncmp(request.str_method, "POST", 4)) {
			rc = http_join_room(client_fd, read_buf, &request);
			if (rc < 0)
				return ZT_RC_SOCKET;
			break;
		}

		if (!strcmp(request.str_uri, "/rooms") && !strncmp(request.str_method, "GET", 3)) {
			rc = http_list_rooms(client_fd, read_buf, &request);
			if (rc < 0)
				return ZT_RC_SOCKET;
			break;
		}

		if (!strncmp(request.str_uri, "/friends?user_id=", 17) &&
		    !strncmp(request.str_method, "GET", 3)) {
			rc = http_list_friends(client_fd, read_buf, &request);
			if (rc < 0)
				return ZT_RC_SOCKET;
			break;
		}

		if (!strcmp(request.str_uri, "/room/invite") && !strncmp(request.str_method, "POST", 4)) {
			rc = http_invite_room(client_fd, read_buf, &request);
			if (rc < 0)
				return ZT_RC_SOCKET;
			break;
		}

		if (!strcmp(request.str_uri, "/"))
			snprintf(request.str_uri, sizeof(request.str_uri), "%s", "/indx.html");

		rc = hdl_header_mime(request.t_content_headers[request.i_content_header_count].str_value,
				     VALUE_MAX_LEN, request.str_uri);
		if (rc == ZT_RC_OK)
			status = 200;
		else
			status = 400;

		rc = hdl_header(header, read_buf, status, &request);
		if (rc < 0) {
			LOG_ERR("hdl_header fail\n");
			return ZT_RC_SOCKET;
		}

		LOG_FMT_CENTER("hdl_socket_request parsing");
		LOG_MSG("Method: %s, URI: %s\n", request.str_method, request.str_uri);

		break;
	}

	return ZT_RC_OK;
}

int hdl_accept(int listen_fd, int epfd)
{
	struct sockaddr_in client_addr = {0};
	socklen_t addr_len = sizeof(client_addr);
	struct epoll_event ev;
	int rc = 0;
	int client_fd = -1;

	client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (client_fd < 0) {
		LOG_MSG("[hdl_accept] accept fail\n");
		return ZT_RC_SOCKET;
	}

	LOG_MSG("[hdl_accept] client FD=%d from %s:%d\n", client_fd, inet_ntoa(client_addr.sin_addr),
		ntohs(client_addr.sin_port));

	rc = ctx_http_insert(&g_server_ctx, client_fd, client_addr);
	if (rc < 0) {
		LOG_MSG("[hdl_accept] ctx_http_insert fail\n");
		close(client_fd);
		return ZT_RC_CTX;
	}

	socket_set_nonblocking(client_fd);
	ev.events = EPOLLIN | EPOLLRDHUP;
	ev.data.fd = client_fd;
	rc = epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
	if (rc < 0) {
		LOG_MSG("[hdl_accept] epoll_ctl add fail <%d:%s>\n", errno, strerror(errno));
		close(client_fd);
		return ZT_RC_CTX;
	}

	g_client_fd[client_fd / 8] |= (unsigned char)(1u << (unsigned)(client_fd % 8));

	return ZT_RC_OK;
}

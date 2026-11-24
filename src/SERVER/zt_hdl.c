#include "zt_inc.h"

#include <ctype.h>
#include <poll.h>
#include <strings.h>
#include <sys/stat.h>

extern zt_server_ctx_t g_server_ctx;
extern unsigned char g_client_fd[MAX_CLIENTS / 8];

#define HDL_SEND_TIMEOUT_MS 2000
#define HDL_STATIC_FILE_MAX_LEN (2 * 1024 * 1024)
#define HDL_WEB_ROOT_DEFAULT "./www"
#define HDL_WEB_ROOT_ENV "ZT_WEB_ROOT"

/* Per-fd buffer: a request may arrive in several reads. */
typedef struct {
	char buf[BUF_MAX_LEN];
	size_t len;
} hdl_recv_buf_t;

static hdl_recv_buf_t g_recv_buf[MAX_CLIENTS];

typedef int (*hdl_route_fn_t)(int sock_fd, const char *buf, req_t *request);

typedef struct {
	const char *str_method; /* NULL: any method */
	const char *str_path;
	bool b_prefix;          /* prefix match (query string) */
	hdl_route_fn_t fn;
} hdl_route_t;

static const hdl_route_t g_routes[] = {
	{ NULL, "/join", false, http_join },
	{ NULL, "/login", false, http_login },
	{ "POST", "/room", false, http_create_room },
	{ "GET", "/room?id=", true, http_search_room },
	{ "POST", "/room/join", false, http_join_room },
	{ "GET", "/rooms", false, http_list_rooms },
	{ "GET", "/friends?user_id=", true, http_list_friends },
	{ "POST", "/room/invite", false, http_invite_room },
	{ "POST", "/room/message", false, http_send_message },
	{ "GET", "/room/messages?", true, http_list_messages },
};

static int hdl_send_all(int client_fd, const char *buf, size_t len)
{
	size_t off = 0;

	while (off < len) {
		ssize_t n = send(client_fd, buf + off, len - off, MSG_NOSIGNAL);

		if (n > 0) {
			off += (size_t)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			struct pollfd pfd = { .fd = client_fd, .events = POLLOUT };

			if (poll(&pfd, 1, HDL_SEND_TIMEOUT_MS) > 0)
				continue;
			LOG_MSG("[hdl_send_all] FD=%d send timeout\n", client_fd);
			return ZT_RC_SOCKET;
		}

		LOG_MSG("[hdl_send_all] FD=%d send fail <%d:%s>\n", client_fd, errno, strerror(errno));
		return ZT_RC_SOCKET;
	}

	return ZT_RC_OK;
}

static int hdl_send_http_wire(int client_fd, res_t *response)
{
	char wire[HEADER_MAX_LEN + DATA_MAX_LEN + 128];
	size_t blen;
	int n;

	if (client_fd < 0 || response == NULL)
		return ZT_RC_ARG_INVALID;

	blen = strlen(response->t_body.str_data);
	n = snprintf(wire, sizeof(wire),
		     "HTTP/1.1 %u %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s",
		     response->i_status_code, response->str_reason, blen, response->t_body.str_data);
	if (n < 0 || (size_t)n >= sizeof(wire))
		return ZT_RC_ARG_INVALID;

	LOG_FMT_CENTER("Send Response");
	LOG_MSG("%s\n", wire);

	return hdl_send_all(client_fd, wire, (size_t)n);
}

int hdl_send_http_json(int client_fd, res_t *response)
{
	return hdl_send_http_wire(client_fd, response);
}

static int hdl_send_error(int client_fd, int status, zt_err_code_t err_code)
{
	char body[256];
	char wire[512];
	int blen, n;

	blen = snprintf(body, sizeof(body), BODY_FMT, "false", ZT_ERR_STR(err_code),
			ZT_ERR_DEFAULT_MSG(err_code), "null");
	if (blen < 0 || (size_t)blen >= sizeof(body))
		return ZT_RC_ARG_INVALID;

	n = snprintf(wire, sizeof(wire),
		     "HTTP/1.1 %d %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s",
		     status, zt_http_status_to_reason(status), blen, body);
	if (n < 0 || (size_t)n >= sizeof(wire))
		return ZT_RC_ARG_INVALID;

	LOG_MSG("[hdl_send_error] FD=%d %d %s\n", client_fd, status, ZT_ERR_STR(err_code));
	return hdl_send_all(client_fd, wire, (size_t)n);
}

void hdl_bad_request(int client_fd)
{
	static const char msg[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";

	if (client_fd >= 0)
		(void)hdl_send_all(client_fd, msg, sizeof(msg) - 1);
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
	const char *ext;
	const char *mime = "application/octet-stream";

	if (content_type == NULL) {
		LOG_MSG("[hdl_header_mime] Content Type is NULL\n");
		return ZT_RC_ARG_INVALID;
	}

	if (uri == NULL) {
		LOG_MSG("[hdl_header_mime] URI is NULL\n");
		return ZT_RC_ARG_INVALID;
	}

	ext = strrchr(uri, '.');

	if (ext == NULL)
		mime = "application/json";
	else if (!strcasecmp(ext, ".html"))
		mime = "text/html; charset=utf-8";
	else if (!strcasecmp(ext, ".css"))
		mime = "text/css; charset=utf-8";
	else if (!strcasecmp(ext, ".js"))
		mime = "text/javascript; charset=utf-8";
	else if (!strcasecmp(ext, ".json"))
		mime = "application/json";
	else if (!strcasecmp(ext, ".svg"))
		mime = "image/svg+xml";
	else if (!strcasecmp(ext, ".png"))
		mime = "image/png";
	else if (!strcasecmp(ext, ".jpg") || !strcasecmp(ext, ".jpeg"))
		mime = "image/jpeg";
	else if (!strcasecmp(ext, ".ico"))
		mime = "image/x-icon";
	else if (!strcasecmp(ext, ".txt"))
		mime = "text/plain; charset=utf-8";

	snprintf(content_type, (size_t)size, "%s", mime);
	return ZT_RC_OK;
}

/* Reject paths that could escape the web root. */
static bool hdl_static_path_ok(const char *path)
{
	const char *p;

	if (path[0] != '/' || strstr(path, "..") || strstr(path, "//"))
		return false;

	for (p = path; *p; p++) {
		if (!isalnum((unsigned char)*p) && *p != '/' && *p != '.' && *p != '-' && *p != '_')
			return false;
	}

	return true;
}

static int hdl_send_static(int client_fd, const char *uri, bool b_head_only)
{
	char path[URI_MAX_LEN];
	char full[FILE_MAX_LEN + URI_MAX_LEN];
	char ctype[VALUE_MAX_LEN];
	char header[HEADER_MAX_LEN * 2];
	char chunk[8192];
	const char *root = getenv(HDL_WEB_ROOT_ENV);
	size_t ulen;
	struct stat st;
	int file, n, rc;

	if (root == NULL || root[0] == '\0')
		root = HDL_WEB_ROOT_DEFAULT;

	ulen = strcspn(uri, "?#");
	if (ulen == 0 || ulen >= sizeof(path))
		return hdl_send_error(client_fd, HTTP_NOT_FOUND, ZT_ERR_NOT_FOUND);

	memcpy(path, uri, ulen);
	path[ulen] = '\0';
	if (!strcmp(path, "/"))
		snprintf(path, sizeof(path), "%s", "/index.html");

	if (!hdl_static_path_ok(path))
		return hdl_send_error(client_fd, HTTP_NOT_FOUND, ZT_ERR_NOT_FOUND);

	n = snprintf(full, sizeof(full), "%s%s", root, path);
	if (n < 0 || (size_t)n >= sizeof(full))
		return hdl_send_error(client_fd, HTTP_NOT_FOUND, ZT_ERR_NOT_FOUND);

	file = open(full, O_RDONLY | O_CLOEXEC);
	if (file < 0)
		return hdl_send_error(client_fd, HTTP_NOT_FOUND, ZT_ERR_NOT_FOUND);

	if (fstat(file, &st) < 0 || !S_ISREG(st.st_mode) || st.st_size > HDL_STATIC_FILE_MAX_LEN) {
		close(file);
		return hdl_send_error(client_fd, HTTP_NOT_FOUND, ZT_ERR_NOT_FOUND);
	}

	hdl_header_mime(ctype, sizeof(ctype), path);
	n = snprintf(header, sizeof(header),
		     "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lld\r\n"
		     "Cache-Control: no-cache\r\nX-Content-Type-Options: nosniff\r\n\r\n",
		     ctype, (long long)st.st_size);
	if (n < 0 || (size_t)n >= sizeof(header)) {
		close(file);
		return ZT_RC_ARG_INVALID;
	}

	rc = hdl_send_all(client_fd, header, (size_t)n);
	while (rc == ZT_RC_OK && !b_head_only) {
		ssize_t r = read(file, chunk, sizeof(chunk));

		if (r < 0 && errno == EINTR)
			continue;
		if (r <= 0)
			break;
		rc = hdl_send_all(client_fd, chunk, (size_t)r);
	}
	close(file);

	LOG_MSG("[static] FD=%d %s %s (%lld bytes)\n", client_fd, b_head_only ? "HEAD" : "GET", path,
		(long long)st.st_size);
	return rc;
}

/* 0 if absent, -1 if invalid */
static long hdl_content_length(const char *buf, size_t header_len)
{
	const char *p = buf;
	const char *end = buf + header_len;

	while (p < end) {
		const char *eol = memchr(p, '\n', (size_t)(end - p));
		size_t llen = eol ? (size_t)(eol - p) : (size_t)(end - p);

		if (llen >= 15 && !strncasecmp(p, "Content-Length:", 15)) {
			char *num_end = NULL;
			long v = strtol(p + 15, &num_end, 10);

			return (num_end == p + 15 || v < 0) ? -1 : v;
		}

		if (eol == NULL)
			break;
		p = eol + 1;
	}

	return 0;
}

static int hdl_dispatch(int client_fd, char *read_buf)
{
	req_t request = {0};
	char parse_buf[BUF_MAX_LEN];
	char *method, *uri, *version;
	size_t i;
	int rc;

	LOG_FMT_CENTER("hdl_socket_request");
	LOG_MSG("%s\n", read_buf);

	snprintf(request.str_body, sizeof(request.str_body), "%s", read_buf);
	snprintf(parse_buf, sizeof(parse_buf), "%s", read_buf);

	method = strtok(parse_buf, " ");
	uri = strtok(NULL, " ");
	version = strtok(NULL, "\r\n");

	if (!method || !uri || !version) {
		LOG_MSG("[hdl_dispatch] invalid request line FD=%d\n", client_fd);
		hdl_bad_request(client_fd);
		return ZT_RC_FAIL;
	}

	snprintf(request.str_method, sizeof(request.str_method), "%s", method);
	snprintf(request.str_uri, sizeof(request.str_uri), "%s", uri);
	snprintf(request.str_version, sizeof(request.str_version), "%s", version);

	for (i = 0; i < sizeof(g_routes) / sizeof(g_routes[0]); i++) {
		const hdl_route_t *route = &g_routes[i];
		bool b_path = route->b_prefix
				      ? !strncmp(request.str_uri, route->str_path, strlen(route->str_path))
				      : !strcmp(request.str_uri, route->str_path);

		if (!b_path || (route->str_method && strcmp(request.str_method, route->str_method)))
			continue;

		rc = route->fn(client_fd, read_buf, &request);
		return (rc == ZT_RC_SOCKET) ? ZT_RC_SOCKET : ZT_RC_OK;
	}

	if (!strcmp(request.str_method, "GET") || !strcmp(request.str_method, "HEAD")) {
		rc = hdl_send_static(client_fd, request.str_uri, !strcmp(request.str_method, "HEAD"));
		return (rc == ZT_RC_SOCKET) ? ZT_RC_SOCKET : ZT_RC_OK;
	}

	rc = hdl_send_error(client_fd, HTTP_NOT_FOUND, ZT_ERR_NOT_FOUND);
	return (rc == ZT_RC_SOCKET) ? ZT_RC_SOCKET : ZT_RC_OK;
}

int hdl_socket(int epfd, int client_fd)
{
	hdl_recv_buf_t *rb;

	(void)epfd;

	if (client_fd < 0 || client_fd >= MAX_CLIENTS) {
		LOG_ERR("[hdl_socket] invalid client fd %d\n", client_fd);
		return ZT_RC_ARG_INVALID;
	}

	rb = &g_recv_buf[client_fd];

	while (rb->len < sizeof(rb->buf) - 1) {
		ssize_t n = recv(client_fd, rb->buf + rb->len, sizeof(rb->buf) - 1 - rb->len, 0);

		if (n > 0) {
			rb->len += (size_t)n;
			continue;
		}
		if (n == 0) {
			LOG_MSG("[hdl_socket] FD=%d closed (client disconnect)\n", client_fd);
			rb->len = 0;
			return ZT_RC_FAIL;
		}
		if (errno == EINTR)
			continue;
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			break;

		LOG_MSG("[hdl_socket] read error FD=%d <%d:%s>\n", client_fd, errno, strerror(errno));
		rb->len = 0;
		return ZT_RC_FAIL;
	}
	rb->buf[rb->len] = '\0';

	/* The buffer may hold several pipelined requests. */
	while (rb->len > 0) {
		char req_buf[BUF_MAX_LEN];
		char *hdr_end = strstr(rb->buf, "\r\n\r\n");
		size_t header_len, total;
		long clen;
		int rc;

		if (hdr_end == NULL) {
			if (rb->len >= sizeof(rb->buf) - 1) {
				(void)hdl_send_error(client_fd, HTTP_PAYLOAD_TOO_LARGE, ZT_ERR_TOO_LARGE);
				rb->len = 0;
				return ZT_RC_FAIL;
			}
			return ZT_RC_OK; /* incomplete */
		}

		header_len = (size_t)(hdr_end - rb->buf) + 4;
		clen = hdl_content_length(rb->buf, header_len);
		if (clen < 0 || (size_t)clen > sizeof(rb->buf) - 1 - header_len) {
			(void)hdl_send_error(client_fd, HTTP_PAYLOAD_TOO_LARGE, ZT_ERR_TOO_LARGE);
			rb->len = 0;
			return ZT_RC_FAIL;
		}

		total = header_len + (size_t)clen;
		if (rb->len < total)
			return ZT_RC_OK; /* incomplete */

		memcpy(req_buf, rb->buf, total);
		req_buf[total] = '\0';
		memmove(rb->buf, rb->buf + total, rb->len - total);
		rb->len -= total;
		rb->buf[rb->len] = '\0';

		rc = hdl_dispatch(client_fd, req_buf);
		if (rc != ZT_RC_OK) {
			rb->len = 0;
			return rc;
		}
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

	/* fd-indexed arrays are sized MAX_CLIENTS. */
	if (client_fd >= MAX_CLIENTS) {
		LOG_MSG("[hdl_accept] FD=%d exceeds MAX_CLIENTS(%d), closing\n", client_fd, MAX_CLIENTS);
		close(client_fd);
		return ZT_RC_OK;
	}

	rc = ctx_http_insert(&g_server_ctx, client_fd, client_addr);
	if (rc < 0) {
		LOG_MSG("[hdl_accept] ctx_http_insert fail\n");
		close(client_fd);
		return ZT_RC_CTX;
	}

	g_recv_buf[client_fd].len = 0;

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

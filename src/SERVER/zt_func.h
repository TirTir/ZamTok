#ifndef ZT_FUNC_H
#define ZT_FUNC_H

#include "zt_common.h"

int socket_init(int *sock_out);
int socket_bind(int sock_fd, int port);
int socket_event_loop(int listen_fd);
int socket_set_nonblocking(int sock_fd);
int socket_send(int sock_fd, const char *buf, int len);

int ctx_init(zt_server_ctx_t *ctx);
int ctx_http_insert(zt_server_ctx_t *ctx, int client_fd, struct sockaddr_in client_addr);
int ctx_http_free(zt_server_ctx_t *ctx);

int http_join(int sock_fd, const char *buf, req_t *request);
int http_login(int sock_fd, const char *buf, req_t *request);
int http_create_room(int sock_fd, const char *buf, req_t *request);
int http_search_room(int sock_fd, const char *buf, req_t *request);
int http_join_room(int sock_fd, const char *buf, req_t *request);
int http_list_rooms(int sock_fd, const char *buf, req_t *request);
int http_list_friends(int sock_fd, const char *buf, req_t *request);
int http_invite_room(int sock_fd, const char *buf, req_t *request);

int hdl_accept(int listen_fd, int epfd);
int hdl_socket(int epfd, int client_fd);
int hdl_header(char *header_buf, char *request_buf, int status, req_t *msg);
int hdl_header_mime(char *content_type, int size, const char *uri);
void hdl_bad_request(int client_fd);
int hdl_send_http_json(int client_fd, res_t *response);

#endif

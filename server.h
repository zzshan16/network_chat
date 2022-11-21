#include "sockets.h"
#include "users.h"
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sys/epoll.h>

#define MAX_EVENTS 16
void* init_player_id_stack(int max_players);
int pop_player_id_stack(int* stack);
int push_player_id_stack(int* stack, int id);
void parse_tcp(int fd, char* buf, int epollfd, fd_info_s* fd_info);
int packet_handle(char* packet, int length, int fd, fd_info_s* fd_info);
int send_function(char* packet, int length, char* buf, int* pos, uint16_t x);
void broadcast(char* msg, char* username, unsigned char len);
int start_server(void);
void shift_fd_buf(int fd);
void fd_init(fd_info_s* fd_info);
int* pid_st_p;
int fd_count;
int* fd_arr_p;
fd_info_s** fd_info_p;
char* g_buf;

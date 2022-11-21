#include "server.h"

void shift_fd_buf(int fd){
  int x = fd_count;
  fd_count--;	
  if (*(fd_arr_p+x-1) == fd){
    return;
  }
  for(int i = 0; i < x; ++i){
    if (*(fd_arr_p+i) == fd){
      *(fd_arr_p+i) = *(fd_arr_p+x-1);
      return;
    }
  }
}
int send_function(char* packet, int length, char* buf, int* pos, uint16_t x){
  int space_remaining = 3072 - *pos;
  if (space_remaining - length - 3 < 0){
    puts("insufficient space in buffer");
    return -1;
  }
  uint16_t op = htons(x);
  memcpy(buf + *pos, &op, 2);
  unsigned char size = (length >> 8) ? 255: length;
  *(buf + *pos + 2) = size;
  memcpy(buf + *pos +3, packet, size);
  *pos += size + 3;
  return 0;
}
void broadcast(char* msg, char* username, unsigned char len){
  int t = strlen(username);
  *g_buf = '<';
  memcpy(g_buf+1, username, t);
  *(g_buf+t+1) = '>';
  memcpy(g_buf+t+2, msg, len);
  *(g_buf+t+len+2) = 0;
  len = (strlen(g_buf) +1 >> 8) ? 255 : strlen(g_buf) + 1;
  for(int i = 0; i < fd_count; i++){
    t = *(fd_arr_p + i);
    if (send_function(g_buf, len, (*(fd_info_p+t))->buf+1024, &((*(fd_info_p+t))->wb), 11) < 0){//opcode 11 for chat message
      puts("broadcast error");
    }
    //printf("broadcasting to id %d\n", (*(fd_info_p+t))->id);
  }
  return;
}
void quit_msg(char* msg, char* username, unsigned char len){
  char buf[20] = "-!- ";
  memcpy(g_buf, buf, 4);
  int t = strlen(username);
  memcpy(g_buf+4, username, t);
  strcpy(buf, " has quit [");
  memcpy(g_buf+4+t, buf, 11);
  if (206 > len){
    memcpy(g_buf+t+15, msg, len);
    *(g_buf+t+15+len) = ']';
    *(g_buf+t+16+len) = 0;
  }
  else{
    memcpy(g_buf+t+15, msg, 206);
    *(g_buf+t+221) = ']';
    *(g_buf+t+222) = 0;
  }
  len = (strlen(g_buf) +1 >> 8) ? 255 : strlen(g_buf) + 1;
  //printf("msg is %s, length is %d\n", g_buf, len);
  for(int i = 0; i < fd_count; i++){
    t = *(fd_arr_p + i);
    if (send_function(g_buf, len, (*(fd_info_p+t))->buf+1024, &((*(fd_info_p+t))->wb), 11) < 0){//opcode 11 for chat message
      puts("broadcast error");
    }
  }
  return;
}
void me_msg(char* msg, char* username, unsigned char len){
  char buf[20] = "* ";
  memcpy(g_buf, buf, 2);
  int t = strlen(username);
  memcpy(g_buf+2, username, t);
  *(g_buf+2+t) = ' ';
  if (217 > len){
    memcpy(g_buf+t+3, msg, len);
    *(g_buf+t+3+len) = 0;
  }
  else{
    memcpy(g_buf+t+3, msg, 217);
    *(g_buf+t+220) = 0;
  }
  len = (strlen(g_buf) +1 >> 8) ? 255 : strlen(g_buf) + 1;
  //printf("msg is %s, length is %d\n", g_buf, len);
  for(int i = 0; i < fd_count; i++){
    t = *(fd_arr_p + i);
    if (send_function(g_buf, len, (*(fd_info_p+t))->buf+1024, &((*(fd_info_p+t))->wb), 11) < 0){//opcode 11 for chat message
      puts("broadcast error");
    }
  }
  return;
}
void topic(char* msg, char* username, unsigned char len){
  char buf[40] = "-!- ";
  memcpy(g_buf, buf, 4);
  int t = strlen(username);
  memcpy(g_buf+4, username, t);
  strcpy(buf, " changed the topic to: ");
  memcpy(g_buf+4+t, buf, 23);
  if (195 > len){
    memcpy(g_buf+t+27, msg, len);
    *(g_buf+t+27+len) = 0;
  }
  else{
    memcpy(g_buf+t+27, msg, 195);
    *(g_buf+t+222) = 0;
  }
  int new_len = (strlen(g_buf) +1 >> 8) ? 255 : strlen(g_buf) + 1;
  //printf("msg is %s, length is %d\n", g_buf, len);
  for(int i = 0; i < fd_count; i++){
    t = *(fd_arr_p + i);
    if (send_function(g_buf, new_len, (*(fd_info_p+t))->buf+1024, &((*(fd_info_p+t))->wb), 11u) < 0){//opcode 11 for chat message
      puts("broadcast error");
    }
    if (send_function(msg, len, (*(fd_info_p+t))->buf+1024, &((*(fd_info_p+t))->wb), 15u) < 0){//opcode 11 for chat message
      puts("broadcast error");
    }
  }
  return;
}


int packet_handle(char* packet, int length, int fd, fd_info_s* fd_info){
  unsigned char len = *(packet+2);
  //printf("packet length - header == %u\n", len);
  unsigned int x = *packet << 8;
  x|= *(packet+1);
  //printf("opcode %X \n", x);
  switch(x){
  case 1:
    return 0;
  case 10:;//login
    if (length != 67){
      puts("login error");
      return -1;
    }
    if ((fd_info->flags & (1ul << 1))){//already logged in
      fd_info->flags &= (~(1ul<<1));//unset logged in flag
      push_player_id_stack(pid_st_p, fd_info->id);//allow id to be reused
      fd_info->id = -1;
    }
    //puts("attempting login");
    char userdata[64];
    memcpy(userdata, packet+3, 64);
    if(verify_user(userdata)){
      memcpy(fd_info->username, userdata, 32);
      fd_info->id = pop_player_id_stack(pid_st_p);
      //printf("checking id of %d\n", fd_info->id);
      if (fd_info->id >= 0){
	fd_info->flags |= (1ul << 1);//logged in bit
	sprintf(g_buf, "successfully logged in as %s; current file descriptor is %d\n", fd_info->username, fd);
	send_function(g_buf, strlen(g_buf), (*(fd_info_p + fd))->buf+1024, &((*(fd_info_p+fd))->wb), 12);//opcode 12 for server message
	char a = fd_info->id;
      }
    }
    else{
      puts("login failed");
    }
    break;
  case 11://broadcast in chat
    if (!(fd_info->flags & (1ul << 1))){
      puts("not logged in");
      return -1;//must be logged in
    }
    broadcast(packet+3, fd_info->username, len);
    break;
  case 12:// "/me *"
    if (!(fd_info->flags & (1ul << 1))){
      puts("not logged in");
      return -1;//must be logged in
    }
    me_msg(packet+3, fd_info->username, len);
    break;    
  case 15:// topic
    if (!(fd_info->flags & (1ul << 1))){
      puts("not logged in");
      return -1;//must be logged in
    }
    topic(packet+3, fd_info->username, len);
    break;    
  case 60://logout
    if ((fd_info->flags & (1ul << 1))){//already logged in
      quit_msg(packet+3, fd_info->username, len);
      fd_info->flags &= (~(1ul<<1));//unset logged in flag
      fd_info->flags |= (1u <<2);
      //push_player_id_stack(pid_st_p, fd_info->id);//allow id to be reused
      //fd_info->id = -1;
    }
    break;    
  default:
    printf("invalid packet header %d\n", x);
    //kills fd
    return -1;
  }
  return 1;
}
void parse_tcp(int fd, char* buf, int epollfd, fd_info_s* fd_info){
  char* wb_p = fd_info->buf+1024;
  int wb_length = fd_info->wb;
  int moved = 0;
  unsigned char packet_len;
  if (fd_info->flags & (1u << 2))
    goto kill;
  while (wb_length > 0){
    int t;
    if ((t = write(fd, wb_p, wb_length)) < 0){
      switch(errno){
      case EPIPE:;
      kill:;
	printf("killing fd %d\n", fd);
	if (fd_info->id >= 0)
	  push_player_id_stack(pid_st_p, fd_info->id);
	shift_fd_buf(fd);
	free(fd_info->buf);
	free(fd_info);
	close(fd);
	return;
      case EAGAIN:
	fd_info->wb = wb_length;
	puts("failed to write");
	goto check_read;
      }
    }
    if (t == wb_length){
      fd_info->wb = 0;
      *wb_p = 0;
      //printf("wrote %d bytes to fd %d\n", t, fd);
      goto check_read;
    }
    wb_length -= t;
    memmove(wb_p, wb_p + t, 3072-t);
    *(wb_p+t) = 0;
    //printf("wrote %d bytes to fd %d\n", t, fd);
  }
  
 check_read:;
  char* rb_p = fd_info->buf;
  int t;
  int x = fd_info->rb;
 do_read:
  while (fd_info->rb < 1024){
    if ((t = read(fd, rb_p + fd_info->rb, 1024 - (fd_info->rb))) == -1){
      if (errno == EAGAIN)
	goto done_read;
      goto kill;
    }
    fd_info->rb += t;
    if (moved++ >7){
      //puts("read 7 consecutive times");
      goto kill;
    }
  }
 done_read:
  if (fd_info->rb < 3)
    return;
  packet_len = *(rb_p+2);
  if (fd_info->rb < packet_len +3){
    return;
  }
  if (packet_handle(rb_p,packet_len + 3, fd, fd_info)<0)
    goto kill;
  memmove(rb_p, rb_p+packet_len+3, 1024 - packet_len  - 3);
  fd_info->rb -= packet_len+3;
  if (fd_info->rb > 2){
    goto done_read;
  }
  return;
}
void fd_init(fd_info_s* fd_info){
  fd_info->buf = malloc(4096);
  memset(fd_info->buf, 0, 4096);
  fd_info->rb = 0;
  fd_info->wb = 0;
  fd_info->flags = 1;
  fd_info->id = -1;
  fd_info->request_count = 0;
  return;
}
void* init_player_id_stack(int max_players){
  int *stack = malloc(sizeof(int)*(max_players+1));
  *stack = max_players;
  for(int i = max_players; i > 0; --i){
    *(stack+i) = max_players - i;
  }
  return stack;
}
int pop_player_id_stack(int* stack){
  int x = *stack;
  if (x == 0)
    return -1;
  int y = *(stack+x);
  (*stack)--;
  return y;
}
int push_player_id_stack(int* stack, int id){
  (*stack)++;
  *(stack + *stack) = id;
  return *stack;
}
int start_server(void)
{
  int lfd, cfd, epollfd, nfds;
  pid_st_p = init_player_id_stack(MAX_EVENTS);
  if(!pid_st_p)
    return 0;
  struct epoll_event ev, events[MAX_EVENTS];
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    return 1;//REPLACE IGNORE WITH SIGPIPE HANDLING FUNCTION?
  lfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT_NUMBER);
  addr.sin_addr.s_addr = INADDR_ANY;//does not need htonl for INADDR_ANY
  if (bind(lfd,(const struct sockaddr*) &addr, sizeof(struct sockaddr_in))){
    puts("bind failed");
    return 1;
  }
  if (listen(lfd, 10)){
    puts("listen failed");
    return 1;
  }
  epollfd = epoll_create(1);
  if (epollfd == -1) {
    puts("epoll_create failed");
    return 2;
  }
  ev.events = EPOLLIN;
  ev.data.fd = lfd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, lfd, &ev) == -1) {
    puts("epoll_ctl: lfd");
    return 2;
  }
  char* buf = malloc(4096);
  int fd_arr[MAX_EVENTS];
  fd_arr_p = fd_arr;
  fd_count = 0;
  char* buf_0 = malloc(1024);
  g_buf = buf_0;
  fd_info_s** users = malloc(1024*sizeof(struct fd_info_s *));
  fd_info_p = users;
  int yes = 1;
  ioctl(lfd, FIONBIO, &yes);
  for (;;) {
    nfds = epoll_wait(epollfd, events, MAX_EVENTS, 1);//timeout of 1ms
    if (nfds == -1) {
      puts("epoll_wait");
      return 2;
    }
    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == lfd) {
	if (fd_count < MAX_EVENTS){
	  cfd = accept(lfd, NULL, NULL);
	  setsockopt(cfd, IPPROTO_TCP,TCP_NODELAY,&yes,sizeof(int));
	  fd_arr[fd_count++] = cfd;
	  if (cfd == -1) {
	    printf("errno = %d", errno);
	    puts("accept");
	    return 2;
	  }
	  ioctl(cfd, FIONBIO, &yes);
	  //setsockopt(cfd, IPPROTO_TCP,TCP_NODELAY,&yes,sizeof(int));
	  ev.events = EPOLLIN|EPOLLOUT;//EPOLLET????
	  ev.data.fd = cfd;
	  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, cfd,
			&ev) == -1) {
	    puts("epoll_ctl: cfd");
	    return 2;
	  }
	  *(users+cfd) = malloc(sizeof(fd_info_s));
	  fd_init(*(users+cfd));
	} else{
	  cfd = accept(lfd, NULL, NULL);
	  close(cfd);
	}
      } else {
	parse_tcp(events[n].data.fd, buf, epollfd, *(users+events[n].data.fd));
	(*(users+cfd))->request_count = 0;//????
      }
    }
    usleep(100);
  }
  free(pid_st_p);
  free(buf);
  free(users);
  return 0;
}
int main(int argc, char** argv){
  return start_server();
}

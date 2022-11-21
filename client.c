#include "sockets.h"
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <dlfcn.h>
#define SERVER_IP "127.0.0.1"

/*
 * TO DO
 * implement /quit messages; very easy. ------- DONE
 * implement /me; very easy ------------------- DONE
 * implement /names; easy. store names locally for tab auto-completion?
 * implement /topic; easy. should display topic at the top if set. -------- DONE
 * implement timestamps; need to determine a format to specify the time---- DONE
 * implement logging; (within a module?)
 * implement private messages; perhaps needs hashmap of usernames on the server?
 * implement tab auto-completion; may be difficult. will need names to be working
 * implement method to clear input; ------------------------ DONE
 * implement MOTD
 * move reception of messages and handling of message buffers into a separate function
 * module that allows more lines to be displayed (possibly from log)
 * implement additional login methods
 * implement offline mode?
 * implement join messages
 * module that hides join/part/topic messages
 * 
 */
int packet_handle(unsigned char* read_buf, int* len, unsigned int t, void** arr);
void inthandler(int x);
char* w_buf;
char* read_login(char* filename);
int send_function(char* packet, int length, char* fp, int* pos, uint16_t x);
void setup_message_buffers(void);
void** init_arr(void* wb, void* network_flags);
void** global_arr;
char** message_history;
int message_count;
int message_index;
char* text_input;
int text_len;
int delta;
int present_day;
static struct termios* termios_bak;
void handle_text_command(void);
void check_date_change(struct tm* timeinfo);
void add_time_sec(char* string);
void add_time_min(char* string);
void (*call_once)();
void (*call_repeat)();
static void* dlp;

void* load_fun_fun(char* path, int* once, int* repeat){
  if (dlp)
    dlclose(dlp);
  if (access(path, F_OK))
    return NULL;
  void* dl = dlopen(path, RTLD_LAZY);
  if (!dl)
    return NULL;
  call_once = dlsym(dl, "once");
  call_repeat = dlsym(dl, "repeat");
  if (!call_once)
    *once = 0;
  else
    *once = 1;
  if (!call_repeat)
    *repeat = 0;
  else
    *repeat = 1;
  return dl;
}

void check_date_change(struct tm* timeinfo){
  int t = timeinfo->tm_mday + (timeinfo->tm_mon * 100);
  if (present_day != t){
    //fprintf(fp, "Day changed to DD MM YYYY\n", ....);
    present_day = t;
  }
  return;
}
void add_time_sec(char* string){
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  sprintf(string, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}
void add_time_min(char* string){
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  sprintf(string, "%02d:%02d ", timeinfo->tm_hour, timeinfo->tm_min);
}

void handle_text_command(void){
  if (text_len < 2) return;
  char buf[20];
  memset(&buf, 0, 20);
  switch(*(text_input+1)){
  case 'q'://quit
    memcpy(buf, text_input+1, 4);
    if (strcmp(buf, "quit"))
      break;
    send_function(text_input+6, text_len-6, w_buf, *(global_arr+0), 60u);
    break;
  case 'm'://me, msg
    memcpy(buf, text_input+1, 2);
    if (!strcmp(buf, "me")){
      //send message with /me
      send_function(text_input+4, text_len-4, w_buf, *(global_arr+0), 12u);
      break;
    }
    *(buf+2) = *(text_input+3);
    if (!strcmp(buf, "msg")){
      //attempt to send private message
      break;
    }
    break;
  case 'n'://names
    memcpy(buf, text_input+1, 5);
    if (strcmp(buf, "names"))
      break;
    //send function requesting names
    break;
  case 't'://topic
    memcpy(buf, text_input+1, 5);
    if (!strcmp(buf, "topic")){
      if ( text_len - 7 <= 0)
	break;
      send_function(text_input + 7, text_len - 7, w_buf, *(global_arr+0), 15u);
      break;
    }
    break;
  case 'l'://load
    memcpy(buf, text_input+1, 4);
    if (!strcmp(buf, "load")){
      dlp = load_fun_fun(text_input+6, (int*)global_arr+5, (int*)global_arr+6);
      if (*(global_arr+5))
	call_once();// use _init() instead??
      if (!dlp){
	printf("\r\n");
	printf("failed to load\r\n");
	usleep(100000);
	delta = 0;
      }
    }
    break;
  }
  text_len = 0;
  *text_input = 0;
}
void inthandler(int x){
  printf("\r\n");
  if (!termios_bak)
    exit(0);
  tcsetattr(0, TCSANOW,termios_bak);
  exit(0);
}
void update_fun(void** arr){
  if (*(int*)(arr+8) & 1){
    send_function(text_input, strlen(text_input), w_buf, *(arr+0), 11);
    *(int*)(arr+8) &= ~1u;
    text_len = 0;
    memset(text_input, 0, 256);
    delta = 1;
  }
  if (delta){
    system("clear");
    if (*((char*)*(arr+9))){
      printf("%s\r\n", (char*)*(arr+9));
    }
    else
      printf("\r\n");
    if (message_count){
      for (int i = 7; i >= message_count ; --i){
	printf("%s\r\n", *(message_history+i));
      }
    }
    else{
      for (int i = 7; i >= 0; --i){
	int t = (i + message_index) & 7;
	printf("%s\r\n", *(message_history+t));
      }
    }
    if (text_len){
      printf("%s", text_input);
    }
    fflush(stdout);
    delta = 0;
  }
}
void perform_update(void** arr){
  /* if (*(global_arr+6)){ */
  /*   call_repeat(); */
  /* } */
  if (*(int*)(*(arr+7)) & 1){//this replaces the above. call_repeat no longer needed
    ((void(*)())(*(*(void***)(arr+7) + 4)))(arr);
  }
  else{
    ((void(*)())(*(*(void***)(arr+7) + 3)))(arr); //update_fun(arr);
    usleep(100);
  }
  return;
}
char* read_login(char* filename){
  FILE* fp = fopen(filename, "r");
  char* buf = malloc(64);
  memset(buf, 0, 64);
  fgets(buf, 32, fp);
  fgets(buf+32, 32, fp);
  fclose(fp);
  *(buf+strlen(buf) -1) = '\0';
  *(buf+32+strlen(buf+32) -1) = '\0';
  /* for(int i = 0; i < 64; ++i){ */
  /*   printf("%c", *(buf+i)); */
  /* } */
  /* printf("\n"); */
  return buf;
}
int send_function(char* packet, int length, char* fp, int* pos, uint16_t x){
  if (length < 0)
    length = 0;
  int space_remaining = 4096 - *pos;
  if (space_remaining - length - 3 < 0){
    printf("insufficient space in buffer: space remaining %d, length to be written %d\n",
	   space_remaining, length+3);
    return -1;
  }
  uint16_t op = htons(x);
  memcpy(fp + *pos, &op, 2);
  unsigned char size = (length >> 8) ? 255: length;
  *(fp+ *pos + 2) = size;
  memcpy(fp + *pos +3, packet, size);
  *pos += size + 3;
  return 0;
}
int packet_handle(unsigned char* read_buf, int* len, unsigned int t, void** arr){
  unsigned int x = *read_buf << 8;
  x |= *(read_buf+1);
  //printf("handling packet opcode == %u\n", x);
  switch(x){
  case 11://chat message
    *(read_buf + t +2) = 0;
    if (message_count == 0){//move messages into a seperate function
      message_index = (message_index + 7) & 7;//decrement message_index
      add_time_min(*(message_history+message_index));
      memcpy(*(message_history+message_index) + 6, read_buf+3, t);
      *(*(message_history+message_index) + t + 6) = 0;
    }
    else{
      char* message = malloc(512);
      memset(message, 0, 512);
      add_time_min(message);
      memcpy(message+6, read_buf+3, t);
      *(message+t+6) = 0;
      *(message_history+(--message_count)) = message;
      message_index = (message_index + 7) & 7;//decrement message_index
    }
    *len -= t+3;
    delta = 1;
    break;
  case 12:
    *(read_buf + t +2) = 0;
    system("clear");
    printf("SERVER MESSAGE %s\r\n", read_buf+3);
    usleep(400000);
    *len -= t+3;
    break;
  case 15:
    memcpy(*(arr+9), read_buf+3, t);
    *((char*)*(arr+9)+t) = 0;
    *len -= t+3;
    break;
  default:
    if (*(int*)(*(arr+7)) & 8){
      ((void(*)())(*(*(void***)(arr+7) + 7)))(read_buf, len, t, arr);
    }
    else{
      puts("error processing packet");
      *len = 0;
    }
  }
  return t+3;
}
void setup_message_buffers(void){
  message_history = malloc(8*sizeof(char*));
  memset(message_history, 0, sizeof(char*));
  message_index = 0;
  message_count = 8;
}
void** init_arr(void* wb, void* network_flags){
  void** arr = malloc(16*sizeof(void*));
  *(arr+0) = wb;
  *(arr+1) = network_flags;
  *(arr+2) = malloc(4096);
  w_buf = *(arr+2);
  *(int*)(arr+5) = 0;//call once
  *(int*)(arr+6) = 0;//call repeat
  *(arr+7) = malloc(16*sizeof(void*));//function pointer arr
  *(int*)(*(void***)(arr+7)) = 0;// 1 is for update fun, 4 is for receiving packets
  *(*(void***)(arr+7) + 1) = send_function;
  *(*(void***)(arr+7) + 2) = NULL;
  *(*(void***)(arr+7) + 3) = update_fun;
  *(*(void***)(arr+7) + 4) = NULL;//alternative update function
  *(*(void***)(arr+7) + 5) = handle_text_command;
  *(*(void***)(arr+7) + 6) = NULL;//for alternative /commands handling
  *(*(void***)(arr+7) + 7) = NULL;//used for receiving packets
  *(int*)(arr+8) = 0;
  *(arr+9) = malloc(1024);//topic buffer
  memset(*(arr+9), 0, 1024);
  global_arr = arr;
  text_len = 0;
  text_input = malloc(256);
  memset(text_input, 0, 256);
  delta = 0;
  present_day = 0;
  dlp = 0;
  return arr;
}
void connection_loop(int fd, char* login_data){
  setup_message_buffers();
  char read_buf[2048];
  int rb,wb,x;
  unsigned int t;
  int network_flags = 0;
  void** arr = init_arr(&wb, &network_flags);
  wb = rb = 0;
  struct pollfd fd_poll;
  fd_poll.fd = fd;
  fd_poll.events = POLLIN|POLLOUT;
  send_function(login_data, 64, w_buf, *(arr+0), 10u);
  for(;;){
    if ((x=poll(&fd_poll,1,0)) < 0){//0ms timeout
      puts("error: poll returned value less than 0");
      break;
    }
    if (x > 0){
      if (fd_poll.revents & POLLIN){
	if((x = read(fd, read_buf, 2048 - rb)) < 0){
	  if (errno != EAGAIN){
	    puts("error reading from pipe");
	    break;
	  }
	}
	else{
	  rb += x;
	}
      check_read:
	if (rb >= 3){
	  t = *(unsigned char*)(read_buf+2);
	  if (rb < t + 3){
	    goto check_pollout;
	  }
	  x = packet_handle(read_buf, &rb,t, arr);
	  if (x < 0){
	    puts("packet handling error;");
	    break;
	  }
	  memmove(read_buf, read_buf + x, 2048 - x);
	  //printf("handled %d bytes\n", x);
	  goto check_read;
	}
      }
    check_pollout:
      if (fd_poll.revents & POLLOUT){
	if (wb > 0){
	  if((x = write(fd, w_buf, wb))< 0){
	    switch(errno){
	    case EAGAIN:
	      break;
	    case EPIPE:
	      puts("connection terminated");
	      goto end;
	    default:
	      puts("unknown error writing to pipe");
	      goto end;
	    }
	  }
	  else{
	    memmove(w_buf, w_buf + x, 4096 -x);
	    wb -= x;
	    //printf("sent %d bytes\n", x);
	  }
	}
      }
      if (fd_poll.revents & (POLLERR|POLLHUP)){
	printf("connection lost\r");
	break;
	//terminate connection
      }
    }
    char temp_buf[50];
    int n = read(0, temp_buf,50);
    if (n > 0){
      delta = 1;
      int i = 0;
      while (i < n){
	char j = *(temp_buf+i);
	switch(j){
	/* case 0x03: */
	/*   printf("quitting\r\n"); */
	/*   goto end; */
	case 0x0D://enter
	  i = n;
	  if (*text_input != '/')
	    *(int*)(global_arr+8) |= 1;
	  else{
	    if (*(int*)(*(void***)(arr+7)) & 2){
	      ((void(*)())(*(*(void***)(arr+7) + 6)))();
	    }
	    else{
	      handle_text_command();
	    }
	  }
	  break;
	case 0x7F://backspace
	  if (text_len)
	    *(text_input+--text_len) = 0;
	  break;
	case 0x09://tab?
	  printf("tab pressed\r\n");
	  delta = 0;
	  break;
	case 0x08:
	  *(text_input) = 0;
	  text_len = 0;
	  break;
	case 0x1B://skips ctrl+insert?
	  i+=3;
	  break;
	default:
	  /* printf("%x\r\n", j); */
	  /* delta = 0; */
	  if (text_len < 240){
	    /* if (j >= 0x20 && j <= 0x7E){ */
	    if (j >= 0x20){
	      *(text_input+text_len++) = j;
	      *(text_input+text_len) = 0;
	    }
	  }
	}
	++i;
      }
      
    }
    perform_update(arr);
  }
 end:
  if (dlp)
    dlclose(dlp);
  close(fd);
  tcsetattr(0, TCSANOW,termios_bak);
  free(w_buf);
  free(*(arr+7));
  free(arr);
  for(int i = 0; i< 8; ++i){
    free(*(message_history+i));
  }
  free(message_history);
}
int start_client(char* login_data){
  system("clear");
  signal(SIGINT, inthandler);
  struct termios termios_data;
  termios_bak = malloc(sizeof(struct termios));
  tcgetattr(0,&termios_data);
  memcpy(termios_bak, &termios_data, sizeof(struct termios));
  cfmakeraw(&termios_data);
  tcsetattr(0, TCSANOW,&termios_data);
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT_NUMBER);
  addr.sin_addr.s_addr = inet_addr(SERVER_IP);
  if (connect(fd, (struct sockaddr*) &addr, sizeof(struct sockaddr_in))){
    printf("failed to connect; errno: %d\r\n", errno);
    tcsetattr(0, TCSANOW,termios_bak);
    return 1;
  }
  int yes = 1;
  ioctl(fd, FIONBIO, &yes);
  ioctl(0, FIONBIO, &yes);//set stdin to non-block
  setsockopt(fd, IPPROTO_TCP,TCP_NODELAY,&yes,sizeof(int));
  connection_loop(fd, login_data);
  return 0;
}
int main(int argc, char** argv){
  if (argc !=2){
    puts("./client login_data");
    return 1;
  }
  char* login_data = read_login(*(argv+1));
  start_client(login_data);
  free(login_data);
  return 0;
}

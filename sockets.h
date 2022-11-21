#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/tcp.h>

#define PORT_NUMBER 8887
typedef struct fd_info_s{
  unsigned int flags;
  unsigned int rb;
  unsigned int wb;
  unsigned int request_count;
  char id;
  char username[32];
  char* buf;
  void* temp;
} fd_info_s;



#include "users.h"
int verify_user(char *userdata){
  sanitize_32(userdata);
  //sanitize_32(userdata+32);//is this necessary???
  int fd = open("./users", O_RDWR|O_APPEND);
  struct stat statbuf;
  fstat(fd, &statbuf);
  long signed int filesize = statbuf.st_size;
  int num_read;
  char charbuf[4096];
  while (filesize > 63){
    num_read = read(fd, charbuf, 4096);
    for(int i = 0; i < num_read; i += 64){
      if(!memcmp(charbuf+i, userdata, 64))
	return 1;
    }
    filesize -= num_read;
  }
  //auto register
  if (statbuf.st_size < 40000){
    //check if username is taken
    int name_in_use = 0;
    int usersfd = open("./known_users", O_RDWR|O_APPEND);
    while(num_read = read(usersfd, charbuf, 4096)){
      for(int i = 0; i < num_read; i += 32){
	if(!memcmp(charbuf+i, userdata, 32))
	  return 0;
      }
    }
    write(usersfd, userdata, 32);
    close(usersfd);
    write(fd, userdata, 64);
    close(fd);
    return 1;
  }
  close(fd);
  return 0;
}
void sanitize_32(char* string){
  for(int i = 0; i < 31; i++){
    switch(*(string+i)){
    case 'a':
      *(string+i) = 'a';
      break;
    case 'b':
      *(string+i) = 'b';
      break;
    case 'c':
      *(string+i) = 'c';
      break;
    case 'd':
      *(string+i) = 'd';
      break;
    case 'e':
      *(string+i) = 'e';
      break;
    case 'f':
      *(string+i) = 'f';
      break;
    case 'g':
      *(string+i) = 'g';
      break;
    case 'h':
      *(string+i) = 'h';
      break;
    case 'i':
      *(string+i) = 'i';
      break;
    case 'j':
      *(string+i) = 'j';
      break;
    case 'k':
      *(string+i) = 'k';
      break;
    case 'l':
      *(string+i) = 'l';
      break;
    case 'm':
      *(string+i) = 'm';
      break;
    case 'n':
      *(string+i) = 'n';
      break;
    case 'o':
      *(string+i) = 'o';
      break;
    case 'p':
      *(string+i) = 'p';
      break;
    case 'q':
      *(string+i) = 'q';
      break;
    case 'r':
      *(string+i) = 'r';
      break;
    case 's':
      *(string+i) = 's';
      break;
    case 't':
      *(string+i) = 't';
      break;
    case 'u':
      *(string+i) = 'u';
      break;
    case 'v':
      *(string+i) = 'v';
      break;
    case 'w':
      *(string+i) = 'w';
      break;
    case 'x':
      *(string+i) = 'x';
      break;
    case 'y':
      *(string+i) = 'y';
      break;
    case 'z':
      *(string+i) = 'z';
      break;
    case '0':
      *(string+i) = '0';
      break;
    case '9':
      *(string+i) = '9';
      break;
    case '8':
      *(string+i) = '8';
      break;
    case '7':
      *(string+i) = '7';
      break;
    case '6':
      *(string+i) = '6';
      break;
    case '5':
      *(string+i) = '5';
      break;
    case '4':
      *(string+i) = '4';
      break;
    case '3':
      *(string+i) = '3';
      break;
    case '2':
      *(string+i) = '2';
      break;
    case '1':
      *(string+i) = '1';
      break;
    case 'A':
      *(string+i) = 'A';
      break;
    case 'B':
      *(string+i) = 'B';
      break;
    case 'C':
      *(string+i) = 'C';
      break;
    case 'D':
      *(string+i) = 'D';
      break;
    case 'E':
      *(string+i) = 'E';
      break;
    case 'F':
      *(string+i) = 'F';
      break;
    case 'G':
      *(string+i) = 'G';
      break;
    case 'H':
      *(string+i) = 'H';
      break;
    case 'I':
      *(string+i) = 'I';
      break;
    case 'J':
      *(string+i) = 'J';
      break;
    case 'K':
      *(string+i) = 'K';
      break;
    case 'L':
      *(string+i) = 'L';
      break;
    case 'M':
      *(string+i) = 'M';
      break;
    case 'N':
      *(string+i) = 'N';
      break;
    case 'O':
      *(string+i) = 'O';
      break;
    case 'P':
      *(string+i) = 'P';
      break;
    case 'Q':
      *(string+i) = 'Q';
      break;
    case 'R':
      *(string+i) = 'R';
      break;
    case 'S':
      *(string+i) = 'S';
      break;
    case 'T':
      *(string+i) = 'T';
      break;
    case 'U':
      *(string+i) = 'U';
      break;
    case 'V':
      *(string+i) = 'V';
      break;
    case 'W':
      *(string+i) = 'W';
      break;
    case 'X':
      *(string+i) = 'X';
      break;
    case 'Y':
      *(string+i) = 'Y';
      break;
    case 'Z':
      *(string+i) = 'Z';
      break;
    case '_':
      *(string+i) = '_';
      break;
    case '.':
      *(string+i) = '.';
      break;
    case ',':
      *(string+i) = ',';
      break;
    case '>':
      *(string+i) = '>';
      break;
    case '<':
      *(string+i) = '<';
      break;
    case ':':
      *(string+i) = ':';
      break;
    case '@':
      *(string+i) = '@';
      break;
    case '\0':
      return;
    default:
      *(string+i) = '?';
    }
  }
  *(string+31) = '\0';
  return;
}


#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

int main()
{
  char *host = "www.baidu.com";

  struct hostent *hostinfo = gethostbyname(host);
  printf("hostinfo: h_name =  %s, h_aliases = %s, h_addrtype = %d, h_length = %d\n", hostinfo->h_name, hostinfo->h_aliases[0], hostinfo->h_addrtype, hostinfo->h_length);
  for (int i = 0; i < hostinfo->h_length; i++)
  {
    printf("hostaddr[%d] = %s\n", i, hostinfo->h_addr_list[i]);
  }
  assert(hostinfo);

  struct servent *servinfo = getservbyname("daytime", "tcp");
  assert(servinfo);
  printf("daytime port is %d\n", ntohs(servinfo->s_port));

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = servinfo->s_port;
  address.sin_addr = *(struct in_addr *)*hostinfo->h_addr_list;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  int result = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
  assert(result != -1);

  char buffer[128];
  result = read(sockfd, buffer, sizeof(buffer));
  assert(result > 0);
  buffer[result] = '\0';
  printf("the day tiem is: %s", buffer);
  close(sockfd);
  return 0;
}

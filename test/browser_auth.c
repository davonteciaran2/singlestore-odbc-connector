/************************************************************************************
   Copyright (C) 2013,2018 MariaDB Corporation AB
                 2021 SingleStore, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc.,
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*************************************************************************************/

#include "ma_odbc.h"
#include "plugins/browser_auth.h"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#endif

#ifdef WIN32
#define SOCKET_ SOCKET
#else
#define SOCKET_ int
#endif

#define BUFFER_SIZE 2048
#define TOKEN "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJlbWFpbCI6InRlc3QtZW1haWxAZ21haWwuY29tIiwiZGJVc2VybmFtZSI6InRlc3QtdXNlciIsImV4cCI6MTkxNjIzOTAyMn0.kQPJ2yLs8-G5bUuYBddmyKGQmaimVop2mptZ5IqtF3c"
#define HTTP_204 "HTTP/1.1 204 No Content\r\nAccess-Control-Allow-Origin: *\r\n\r\n"

#define CONTENT_LENGTH "Content-Length: "
long tryGetFullRequestLength(const char *request)
{
  char *bodyStart;
  char *contentLengthHeader;
  long headersLen;
  long contentLen;

  if (!(bodyStart = strstr(request, "\r\n\r\n")))
  {
    // request doesn't contain all headers
    return -1;
  }
  bodyStart += 4;
  headersLen = bodyStart - request;

  if (!(contentLengthHeader = strstr(request, CONTENT_LENGTH)))
  {
    // No Content-Length header
    // Assuming that the body is empty
    return headersLen;
  }
  contentLengthHeader += strlen(CONTENT_LENGTH);
  contentLen = strtol(contentLengthHeader, NULL, 10);

  return headersLen + contentLen;
}

int makeSocketNonBlocking(SOCKET_ socket)
{
#ifdef WIN32
  u_long mode = 1;  // 1 to enable non-blocking socket
  if (ioctlsocket(socket, FIONBIO, &mode))
  {
    return 1;
  }
#else
  int flags;
  if ((flags = fcntl(socket, F_GETFL)) == -1)
  {
    return 1;
  }
  if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1)
  {
    return 1;
  }
#endif

  return 0;
}

int invalidSocketCheck(SOCKET_ s)
{
#ifdef WIN32
  return s == INVALID_SOCKET;
#else
  return s < 0;
#endif
}

int closeSocket(SOCKET_ s)
{
#ifdef WIN32
  closesocket(s);
#else
  close(s);
#endif
}

int isBlockingError()
{
#ifdef WIN32
  return WSAGetLastError() == WSAEWOULDBLOCK;
#else
  return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

SOCKET_ startMockPortal()
{
  struct sockaddr_in serverAddress;
  SOCKET_ serverSocket;
  int res;

  // Socket setup: creates an endpoint for communication, returns a descriptor
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  assert(!invalidSocketCheck(serverSocket) && "Failed to create socket");

  // Construct local address structure
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(18087);
  serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1;

  // Bind socket to local address
  res = bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
  assert(!res && "Failed to bind socket");

  // Mark socket to listen for incoming connections
  res = listen(serverSocket, 10);
  assert(!res && "Failed to mark socket to listen for incoming connections");

  return serverSocket;
}

void sendJWT(char *url)
{
  char command[BUFFER_SIZE];

  memset(command ,0 , BUFFER_SIZE);
  strcat(command, "curl -X POST -d '");
  strcat(command, TOKEN);
  strcat(command, "' ");
  strcat(command, url);

  system(command);
}

void sleepMicroseconds(int microseconds)
{
#ifdef WIN32
  Sleep(microseconds);
#else
  usleep(microseconds);
#endif
}

#ifdef WIN32
DWORD WINAPI
#else
void *
#endif
handle(void *serverSocketVoid)
{
  printf("handle 0\n");
  fflush(stdout);

  SOCKET_ clientSocket, *serverSocket;
  char buff[BUFFER_SIZE];
  char request[BUFFER_SIZE];
  char *umlStart, *umlEnd;
  int size_recv;
  int length;
  long fullRequestLength = -1;

  printf("handle 1\n");
  fflush(stdout);
  serverSocket = (SOCKET_*)serverSocketVoid;

  printf("handle 2\n");
  fflush(stdout);
  // Accept socket
  clientSocket = accept(*serverSocket, NULL, NULL);
  assert(!invalidSocketCheck(clientSocket) && "Failed to accept the connection");

  printf("handle 3\n");
  fflush(stdout);
  // Read the result
  memset(buff ,0 , BUFFER_SIZE);
  memset(request ,0 , BUFFER_SIZE);
  printf("handle 33\n");
  fflush(stdout);

  makeSocketNonBlocking(clientSocket);
  while(fullRequestLength == -1 || length < fullRequestLength)
  {
    memset(buff, 0 , BUFFER_SIZE);
    size_recv = recv(clientSocket, buff, BUFFER_SIZE-1, 0);
    if (size_recv < 0)
    {
      assert(isBlockingError() && "Error while reading request");
      sleepMicroseconds(10);
      continue;
    }
    length += size_recv;

    strcat(request, buff);

    if (fullRequestLength == -1)
    {
      fullRequestLength = tryGetFullRequestLength(request);
    }
  }
  printf("handle 34 %d\n", length);
  fflush(stdout);
  assert(length >= 0 && "Failed to read the response");

  printf("handle 4\n");
  fflush(stdout);
  // Parse port from the request
  umlStart = strstr(request, "returnTo=");
  assert(umlStart && "Wrong request");
  umlStart += strlen("returnTo=");
  umlEnd = strstr(umlStart, "&");
  assert(umlEnd && "Wrong request");
  *umlEnd = 0;

  printf("handle 5\n");
  fflush(stdout);
  // Answer and close socket
  send(clientSocket, HTTP_204, sizeof(HTTP_204), 0);
  closeSocket(clientSocket);
  printf("handle 6\n");
  fflush(stdout);

  // JWT to the driver
  sendJWT(umlStart);
}

int main(int argc, char **argv)
{
  SQLHANDLE henv, hdbc;
  BrowserAuthCredentials creds;
  SOCKET_ serverSocket;
  int res;

  printf("main 0\n");
  fflush(stdout);
#ifdef WIN32
  HANDLE thread;
#else
  pthread_t thread;
#endif
  printf("main 1\n");
  fflush(stdout);
  SQLAllocHandle(SQL_HANDLE_ENV, NULL, &henv);
  SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC2, SQL_IS_INTEGER);
  SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

  printf("main 2\n");
  fflush(stdout);
  serverSocket = startMockPortal();
#ifdef WIN32
  thread = CreateThread(NULL, 0, handle, &serverSocket, 0, NULL);
#else
  pthread_create(&thread, NULL, handle, &serverSocket);
#endif

  printf("main 3\n");
  fflush(stdout);
  res = BrowserAuthInternal(hdbc, "test-email@gmail.com", "http://127.0.0.1:18087", 1, &creds);
  printf("main 4\n");
  fflush(stdout);
  assert(!res && "Browser authentication failed");
#ifdef WIN32
  WaitForSingleObject(thread, INFINITE);
#else
  printf("main 5\n");
  fflush(stdout);
  pthread_join(thread, NULL);
  printf("main 6\n");
  fflush(stdout);
#endif
  closeSocket(serverSocket);

  printf("main 7\n");
  fflush(stdout);
  assert(!strcmp(creds.email, "test-email@gmail.com") && "Wrong email");
  assert(!strcmp(creds.token, TOKEN) && "Wrong token");
  assert(!strcmp(creds.username, "test-user") && "Wrong username");
  assert(creds.expiration == 1916239022 && "Wrong exp");

  printf("main 8\n");
  fflush(stdout);
  BrowserAuthCredentialsFree(&creds);

  // Test that BrowserAuth fails when server is not responding
  res = BrowserAuthInternal(hdbc, "test-email@gmail.com", "http://127.0.0.1:18087", 1, &creds);
  assert(res && "Browser authentication expected to fail");

  SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  SQLFreeHandle(SQL_HANDLE_ENV, henv);

  printf("main 9\n");
  fflush(stdout);
  return 0;
}

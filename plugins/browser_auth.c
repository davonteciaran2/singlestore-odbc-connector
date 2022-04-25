/************************************************************************************
   Copyright (C) 2021 SingleStore, Inc.

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

#include "browser_auth.h"
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#endif
#include <vendor/b64.c/b64.h>
#include <vendor/cJSON/cJSON.h>

int getJsonStringField(MADB_Dbc *Dbc, cJSON *json, const char *fieldName, char **dst)
{
  cJSON *field;
  field = cJSON_GetObjectItem(json, fieldName);
  if (!cJSON_IsString(field) || (field->valuestring == NULL)) {
    return MADB_SetError(&Dbc->Error, MADB_ERR_28000, "Failed to parse browser authentication result", 0);
  }
  if (!(*dst = strdup(field->valuestring)))
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }
  return 0;
}

int getJsonIntField(MADB_Dbc *Dbc, cJSON *json, const char *fieldName, int *dst)
{
  cJSON *field;
  field = cJSON_GetObjectItem(json, fieldName);
  if (!cJSON_IsNumber(field)) {
    return MADB_SetError(&Dbc->Error, MADB_ERR_28000, "Failed to parse browser authentication result", 0);
  }
  *dst = field->valueint;
  return 0;
}

void BrowserAuthCredentialsFree(BrowserAuthCredentials *bac)
{
  if (bac) {
    MADB_FREE(bac->token);
    MADB_FREE(bac->username);
    MADB_FREE(bac->email);
  }
}

#ifdef WIN32
#define SOCKET_ SOCKET
#else
#define SOCKET_ int
#endif

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

int getOpenBrowserCommand(MADB_Dbc *Dbc, char *returnTo, char *email, char *endpoint, MADB_DynString *openBrowserCommand)
{
#ifdef WIN32
  if (MADB_DynstrAppend(openBrowserCommand, "START "))
#elif __APPLE__
  if (MADB_DynstrAppend(openBrowserCommand, "open "))
#else
  if (MADB_DynstrAppend(openBrowserCommand, "xdg-open "))
#endif
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }
  if (MADB_DynstrAppend(openBrowserCommand, "\"") ||
      MADB_DynstrAppend(openBrowserCommand, endpoint == NULL ? PORTAL_SSO_ENDPOINT: endpoint) ||
      MADB_DynstrAppend(openBrowserCommand, "?returnTo=") ||
      MADB_DynstrAppend(openBrowserCommand, returnTo) ||
      (email &&
       (MADB_DynstrAppend(openBrowserCommand, "&email=") ||
        MADB_DynstrAppend(openBrowserCommand, email))) ||
      MADB_DynstrAppend(openBrowserCommand, "\""))
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }

#ifndef WIN32
  if (MADB_DynstrAppend(openBrowserCommand, " &"))
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }
#endif

  return 0;
}

int startLocalHttpServer(MADB_Dbc *Dbc, SOCKET_ *serverSocket, MADB_DynString *serverEndpoint)
{
  char portStr[10];
  struct sockaddr_in serverAddress;
  socklen_t addrLen;

  // Socket setup: creates an endpoint for communication, returns a descriptor
  if (invalidSocketCheck(*serverSocket = socket(AF_INET, SOCK_STREAM, 0)))
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to create socket", 0);
  }

  // Construct local address structure
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = 0; // choose any free port
  serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1;

  // Bind socket to local address
  if (bind(*serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to bind socket for browser authentication", 0);
    goto cleanupSocket;
  }

  // Get information about server address
  addrLen = sizeof(serverAddress);
  if (getsockname(*serverSocket, (struct sockaddr *)&serverAddress, &addrLen)) {
    MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to get information about socket address for browser authentication", 0);
    goto cleanupSocket;
  }
  sprintf(portStr, "%d", ntohs(serverAddress.sin_port));

  // Mark socket to listen for incoming connections
  // 10 is the maximum length to which the queue of pending connections for sockfd may grow
  if (listen(*serverSocket, 10))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to mark socket to listen for incoming connections for browser authentication", 0);
    goto cleanupSocket;
  }

  // Generate server endpoint
  if (MADB_DynstrAppend(serverEndpoint, "http://127.0.0.1:")  || MADB_DynstrAppend(serverEndpoint, portStr))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
    goto cleanupSocket;
  }

  return 0;

cleanupSocket:
  closeSocket(*serverSocket);
  return Dbc->Error.ReturnValue;
}

int parseCredentialsFromJson(MADB_Dbc *Dbc, cJSON *json, const char *jwt, BrowserAuthCredentials *credentials)
{
  memset(credentials, 0, sizeof(BrowserAuthCredentials));

  if (getJsonStringField(Dbc, json, "email", &credentials->email) ||
      getJsonStringField(Dbc, json, "dbUsername", &credentials->username) ||
      getJsonIntField(Dbc, json, "exp", &credentials->expiration))
  {
    goto error;
  }

  if (!(credentials->token = strdup(jwt)))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_HY000, NULL, 0);
    goto error;
  }

  return 0;

error:
  BrowserAuthCredentialsFree(credentials);
  return Dbc->Error.ReturnValue;
}

// parseRequest parses HTTP request and retrieves credentials from it
//
// Request contains status line, zero or more header lines terminated by "\r\n" each.
// Then one line with "\r\n" and request body.
// <status>\r\n
// <header>\r\n
// ...
// <header>\r\n
// \r\n
// <body>
//
// Body of the request should contain JWT.
// JWT consists of three parts separated by dot "."
// <header>.<payload>.<verify signature>
//
// Second part of the JWT contains base64URL encoded JSON
// This JSON should contain all fields of BrowserAuthCredentials
int parseRequest(MADB_Dbc *Dbc, const char *request, BrowserAuthCredentials *credentials)
{
  const char *bodyStart;
  const char *jwtPayloadStart;
  const char *jwtPayloadEnd;
  const char *base64URL;
  char *base64;
  char *jsonStr;
  long base64URLLen;
  long base64Len;
  cJSON *json;
  int i;

  if ((bodyStart = strstr(request, "\r\n\r\n")) == NULL)
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to parse browser authentication credentials: http request doesn't contain body", 0);
  }
  bodyStart += 4;

  if ((jwtPayloadStart = strstr(bodyStart, ".")) == NULL)
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to parse browser authentication credentials: JWT contain only header", 0);
  }
  jwtPayloadStart++;

  if ((jwtPayloadEnd = strstr(jwtPayloadStart, ".")) == NULL)
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to parse browser authentication credentials: JWT contain only header and payload", 0);
  }

  base64URLLen = jwtPayloadEnd - jwtPayloadStart;
  base64Len = ((base64URLLen + 3) / 4) * 4;

  if (!(base64 = MADB_CALLOC(base64Len+1)))
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_HY000, NULL, 0);
  }
  base64URL = jwtPayloadStart;
  for (i = 0; i < base64Len; i++)
  {
    if (i < base64URLLen)
    {
      if (base64URL[i] == '-')
      {
        base64[i] = '+';
      } else if (base64URL[i] == '_')
      {
        base64[i] = '/';
      } else
      {
        base64[i] = base64URL[i];
      }
    } else
    {
      base64[i] = '=';
    }
  }

  if (!(jsonStr = (char *)b64_decode(base64, base64Len)))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to parse base64 string for browser authentication", 0);
    goto cleanup;
  }

  json = cJSON_Parse(jsonStr);
  if (json == NULL || !cJSON_IsObject(json))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to parse JSON string for browser authentication", 0);
    goto cleanup;
  }

  if (parseCredentialsFromJson(Dbc, json, bodyStart, credentials))
  {
    goto cleanup;
  }

cleanup:
  MADB_FREE(base64);
  MADB_FREE(jsonStr);
  cJSON_Delete(json);

  return Dbc->Error.ReturnValue;
}

// tryGetFullRequestLength takes request prefix and if it contains all headers
// then it retrieves Content-Length header and computes full length of the request
// If Content-Length header is not presented - it assumes that the body is empty
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

int isBlockingError()
{
#ifdef WIN32
  return WSAGetLastError() == WSAEWOULDBLOCK;
#else
  return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

void sleepMicroseconds(int microseconds)
{
#ifdef WIN32
  Sleep(microseconds);
#else
  usleep(microseconds);
#endif
}

#define BUFFER_SIZE 2048
#define HTTP_204 "HTTP/1.1 204 No Content\r\nAccess-Control-Allow-Origin: *\r\n\r\n"
#define HTTP_400 "HTTP/1.1 400 Bad Request\r\nAccess-Control-Allow-Origin: *\r\n\r\n"
#define HTTP_500 "HTTP/1.1 500 Internal Server Error\r\nAccess-Control-Allow-Origin: *\r\n\r\n"
int readRequest(MADB_Dbc *Dbc, SOCKET_ serverSocket, int requestReadTimeoutSec, BrowserAuthCredentials *credentials)
{
  printf("readRequest1\n");
  fflush(stdout);
  int size_recv;
  char buff[BUFFER_SIZE];
  SOCKET_ clientSocket;
  MADB_DynString request;
  long fullRequestLength = -1;
  clock_t startTime;

  startTime = clock();

  printf("readRequest2\n");
  fflush(stdout);
  if (MADB_InitDynamicString(&request, "", BUFFER_SIZE, BUFFER_SIZE))
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }

  // Change serverSocket to be non blocking
  // This is needed in order to don't wait for request on the accept call
  // If request is not send, accept will return an error that can be checked using isBlockingError function
  makeSocketNonBlocking(serverSocket);
  printf("readRequest3\n");
  fflush(stdout);
  while (invalidSocketCheck(clientSocket = accept(serverSocket, NULL, NULL)))
  {
    printf("readRequest4\n");
    fflush(stdout);
    if (isBlockingError())
    {
      printf("readRequest41\n");
      fflush(stdout);
      if ((clock() - startTime)/CLOCKS_PER_SEC > requestReadTimeoutSec)
      {
        printf("readRequest42\n");
        fflush(stdout);
        MADB_SetError(&Dbc->Error, MADB_ERR_HYT00, "Browser authentication response timeout expired", 0);
        send(clientSocket, HTTP_400, sizeof(HTTP_400), 0);
        printf("readRequest421 %d\n", Dbc->Error.ReturnValue);
        fflush(stdout);
        goto cleanupRequest;
      }

      sleepMicroseconds(10);
    } else
    {
      printf("readRequest43\n");
      fflush(stdout);
      MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to accept the connection for browser authentication", 0);
      goto cleanupRequest;
    }
  }
  printf("readRequest5\n");
  fflush(stdout);

  // Change clientSocket to be non blocking
  // This is needed in order to don't wait for data on the recv call
  // If data is not, send recv will return an error that can be checked using isBlockingError function
  makeSocketNonBlocking(clientSocket);
  printf("readRequest6\n");
  fflush(stdout);
  while(fullRequestLength == -1 || request.length < fullRequestLength)
  {
    printf("readRequest7\n");
    fflush(stdout);
    memset(buff, 0 , BUFFER_SIZE);
    size_recv = recv(clientSocket, buff, BUFFER_SIZE-1, 0);
    if (size_recv < 0)
    {
      printf("readRequest71\n");
      fflush(stdout);
      if (isBlockingError())
      {
        printf("readRequest72\n");
        fflush(stdout);
        if ((clock() - startTime)/CLOCKS_PER_SEC > requestReadTimeoutSec)
        {
          printf("readRequest73\n");
          fflush(stdout);
          MADB_SetError(&Dbc->Error, MADB_ERR_HYT00, "Browser authentication response timeout expired", 0);
          send(clientSocket, HTTP_400, sizeof(HTTP_400), 0);
          goto cleanupSocket;
        }

        sleepMicroseconds(10);
        continue;
      } else
      {
        printf("readRequest74\n");
        fflush(stdout);
        MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to read data from socket for browser authentication", 0);
        send(clientSocket, HTTP_400, sizeof(HTTP_400), 0);
        goto cleanupSocket;
      }
    }

    printf("readRequest8\n");
    fflush(stdout);
    if (MADB_DynstrAppend(&request, buff))
    {
      MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
      send(clientSocket, HTTP_500, sizeof(HTTP_500), 0);
      goto cleanupSocket;
    }

    printf("readRequest9\n");
    fflush(stdout);
    if (fullRequestLength == -1)
    {
      fullRequestLength = tryGetFullRequestLength(request.str);
    }
  }

  printf("readRequest10\n");
  fflush(stdout);
  if (parseRequest(Dbc, request.str, credentials))
  {
    send(clientSocket, HTTP_400, sizeof(HTTP_400), 0);
    goto cleanupSocket;
  }

  printf("readRequest11\n");
  fflush(stdout);
  size_recv = send(clientSocket, HTTP_204, sizeof(HTTP_204), 0);
  printf("readRequest %d\n", size_recv);
  fflush(stdout);

cleanupSocket:
  closeSocket(clientSocket);
cleanupRequest:
  MADB_DynstrFree(&request);
  return Dbc->Error.ReturnValue;
}

int BrowserAuthInternal(MADB_Dbc *Dbc, char *email, char *endpoint, int requestReadTimeoutSec, BrowserAuthCredentials *credentials)
{
  printf("BrowserAuth 0\n");
  fflush(stdout);

  MADB_DynString serverEndpoint;
  MADB_DynString openBrowserCommand;
  SOCKET_ serverSocket;

  printf("BrowserAuth 1\n");
  fflush(stdout);
  if (MADB_InitDynamicString(&serverEndpoint, "", BUFFER_SIZE, BUFFER_SIZE))
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }
  printf("BrowserAuth 2\n");
  fflush(stdout);
  if (MADB_InitDynamicString(&openBrowserCommand, "", BUFFER_SIZE, BUFFER_SIZE))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
    goto cleanupServerEndpoint;
  }


  printf("BrowserAuth 3\n");
  fflush(stdout);
  if (startLocalHttpServer(Dbc, &serverSocket, &serverEndpoint))
  {
    goto cleanupOpenBrowserCommand;
  }
  printf("BrowserAuth 4\n");
  fflush(stdout);
  if (getOpenBrowserCommand(Dbc, serverEndpoint.str, email, endpoint, &openBrowserCommand))
  {
    goto cleanupServer;
  }
  printf("BrowserAuth 5 %s\n", openBrowserCommand.str);
  fflush(stdout);
  if (system(openBrowserCommand.str))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_S1000, "Failed to open browser", 0);
    goto cleanupServer;
  }


  printf("BrowserAuth 6\n");
  fflush(stdout);
  if (readRequest(Dbc, serverSocket, requestReadTimeoutSec, credentials))
  {
    printf("BrowserAuth 61 %d\n", Dbc->Error.ReturnValue);
    fflush(stdout);
    goto cleanupServer;
  }

  printf("BrowserAuth 7\n");
  fflush(stdout);
cleanupServer:
  closeSocket(serverSocket);
cleanupOpenBrowserCommand:
  MADB_DynstrFree(&openBrowserCommand);
cleanupServerEndpoint:
  MADB_DynstrFree(&serverEndpoint);
  printf("BrowserAuth 71 %d\n", Dbc->Error.ReturnValue);
  fflush(stdout);
  return Dbc->Error.ReturnValue;
}
#undef BUFFER_SIZE

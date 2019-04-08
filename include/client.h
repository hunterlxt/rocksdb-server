#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "config.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

/**
 * DBserver类是用来和远程数据库交互的句柄
 * 每次操作数据库返回bool变量说明这次操作是否成功
 */
class DBserver {
  private:
    int sockfd;
    struct sockaddr_in dest;
    char sendBuf[REQ_SIZE];
    char inBuf[BUF_SIZE];
    enum Type { NUL, PUT, GET, DELETE, SCAN };
    void clearBuffer() {
        bzero(sendBuf, REQ_SIZE);
        bzero(inBuf, BUF_SIZE);
    };

  public:
    bool iswork = false;
    DBserver(const std::string &IP, const int PORT);
    ~DBserver() { Close(); }
    DBserver(DBserver &) = delete;
    DBserver &operator=(DBserver &) = delete;
    bool Connect();
    bool Close();

    bool Put(const std::string &key, const std::string &value);
    bool Delete(const std::string &key);
    bool Get(const std::string &key, std::string *value);
    bool Scan(const std::string &key1, const std::string &key2,
              std::vector<std::pair<std::string, std::string>> &ret);
};

#endif
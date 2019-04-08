#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "rocksdb/db.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define REQ_SIZE 265
#define BUF_SIZE 260
#define KEY_SIZE 8
#define VAL_SIZE 256
#define LARGE_VAL_SIZE 4194304

/**
 * DBserver类是用来和远程数据库交互的句柄
 * 每次操作数据库返回bool变量说明这次操作是否成功
 */
class DBserver {
  private:
    int sockfd;
    struct sockaddr_in dest;
    bool iswork = false;
    char reqBuf[REQ_SIZE];
    char inBuf[BUF_SIZE];
    enum Type { NUL, PUT, GET, DELETE, SCAN };

  public:
    DBserver(const std::string &IP, const int PORT) : sockfd(0) {
        dest.sin_family = PF_INET;
        dest.sin_port = htons(PORT);
        dest.sin_addr.s_addr = inet_addr(IP.c_str());
        sockfd = socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("sock error");
            exit(-1);
        }
    }
    ~DBserver() { Close(); }

    //建立连接到远程数据库
    bool Connect() {
        if (connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
            perror("connect error");
            return false;
        }
        iswork = true;
        return true;
    }
    //关闭远程数据库
    bool Close() {
        iswork = false;
        close(sockfd);
    }

    //四个操作接口
    bool Put(const std::string &key, const std::string &value);
    bool Delete(const std::string &key);
    bool Get(const std::string &key, std::string *value);
    bool Scan(const std::string &key1, const std::string &key2,
              std::vector<std::pair<std::string, std::string>> &ret);

  private:
    void clearBuffer() {
        bzero(reqBuf, REQ_SIZE);
        bzero(inBuf, BUF_SIZE);
    };
};

/**
 * 格式：操作类型+KEY+VALUE
 */
bool DBserver::Put(const std::string &key, const std::string &value) {
    if (key.length() != KEY_SIZE || value.length() != VAL_SIZE)
        return false;
    clearBuffer();
    reqBuf[0] = PUT;
    memcpy(reqBuf + 1, key.data(), KEY_SIZE);
    memcpy(reqBuf + 1 + KEY_SIZE, value.data(), VAL_SIZE);
    if (write(sockfd, reqBuf, REQ_SIZE) < REQ_SIZE) {
        Close();
        return false;
    }
    ssize_t nums = read(sockfd, inBuf, 1);
    if (nums < 1) {
        perror("connection has error");
        Close();
        return false;
    }
    if (inBuf[0] == 'T')
        return true;
    else
        return false;
}

/**
 * 格式：操作类型+KEY (1+8=9 Bytes)
 */
bool DBserver::Delete(const std::string &key) {
    if (key.length() != KEY_SIZE)
        return false;
    clearBuffer();
    reqBuf[0] = DELETE;
    memcpy(reqBuf + 1, key.data(), KEY_SIZE);
    if (write(sockfd, reqBuf, REQ_SIZE) < REQ_SIZE) {
        Close();
        return false;
    }
    ssize_t nums = read(sockfd, inBuf, 1);
    if (nums < 1) {
        perror("connection has error");
        Close();
        return false;
    }
    if (inBuf[0] == 'T')
        return true;
    else
        return false;
}

/**
 * 格式：操作类型+KEY (1+8=9 Bytes)
 */
bool DBserver::Get(const std::string &key, std::string *value) {
    if (key.length() != KEY_SIZE)
        return false;
    clearBuffer();
    reqBuf[0] = GET;
    memcpy(reqBuf + 1, key.data(), KEY_SIZE);
    if (write(sockfd, reqBuf, REQ_SIZE) < REQ_SIZE) {
        Close();
        return false;
    }
    ssize_t nums = read(sockfd, inBuf, 1 + VAL_SIZE);
    if (nums < 1) {
        perror("connection has error");
        Close();
        return false;
    }
    if (inBuf[0] == 'T') {
        value->assign(inBuf, 1, VAL_SIZE);
        return true;
    } else {
        return false;
    }
}

/**
 * 格式：操作类型+KEY+KEY (17 Bytes)
 */
bool DBserver::Scan(const std::string &key1, const std::string &key2,
                    std::vector<std::pair<std::string, std::string>> &ret) {
    if (key1.length() != KEY_SIZE || key2.length() != KEY_SIZE)
        return false;
    clearBuffer();
    reqBuf[0] = SCAN;
    memcpy(reqBuf + 1, key1.data(), KEY_SIZE);
    memcpy(reqBuf + 1 + KEY_SIZE, key2.data(), KEY_SIZE);
    if (write(sockfd, reqBuf, REQ_SIZE) < REQ_SIZE) {
        Close();
        return false;
    }
    size_t num = 0;
    ssize_t nums = read(sockfd, inBuf, 5);
    if (nums < 1) {
        perror("connection has error");
        Close();
        return false;
    }
    if (inBuf[0] == 'T') {
        int size = *(int *)(inBuf + 1);
        char *largeBuf = (char *)malloc(size * (VAL_SIZE + KEY_SIZE));
        size_t index = 0;
        int len = size * (VAL_SIZE + KEY_SIZE);
        while (len > 0) {
            ssize_t readnums =
                read(sockfd, largeBuf + index, size * (VAL_SIZE + KEY_SIZE));
            index += readnums;
            len -= readnums;
        }
        for (int i = 0; i < size; ++i) {
            std::string key(largeBuf + i * (KEY_SIZE + VAL_SIZE), KEY_SIZE);
            std::string value(largeBuf + i * (KEY_SIZE + VAL_SIZE) + KEY_SIZE,
                              VAL_SIZE);
            ret.push_back(std::pair<std::string, std::string>(key, value));
        }
        delete largeBuf;
        return true;
    } else
        return false;
}

#endif
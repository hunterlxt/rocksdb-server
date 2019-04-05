#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "rocksdb/db.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define REQ_SIZE 270
#define BUF_SIZE 257
#define LARGE_SIZE 67108864

/**
 * DBserver类是用来和远程数据库交互的句柄
 * 每次操作数据库返回bool变量说明这次操作是否成功
 */
class DBserver {
  private:
    int sockfd;
    struct sockaddr_in dest;
    char req[REQ_SIZE];
    char buf[BUF_SIZE];
    char *largeBuf = (char *)malloc(LARGE_SIZE);
    enum Type { NUL, PUT, GET, DELETE, SCAN };

  public:
    DBserver(const std::string &IP, const int PORT) : sockfd(0) {
        dest.sin_family = PF_INET;
        dest.sin_port = htons(PORT);
        dest.sin_addr.s_addr = inet_addr(IP.c_str());
        sockfd = socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("sock error");
        }
    }
    ~DBserver() {
        Close();
        delete largeBuf;
    }

    //建立连接到远程数据库
    bool Connect() {
        if (connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
            perror("connect error");
            return false;
        }
    }
    //关闭远程数据库
    bool Close() { close(sockfd); }

    //四个操作接口
    bool Put(const rocksdb::WriteOptions &options, const std::string &key,
             const std::string &value);
    bool Delete(const rocksdb::WriteOptions &options, const std::string &key);
    bool Get(const rocksdb::ReadOptions &options, const std::string &key,
             std::string *value);
    bool Scan();

  private:
    void fillWriteReq(enum Type t, const rocksdb::WriteOptions &options,
                      const std::string &key, const std::string *value) {
        bzero(req, REQ_SIZE);
        bzero(buf, BUF_SIZE);
        req[0] = t;
        memcpy(req + 1, (const void *)&options, 5);
        memcpy(req + 6, key.data(), 8);
        if (value != nullptr)
            memcpy(req + 14, (*value).data(), 256);
    }
    void fillReadReq(enum Type t, const rocksdb::ReadOptions &options,
                     const std::string &key) {
        bzero(req, REQ_SIZE);
        bzero(buf, BUF_SIZE);
        req[0] = t;
        memcpy(req + 1, (const void *)&options, 96);
        memcpy(req + 97, key.data(), 8);
    }
    bool check(const std::string &key, const std::string *value) {
        if (value != nullptr && value->length() != 256)
            return false;
        if (key.length() != 8)
            return false;
        return true;
    }
};

/**
 * 格式：操作类型+写入选项+KEY+VALUE (1+5+8+256=270 Bytes)
 */
bool DBserver::Put(const rocksdb::WriteOptions &options, const std::string &key,
                   const std::string &value) {
    if (!check(key, &value))
        return false;
    fillWriteReq(PUT, options, key, &value);
    write(sockfd, req, REQ_SIZE);
    if (read(sockfd, buf, BUF_SIZE) == 0) {
        perror("server is closed");
        Close();
        return false;
    }
    if (buf[0] == '0')
        return true;
    else
        return false;
}

/**
 * 格式：操作类型+写入选项+KEY (1+5+8=14 Bytes)
 */
bool DBserver::Delete(const rocksdb::WriteOptions &options,
                      const std::string &key) {
    if (!check(key, nullptr))
        return false;
    fillWriteReq(DELETE, options, key, nullptr);
    write(sockfd, req, REQ_SIZE);
    if (read(sockfd, buf, BUF_SIZE) == 0) {
        perror("server is closed");
        Close();
        return false;
    }
    if (buf[0] == '0')
        return true;
    else
        return false;
}

/**
 * 格式：操作类型+读取选项+KEY (1+96+8=105 Bytes)
 */
bool DBserver::Get(const rocksdb::ReadOptions &options, const std::string &key,
                   std::string *value) {
    if (!check(key, nullptr))
        return false;
    fillReadReq(GET, options, key);
    write(sockfd, req, REQ_SIZE);
    if (read(sockfd, buf, BUF_SIZE) == 0) {
        perror("server is closed");
        Close();
        (*value).clear();
        return false;
    }
    if (buf[0] == '0') {
        *value = buf + 1;
        return ((*value).length() == 256);
    } else {
        (*value).clear();
        return false;
    }
}

/**
 * 格式：操作类型+写入选项+KEY+VALUE (270 Bytes)
 */
bool Scan() {}

#endif
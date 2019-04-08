#ifndef _SERVER_H_
#define _SERVER_H_

#include "config.h"
#include "rocksdb/db.h"
#include "uv.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

enum Type { NUL, PUT, GET, DELETE, SCAN };

struct Client {
    static rocksdb::DB *db;
    uv_tcp_t tcp;
    uv_stream_t *server;
    char outBuffer[BUF_SIZE];

    Client(uv_stream_t *s) : server(s) {}
    Client(Client &) = delete;
    Client &operator=(Client &) = delete;
};

bool operate_database(Client *client, const uv_buf_t *reqBuffer,
                      uv_buf_t *outbuf) {
    char *req = reqBuffer->base;
    Type t = (Type)req[0];
    rocksdb::Status s;
    std::string key1;
    std::string key2;
    std::string value;
    std::string retValue;
    bzero(client->outBuffer, BUF_SIZE);
    if (t == PUT) {
        key1.assign(req, 1, KEY_SIZE);
        value.assign(req, 1 + KEY_SIZE, VAL_SIZE);
        s = client->db->Put(rocksdb::WriteOptions(), key1, value);
        if (s.ok())
            client->outBuffer[0] = 'T';
        outbuf->base = client->outBuffer;
        outbuf->len = 1;
    } else if (t == GET) {
        key1.assign(req, 1, KEY_SIZE);
        s = client->db->Get(rocksdb::ReadOptions(), key1, &retValue);
        if (s.ok()) {
            client->outBuffer[0] = 'T';
            memcpy(client->outBuffer + 1, retValue.data(), VAL_SIZE);
        }
        retValue.clear();
        outbuf->base = client->outBuffer;
        outbuf->len = 1 + VAL_SIZE;
    } else if (t == DELETE) {
        key1.assign(req, 1, KEY_SIZE);
        s = client->db->Delete(rocksdb::WriteOptions(), key1);
        if (s.ok())
            client->outBuffer[0] = 'T';
        outbuf->base = client->outBuffer;
        outbuf->len = 1;
    } else if (t == SCAN) {
        key1.assign(req, 1, 8);
        key2.assign(req, 9, 8);
        rocksdb::Iterator *it = client->db->NewIterator(rocksdb::ReadOptions());
        std::vector<std::pair<std::string, std::string>> ret;
        for (it->Seek(key1); it->Valid() && it->key().ToString() < key2;
             it->Next()) {
            ret.push_back(std::pair<std::string, std::string>(
                it->key().ToString(), it->value().ToString()));
        }
        char *largeBuffer =
            (char *)malloc(5 + ret.size() * (KEY_SIZE + VAL_SIZE));

        if (it->status().ok())
            largeBuffer[0] = 'T';
        uint32_t size = ret.size();
        memcpy(largeBuffer + 1, (char *)&size, 4);
        for (int i = 0; i < ret.size(); i++) {
            memcpy(largeBuffer + 5 + i * (KEY_SIZE + VAL_SIZE),
                   ret[i].first.data(), KEY_SIZE);
            memcpy(largeBuffer + 5 + i * (KEY_SIZE + VAL_SIZE) + KEY_SIZE,
                   ret[i].second.data(), VAL_SIZE);
        }
        delete it;
        outbuf->base = largeBuffer;
        outbuf->len = size * (KEY_SIZE + VAL_SIZE) + 5;
    }
}

#endif
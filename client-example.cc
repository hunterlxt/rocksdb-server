#include "client.h"
#include <assert.h>
#include <iostream>

std::string key = "22223333";
std::string value =
    "12345678123456781234567812345678123456781234567812345678123456781234567812"
    "34567812345678123456781234567812345678123456781234567812345678123456781234"
    "56781234567812345678123456781234567812345678123456781234567812345678123456"
    "7812345678123456781234567812345678";

int main() {

    DBserver db("127.0.0.1", 8888);
    db.Connect();
    std::cout << "Connection succeeded!" << std::endl;

    bool flag = false;
    flag = db.Put(rocksdb::WriteOptions(), key, value);
    assert(flag);
    std::cout << "Put Key:" << key << '\n' << "Value:" << value << std::endl;

    value.clear();
    flag = db.Get(rocksdb::ReadOptions(), key, &value);
    assert(flag);
    std::cout << "Get from key:" << key << '\n'
              << "Value:" << value << std::endl;

    flag = db.Delete(rocksdb::WriteOptions(), key);
    assert(flag);

    flag = db.Get(rocksdb::ReadOptions(), key, &value);
    assert(!flag);
    std::cout << "Not found key:" << key << '\n'
              << "Value:" << value << std::endl;

    db.Close();
    std::cout << "Closed server" << std::endl;
    return 0;
}
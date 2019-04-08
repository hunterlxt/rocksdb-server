#include "client.h"
#include <assert.h>
#include <iostream>

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        std::cout << "Usage: ./run-client [Database_IP] [Database_PORT]\n";
        return -1;
    }
    std::string key = "88888888";
    std::string value = "123456781234567812345678123456781234567812345678123456"
                        "78123456781234567812"
                        "345678123456781234567812345678123456781234567812345678"
                        "12345678123456781234"
                        "567812345678123456781234567812345678123456781234567812"
                        "34567812345678123456"
                        "7812345678123456781234567812345678";
    DBserver db(argv[1], atoi(argv[2]));
    bool flag = false;
    flag = db.Connect();
    assert(flag);
    std::cout << "Connection succeeded!\n" << std::endl;

    flag = db.Put(key, value);
    assert(flag);

    value.clear();
    flag = db.Get(key, &value);
    assert(flag);
    std::cout << "Get key:" << key << '\n'
              << "Value:" << value << '\n'
              << std::endl;

    flag = db.Delete(key);
    assert(flag);
    std::cout << "Deleted key:" << key << '\n' << std::endl;

    flag = db.Get(key, &value);
    assert(!flag);
    std::cout << key << " is not found\n" << std::endl;

    char ckey[8];
    for (size_t i = 10000000; i < 10005000; i++) {
        sprintf(ckey, "%d", i);
        key.assign(ckey);
        flag = db.Put(key, value);
        assert(flag);
    }
    std::vector<std::pair<std::string, std::string>> pairs;
    flag = db.Scan("10000000", "20000000", pairs);
    std::cout << pairs.size() << std::endl;
    // for (auto pair : pairs) {
    //     std::cout << pair.first << '\n' << pair.second << std::endl;
    // }

    db.Close();
    std::cout << "\nTest finished!" << std::endl;
    return 0;
}

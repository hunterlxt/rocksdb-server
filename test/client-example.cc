#include "client.h"
#include <assert.h>
#include <iostream>

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
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
    std::cout << "Connection successfully\n"
              << std::endl;

    flag = db.Put(key, value);
    assert(flag);
    clock_t start;
    value.clear();
    start = clock();
    flag = db.Get(key, &value);
    assert(flag);
    std::cout << "Get key:" << key << " successfully"
              << "\nMilli seconds:" << clock() - start << '\n'
              << std::endl;

    flag = db.Delete(key);
    assert(flag);
    std::cout << "Deleted key:" << key << "\nMilli seconds:" << clock() - start << '\n'
              << std::endl;

    flag = db.Get(key, &value);
    assert(!flag);
    std::cout << "Key:" << key << " is not found"
              << "\nMilli seconds:" << clock() - start << '\n'
              << std::endl;

    char ckey[8];

    start = clock();
    for (size_t i = 10000000; i < 10010000; i++)
    {
        sprintf(ckey, "%d", i);
        key.assign(ckey);
        flag = db.Put(key, value);
    }
    std::cout << "Put 10000 KV pairs successfully\nMilli seconds:" << clock() - start << '\n'
              << std::endl;
    std::vector<std::pair<std::string, std::string>> pairs;
    start = clock();
    flag = db.Scan("10000000", "20000000", pairs);
    std::cout << "Scan from 10000000 to 20000000 and return " << pairs.size() << " KV pairs\nMilli seconds:" << clock() - start << '\n'
              << std::endl;

    db.Close();
    std::cout << "===== Test finished =====" << std::endl;
    return 0;
}

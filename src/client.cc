#include "client.h"

/**
 *
 */
DBserver::DBserver(const std::string &IP, const int PORT) : sockfd(0) {
    dest.sin_family = PF_INET;
    dest.sin_port = htons(PORT);
    dest.sin_addr.s_addr = inet_addr(IP.c_str());
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "sock error" << std::endl;
        exit(-1);
    }
}

/**
 *
 */
bool DBserver::Connect() {
    if (connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        std::cerr << "connect error" << std::endl;
        return false;
    }
    iswork = true;
    return true;
}

/**
 * 关闭远程数据库
 */
bool DBserver::Close() {
    iswork = false;
    close(sockfd);
}
/**
 * out：操作类型+KEY+VALUE
 */
bool DBserver::Put(const std::string &key, const std::string &value) {
    if (key.length() != KEY_SIZE || value.length() != VAL_SIZE || !iswork) {
        std::cerr << "PUT:usage error" << std::endl;
        return false;
    }
    clearBuffer();
    sendBuf[0] = PUT;
    memcpy(sendBuf + 1, key.data(), KEY_SIZE);
    memcpy(sendBuf + 1 + KEY_SIZE, value.data(), VAL_SIZE);
    if (write(sockfd, sendBuf, REQ_SIZE) < REQ_SIZE) {
        Close();
        std::cerr << "PUT:write error" << std::endl;
        return false;
    }
    ssize_t nums = read(sockfd, inBuf, 1);
    if (nums < 1) {
        std::cerr << "PUT:read error" << std::endl;
        Close();
        return false;
    }
    if (inBuf[0] == 'T')
        return true;
    else
        return false;
}

/**
 * out：操作类型+KEY
 */
bool DBserver::Delete(const std::string &key) {
    if (key.length() != KEY_SIZE || !iswork) {
        std::cerr << "Delete:usage error" << std::endl;
        return false;
    }
    clearBuffer();
    sendBuf[0] = DELETE;
    memcpy(sendBuf + 1, key.data(), KEY_SIZE);
    if (write(sockfd, sendBuf, REQ_SIZE) < REQ_SIZE) {
        Close();
        std::cerr << "PUT:write error" << std::endl;
        return false;
    }
    ssize_t nums = read(sockfd, inBuf, 1);
    if (nums < 1) {
        std::cerr << "PUT:read error" << std::endl;
        Close();
        return false;
    }
    if (inBuf[0] == 'T')
        return true;
    else
        return false;
}

/**
 * out：操作类型+KEY
 */
bool DBserver::Get(const std::string &key, std::string *value) {
    if (key.length() != KEY_SIZE || !iswork) {
        std::cerr << "Get:usage error" << std::endl;
        return false;
    }
    clearBuffer();
    sendBuf[0] = GET;
    memcpy(sendBuf + 1, key.data(), KEY_SIZE);
    if (write(sockfd, sendBuf, REQ_SIZE) < REQ_SIZE) {
        Close();
        std::cerr << "Get:write error" << std::endl;
        return false;
    }
    ssize_t nums = read(sockfd, inBuf, 1 + VAL_SIZE);
    if (nums < VAL_SIZE + 1) {
        std::cerr << "Get:read error" << std::endl;
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
 * out：操作类型+KEY+KEY
 */
bool DBserver::Scan(const std::string &key1, const std::string &key2,
                    std::vector<std::pair<std::string, std::string>> &ret) {
    if (key1.length() != KEY_SIZE || key2.length() != KEY_SIZE || !iswork) {
        std::cerr << "Scan:usage error" << std::endl;
        return false;
    }
    clearBuffer();
    sendBuf[0] = SCAN;
    memcpy(sendBuf + 1, key1.data(), KEY_SIZE);
    memcpy(sendBuf + 1 + KEY_SIZE, key2.data(), KEY_SIZE);
    if (write(sockfd, sendBuf, REQ_SIZE) < REQ_SIZE) {
        Close();
        std::cerr << "Scan:write error" << std::endl;
        return false;
    }
    ssize_t nums = read(sockfd, inBuf, 5);
    if (nums < 1) {
        std::cerr << "Scan:read error" << std::endl;
        Close();
        return false;
    }
    if (inBuf[0] == 'T') {
        uint32_t size = *(int *)(inBuf + 1);
        char *largeBuf = (char *)malloc(size * (VAL_SIZE + KEY_SIZE));
        int index = 0;
        int len = size * (VAL_SIZE + KEY_SIZE);
        while (len > 0) {
            ssize_t readnums = read(sockfd, largeBuf + index, len);
            index += readnums;
            len -= readnums;
        }
        for (int i = 0; i < size; ++i) {
            ret.push_back(std::pair<std::string, std::string>(
                std::string(largeBuf + i * (KEY_SIZE + VAL_SIZE), KEY_SIZE),
                std::string(largeBuf + i * (KEY_SIZE + VAL_SIZE) + KEY_SIZE,
                            VAL_SIZE)));
        }
        // ??? memory pool
        delete largeBuf;
        return true;
    } else
        return false;
}
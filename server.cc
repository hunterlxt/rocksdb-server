#include "server.h"
#include <string>

std::string database_dir;
int local_port = 0;
rocksdb::DB *Client::db;

void on_new_connection(uv_stream_t *server, int status);
void work_cb(uv_work_t *worker);
void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf);
void after_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf);
void after_write(uv_write_t *req, int status);

int main(int argc, char const *argv[]) {
    // parse the input arguments
    if (argc != 3) {
        fprintf(stdout, "Usage: %s [database_dir] [tcp_port]\n", argv[0]);
        return 1;
    }
    database_dir = "/home/lxt/Desktop/Database";
    local_port = atoi(argv[2]);
    // open the database
    rocksdb::Options opt;
    opt.create_if_missing = true;

    rocksdb::Status s = rocksdb::DB::Open(opt, database_dir, &Client::db);
    // build the socket for listening
    uv_tcp_t server;
    uv_tcp_init(uv_default_loop(), &server);
    struct sockaddr_in server_addr;
    uv_ip4_addr("127.0.0.1", local_port, &server_addr);
    uv_tcp_bind(&server, (const struct sockaddr *)&server_addr, 0);
    int r = uv_listen((uv_stream_t *)&server, 128, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_err_name(r));
        return 1;
    }
    // run loop
    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void on_new_connection(uv_stream_t *server, int status) {
    if (status == -1)
        return;
    // bind the client with a worker
    Client *client = new Client(server);
    uv_work_t *worker = (uv_work_t *)malloc(sizeof(uv_work_t));
    worker->data = client;
    // add the worker to the queue
    uv_queue_work(uv_default_loop(), worker, work_cb, NULL);
}

void work_cb(uv_work_t *worker) {

    Client *client = (Client *)worker->data;
    uv_tcp_init(uv_default_loop(), &client->tcp);
    client->tcp.data = client;
    if (uv_accept(client->server, (uv_stream_t *)&client->tcp) == 0) {
        uv_read_start((uv_stream_t *)&client->tcp, alloc_buffer, after_read);
    } else {
        //这里要加入关闭处理
        // uv_close((uv_handle_t *)client, NULL);
    }
}

void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf) {

    buf->base = (char *)malloc(REQ_SIZE);
    buf->len = REQ_SIZE;
}

void after_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        Client *client = (Client *)tcp->data;
        uv_buf_t outbuf;
        operate_database(client, buf, &outbuf);
        uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
        uv_write((uv_write_t *)req, tcp, &outbuf, 1, after_write);
    }
}

void after_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
}

void after_work_cb(uv_work_t *worker, int status) {}
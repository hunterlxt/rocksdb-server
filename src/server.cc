#include "server.h"
#include <string>

rocksdb::DB *Client::db;

void on_new_connection(uv_stream_t *server, int status);
void work_cb(uv_work_t *worker);
void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf);
void after_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf);
void after_shutdown(uv_shutdown_t *req, int status);
void on_close(uv_handle_t *handle);
void after_write(uv_write_t *req, int status);

int main(int argc, char const *argv[]) {
    // parse the input arguments
    if (argc != 3) {
        fprintf(stdout, "Usage: %s [database_dir] [tcp_port]\n", argv[0]);
        return 1;
    }
    // build the socket for listening
    uv_tcp_t server;
    uv_tcp_init(uv_default_loop(), &server);
    struct sockaddr_in server_addr;
    int local_port = atoi(argv[2]);
    uv_ip4_addr("0.0.0.0", local_port, &server_addr);
    uv_tcp_bind(&server, (const struct sockaddr *)&server_addr, 0);
    int r = uv_listen((uv_stream_t *)&server, 128, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_err_name(r));
        return 1;
    }
    // open the database
    std::string database_dir = argv[1];
    rocksdb::Options opt;
    opt.create_if_missing = true;
    rocksdb::Status s = rocksdb::DB::Open(opt, database_dir, &Client::db);
    // run loop
    fprintf(stdout, "Listening Port:%d", local_port);
    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void on_new_connection(uv_stream_t *server, int status) {
    if (status) {
        fprintf(stderr, "async connect %s\n", uv_err_name(status));
        return;
    }
    // bind the client with a worker
    Client *client = new Client(server);
    uv_work_t *worker = new uv_work_t;
    worker->data = client;
    uv_queue_work(uv_default_loop(), worker, work_cb, NULL);
}

void work_cb(uv_work_t *worker) {
    Client *client = (Client *)worker->data;
    uv_tcp_init(uv_default_loop(), &client->tcp);
    client->tcp.data = client;
    int r = uv_accept(client->server, (uv_stream_t *)&client->tcp);
    if (r) {
        fprintf(stderr, "accept error %s\n", uv_err_name(r));
        uv_close((uv_handle_t *)&client->tcp, NULL);
        return;
    }
    uv_read_start((uv_stream_t *)&client->tcp, alloc_buffer, after_read);
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
    } else if (nread < 0) {
        fprintf(stderr, "client closed: %s\n", uv_strerror(nread));
        uv_shutdown_t *req = (uv_shutdown_t *)malloc(sizeof(*req));
        int r = uv_shutdown(req, tcp, after_shutdown);
    }
    free(buf->base);
}

void after_shutdown(uv_shutdown_t *req, int status) {
    if (status < 0)
        fprintf(stderr, "shutdown error: %s\n", uv_err_name(status));
    uv_close((uv_handle_t *)req->handle, on_close);
    free(req);
}

void on_close(uv_handle_t *handle) { free(handle); }

void after_write(uv_write_t *req, int status) {
    free(req);
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
}

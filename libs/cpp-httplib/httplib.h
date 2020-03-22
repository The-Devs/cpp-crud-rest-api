//
//  httplib.h
//
//  Copyright (c) 2020 Yuji Hirose. All rights reserved.
//  MIT License
//

#ifndef CPPHTTPLIB_HTTPLIB_H
#define CPPHTTPLIB_HTTPLIB_H

/*
 * Configuration
 */

#ifndef CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND
#define CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_KEEPALIVE_TIMEOUT_USECOND
#define CPPHTTPLIB_KEEPALIVE_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_KEEPALIVE_MAX_COUNT
#define CPPHTTPLIB_KEEPALIVE_MAX_COUNT 5
#endif

#ifndef CPPHTTPLIB_READ_TIMEOUT_SECOND
#define CPPHTTPLIB_READ_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_READ_TIMEOUT_USECOND
#define CPPHTTPLIB_READ_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_REQUEST_URI_MAX_LENGTH
#define CPPHTTPLIB_REQUEST_URI_MAX_LENGTH 8192
#endif

#ifndef CPPHTTPLIB_REDIRECT_MAX_COUNT
#define CPPHTTPLIB_REDIRECT_MAX_COUNT 20
#endif

#ifndef CPPHTTPLIB_PAYLOAD_MAX_LENGTH
#define CPPHTTPLIB_PAYLOAD_MAX_LENGTH (std::numeric_limits<size_t>::max())
#endif

#ifndef CPPHTTPLIB_RECV_BUFSIZ
#define CPPHTTPLIB_RECV_BUFSIZ size_t(4096u)
#endif

#ifndef CPPHTTPLIB_THREAD_POOL_COUNT
#define CPPHTTPLIB_THREAD_POOL_COUNT                                           \
  (std::max(1u, std::thread::hardware_concurrency() - 1))
#endif

/*
 * Headers
 */

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif //_CRT_SECURE_NO_WARNINGS

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif //_CRT_NONSTDC_NO_DEPRECATE

#if defined(_MSC_VER)
#ifdef _WIN64
using ssize_t = __int64;
#else
using ssize_t = int;
#endif

#if _MSC_VER < 1900
#define snprintf _snprintf_s
#endif
#endif // _MSC_VER

#ifndef S_ISREG
#define S_ISREG(m) (((m)&S_IFREG) == S_IFREG)
#endif // S_ISREG

#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&S_IFDIR) == S_IFDIR)
#endif // S_ISDIR

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef WSA_FLAG_NO_HANDLE_INHERIT
#define WSA_FLAG_NO_HANDLE_INHERIT 0x80
#endif

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif // strcasecmp

using socket_t = SOCKET;
#ifdef CPPHTTPLIB_USE_POLL
#define poll(fds, nfds, timeout) WSAPoll(fds, nfds, timeout)
#endif

#else // not _WIN32

#include <arpa/inet.h>
#include <cstring>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef CPPHTTPLIB_USE_POLL
#include <poll.h>
#endif
#include <csignal>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

using socket_t = int;
#define INVALID_SOCKET (-1)
#endif //_WIN32

#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <regex>
#include <string>
#include <sys/stat.h>
#include <thread>

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include <iomanip>
#include <sstream>

// #if OPENSSL_VERSION_NUMBER < 0x1010100fL
// #error Sorry, OpenSSL versions prior to 1.1.1 are not supported
// #endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#include <openssl/crypto.h>
inline const unsigned char *ASN1_STRING_get0_data(const ASN1_STRING *asn1) {
  return M_ASN1_STRING_data(asn1);
}
#endif
#endif

#ifdef CPPHTTPLIB_ZLIB_SUPPORT
#include <zlib.h>
#endif

/*
 * Declaration
 */
namespace httplib {

namespace detail {

struct ci {
  bool operator()(const std::string &s1, const std::string &s2) const {
    return std::lexicographical_compare(
        s1.begin(), s1.end(), s2.begin(), s2.end(),
        [](char c1, char c2) { return ::tolower(c1) < ::tolower(c2); });
  }
};

} // namespace detail

using Headers = std::multimap<std::string, std::string, detail::ci>;

using Params = std::multimap<std::string, std::string>;
using Match = std::smatch;

using Progress = std::function<bool(uint64_t current, uint64_t total)>;

struct Response;
using ResponseHandler = std::function<bool(const Response &response)>;

struct MultipartFormData {
  std::string name;
  std::string content;
  std::string filename;
  std::string content_type;
};
using MultipartFormDataItems = std::vector<MultipartFormData>;
using MultipartFormDataMap = std::multimap<std::string, MultipartFormData>;

class DataSink {
public:
  DataSink() = default;
  DataSink(const DataSink &) = delete;
  DataSink &operator=(const DataSink &) = delete;
  DataSink(DataSink &&) = delete;
  DataSink &operator=(DataSink &&) = delete;

  std::function<void(const char *data, size_t data_len)> write;
  std::function<void()> done;
  std::function<bool()> is_writable;
};

using ContentProvider =
    std::function<void(size_t offset, size_t length, DataSink &sink)>;

using ContentReceiver =
    std::function<bool(const char *data, size_t data_length)>;

using MultipartContentHeader =
    std::function<bool(const MultipartFormData &file)>;

class ContentReader {
public:
  using Reader = std::function<bool(ContentReceiver receiver)>;
  using MultipartReader = std::function<bool(MultipartContentHeader header,
                                             ContentReceiver receiver)>;

  ContentReader(Reader reader, MultipartReader muitlpart_reader)
      : reader_(reader), muitlpart_reader_(muitlpart_reader) {}

  bool operator()(MultipartContentHeader header,
                  ContentReceiver receiver) const {
    return muitlpart_reader_(header, receiver);
  }

  bool operator()(ContentReceiver receiver) const { return reader_(receiver); }

  Reader reader_;
  MultipartReader muitlpart_reader_;
};

using Range = std::pair<ssize_t, ssize_t>;
using Ranges = std::vector<Range>;

struct Request {
  std::string method;
  std::string path;
  Headers headers;
  std::string body;

  // for server
  std::string version;
  std::string target;
  Params params;
  MultipartFormDataMap files;
  Ranges ranges;
  Match matches;

  // for client
  size_t redirect_count = CPPHTTPLIB_REDIRECT_MAX_COUNT;
  ResponseHandler response_handler;
  ContentReceiver content_receiver;
  Progress progress;

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  const SSL *ssl;
#endif

  bool has_header(const char *key) const;
  std::string get_header_value(const char *key, size_t id = 0) const;
  size_t get_header_value_count(const char *key) const;
  void set_header(const char *key, const char *val);
  void set_header(const char *key, const std::string &val);

  bool has_param(const char *key) const;
  std::string get_param_value(const char *key, size_t id = 0) const;
  size_t get_param_value_count(const char *key) const;

  bool is_multipart_form_data() const;

  bool has_file(const char *key) const;
  MultipartFormData get_file_value(const char *key) const;

  // private members...
  size_t content_length;
  ContentProvider content_provider;
};

struct Response {
  std::string version;
  int status = -1;
  Headers headers;
  std::string body;

  bool has_header(const char *key) const;
  std::string get_header_value(const char *key, size_t id = 0) const;
  size_t get_header_value_count(const char *key) const;
  void set_header(const char *key, const char *val);
  void set_header(const char *key, const std::string &val);

  void set_redirect(const char *url);
  void set_content(const char *s, size_t n, const char *content_type);
  void set_content(const std::string &s, const char *content_type);

  void set_content_provider(
      size_t length,
      std::function<void(size_t offset, size_t length, DataSink &sink)>
          provider,
      std::function<void()> resource_releaser = [] {});

  void set_chunked_content_provider(
      std::function<void(size_t offset, DataSink &sink)> provider,
      std::function<void()> resource_releaser = [] {});

  Response() = default;
  Response(const Response &) = default;
  Response &operator=(const Response &) = default;
  Response(Response &&) = default;
  Response &operator=(Response &&) = default;
  ~Response() {
    if (content_provider_resource_releaser) {
      content_provider_resource_releaser();
    }
  }

  // private members...
  size_t content_length = 0;
  ContentProvider content_provider;
  std::function<void()> content_provider_resource_releaser;
};

class Stream {
public:
  virtual ~Stream() = default;

  virtual bool is_readable() const = 0;
  virtual bool is_writable() const = 0;

  virtual int read(char *ptr, size_t size) = 0;
  virtual int write(const char *ptr, size_t size) = 0;
  virtual std::string get_remote_addr() const = 0;

  template <typename... Args>
  int write_format(const char *fmt, const Args &... args);
  int write(const char *ptr);
  int write(const std::string &s);
};

class TaskQueue {
public:
  TaskQueue() = default;
  virtual ~TaskQueue() = default;
  virtual void enqueue(std::function<void()> fn) = 0;
  virtual void shutdown() = 0;
};

class ThreadPool : public TaskQueue {
public:
  explicit ThreadPool(size_t n) : shutdown_(false) {
    while (n) {
      threads_.emplace_back(worker(*this));
      n--;
    }
  }

  ThreadPool(const ThreadPool &) = delete;
  ~ThreadPool() override = default;

  void enqueue(std::function<void()> fn) override {
    std::unique_lock<std::mutex> lock(mutex_);
    jobs_.push_back(fn);
    cond_.notify_one();
  }

  void shutdown() override {
    // Stop all worker threads...
    {
      std::unique_lock<std::mutex> lock(mutex_);
      shutdown_ = true;
    }

    cond_.notify_all();

    // Join...
    for (auto &t : threads_) {
      t.join();
    }
  }

private:
  struct worker {
    explicit worker(ThreadPool &pool) : pool_(pool) {}

    void operator()() {
      for (;;) {
        std::function<void()> fn;
        {
          std::unique_lock<std::mutex> lock(pool_.mutex_);

          pool_.cond_.wait(
              lock, [&] { return !pool_.jobs_.empty() || pool_.shutdown_; });

          if (pool_.shutdown_ && pool_.jobs_.empty()) { break; }

          fn = pool_.jobs_.front();
          pool_.jobs_.pop_front();
        }

        assert(true == static_cast<bool>(fn));
        fn();
      }
    }

    ThreadPool &pool_;
  };
  friend struct worker;

  std::vector<std::thread> threads_;
  std::list<std::function<void()>> jobs_;

  bool shutdown_;

  std::condition_variable cond_;
  std::mutex mutex_;
};

using Logger = std::function<void(const Request &, const Response &)>;

class Server {
public:
  using Handler = std::function<void(const Request &, Response &)>;
  using HandlerWithContentReader = std::function<void(
      const Request &, Response &, const ContentReader &content_reader)>;
  using Expect100ContinueHandler =
      std::function<int(const Request &, Response &)>;

  Server();

  virtual ~Server();

  virtual bool is_valid() const;

  Server &Get(const char *pattern, Handler handler);
  Server &Post(const char *pattern, Handler handler);
  Server &Post(const char *pattern, HandlerWithContentReader handler);
  Server &Put(const char *pattern, Handler handler);
  Server &Put(const char *pattern, HandlerWithContentReader handler);
  Server &Patch(const char *pattern, Handler handler);
  Server &Patch(const char *pattern, HandlerWithContentReader handler);
  Server &Delete(const char *pattern, Handler handler);
  Server &Options(const char *pattern, Handler handler);

  [[deprecated]] bool set_base_dir(const char *dir,
                                   const char *mount_point = nullptr);
  bool set_mount_point(const char *mount_point, const char *dir);
  bool remove_mount_point(const char *mount_point);
  void set_file_extension_and_mimetype_mapping(const char *ext,
                                               const char *mime);
  void set_file_request_handler(Handler handler);

  void set_error_handler(Handler handler);
  void set_logger(Logger logger);

  void set_expect_100_continue_handler(Expect100ContinueHandler handler);

  void set_keep_alive_max_count(size_t count);
  void set_read_timeout(time_t sec, time_t usec);
  void set_payload_max_length(size_t length);

  bool bind_to_port(const char *host, int port, int socket_flags = 0);
  int bind_to_any_port(const char *host, int socket_flags = 0);
  bool listen_after_bind();

  bool listen(const char *host, int port, int socket_flags = 0);

  bool is_running() const;
  void stop();

  std::function<TaskQueue *(void)> new_task_queue;

protected:
  bool process_request(Stream &strm, bool last_connection,
                       bool &connection_close,
                       const std::function<void(Request &)> &setup_request);

  size_t keep_alive_max_count_;
  time_t read_timeout_sec_;
  time_t read_timeout_usec_;
  size_t payload_max_length_;

private:
  using Handlers = std::vector<std::pair<std::regex, Handler>>;
  using HandlersForContentReader =
      std::vector<std::pair<std::regex, HandlerWithContentReader>>;

  socket_t create_server_socket(const char *host, int port,
                                int socket_flags) const;
  int bind_internal(const char *host, int port, int socket_flags);
  bool listen_internal();

  bool routing(Request &req, Response &res, Stream &strm, bool last_connection);
  bool handle_file_request(Request &req, Response &res, bool head = false);
  bool dispatch_request(Request &req, Response &res, Handlers &handlers);
  bool dispatch_request_for_content_reader(Request &req, Response &res,
                                           ContentReader content_reader,
                                           HandlersForContentReader &handlers);

  bool parse_request_line(const char *s, Request &req);
  bool write_response(Stream &strm, bool last_connection, const Request &req,
                      Response &res);
  bool write_content_with_provider(Stream &strm, const Request &req,
                                   Response &res, const std::string &boundary,
                                   const std::string &content_type);
  bool read_content(Stream &strm, bool last_connection, Request &req,
                    Response &res);
  bool read_content_with_content_receiver(
      Stream &strm, bool last_connection, Request &req, Response &res,
      ContentReceiver receiver, MultipartContentHeader multipart_header,
      ContentReceiver multipart_receiver);
  bool read_content_core(Stream &strm, bool last_connection, Request &req,
                         Response &res, ContentReceiver receiver,
                         MultipartContentHeader mulitpart_header,
                         ContentReceiver multipart_receiver);

  virtual bool process_and_close_socket(socket_t sock);

  std::atomic<bool> is_running_;
  std::atomic<socket_t> svr_sock_;
  std::vector<std::pair<std::string, std::string>> base_dirs_;
  std::map<std::string, std::string> file_extension_and_mimetype_map_;
  Handler file_request_handler_;
  Handlers get_handlers_;
  Handlers post_handlers_;
  HandlersForContentReader post_handlers_for_content_reader_;
  Handlers put_handlers_;
  HandlersForContentReader put_handlers_for_content_reader_;
  Handlers patch_handlers_;
  HandlersForContentReader patch_handlers_for_content_reader_;
  Handlers delete_handlers_;
  Handlers options_handlers_;
  Handler error_handler_;
  Logger logger_;
  Expect100ContinueHandler expect_100_continue_handler_;
};

class Client {
public:
  explicit Client(const std::string &host, int port = 80,
                  const std::string &client_cert_path = std::string(),
                  const std::string &client_key_path = std::string());

  virtual ~Client();

  virtual bool is_valid() const;

  std::shared_ptr<Response> Get(const char *path);

  std::shared_ptr<Response> Get(const char *path, const Headers &headers);

  std::shared_ptr<Response> Get(const char *path, Progress progress);

  std::shared_ptr<Response> Get(const char *path, const Headers &headers,
                                Progress progress);

  std::shared_ptr<Response> Get(const char *path,
                                ContentReceiver content_receiver);

  std::shared_ptr<Response> Get(const char *path, const Headers &headers,
                                ContentReceiver content_receiver);

  std::shared_ptr<Response>
  Get(const char *path, ContentReceiver content_receiver, Progress progress);

  std::shared_ptr<Response> Get(const char *path, const Headers &headers,
                                ContentReceiver content_receiver,
                                Progress progress);

  std::shared_ptr<Response> Get(const char *path, const Headers &headers,
                                ResponseHandler response_handler,
                                ContentReceiver content_receiver);

  std::shared_ptr<Response> Get(const char *path, const Headers &headers,
                                ResponseHandler response_handler,
                                ContentReceiver content_receiver,
                                Progress progress);

  std::shared_ptr<Response> Head(const char *path);

  std::shared_ptr<Response> Head(const char *path, const Headers &headers);

  std::shared_ptr<Response> Post(const char *path, const std::string &body,
                                 const char *content_type);

  std::shared_ptr<Response> Post(const char *path, const Headers &headers,
                                 const std::string &body,
                                 const char *content_type);

  std::shared_ptr<Response> Post(const char *path, size_t content_length,
                                 ContentProvider content_provider,
                                 const char *content_type);

  std::shared_ptr<Response> Post(const char *path, const Headers &headers,
                                 size_t content_length,
                                 ContentProvider content_provider,
                                 const char *content_type);

  std::shared_ptr<Response> Post(const char *path, const Params &params);

  std::shared_ptr<Response> Post(const char *path, const Headers &headers,
                                 const Params &params);

  std::shared_ptr<Response> Post(const char *path,
                                 const MultipartFormDataItems &items);

  std::shared_ptr<Response> Post(const char *path, const Headers &headers,
                                 const MultipartFormDataItems &items);

  std::shared_ptr<Response> Put(const char *path, const std::string &body,
                                const char *content_type);

  std::shared_ptr<Response> Put(const char *path, const Headers &headers,
                                const std::string &body,
                                const char *content_type);

  std::shared_ptr<Response> Put(const char *path, size_t content_length,
                                ContentProvider content_provider,
                                const char *content_type);

  std::shared_ptr<Response> Put(const char *path, const Headers &headers,
                                size_t content_length,
                                ContentProvider content_provider,
                                const char *content_type);

  std::shared_ptr<Response> Put(const char *path, const Params &params);

  std::shared_ptr<Response> Put(const char *path, const Headers &headers,
                                const Params &params);

  std::shared_ptr<Response> Patch(const char *path, const std::string &body,
                                  const char *content_type);

  std::shared_ptr<Response> Patch(const char *path, const Headers &headers,
                                  const std::string &body,
                                  const char *content_type);

  std::shared_ptr<Response> Patch(const char *path, size_t content_length,
                                  ContentProvider content_provider,
                                  const char *content_type);

  std::shared_ptr<Response> Patch(const char *path, const Headers &headers,
                                  size_t content_length,
                                  ContentProvider content_provider,
                                  const char *content_type);

  std::shared_ptr<Response> Delete(const char *path);

  std::shared_ptr<Response> Delete(const char *path, const std::string &body,
                                   const char *content_type);

  std::shared_ptr<Response> Delete(const char *path, const Headers &headers);

  std::shared_ptr<Response> Delete(const char *path, const Headers &headers,
                                   const std::string &body,
                                   const char *content_type);

  std::shared_ptr<Response> Options(const char *path);

  std::shared_ptr<Response> Options(const char *path, const Headers &headers);

  bool send(const Request &req, Response &res);

  bool send(const std::vector<Request> &requests,
            std::vector<Response> &responses);

  void set_timeout_sec(time_t timeout_sec);

  void set_read_timeout(time_t sec, time_t usec);

  void set_keep_alive_max_count(size_t count);

  void set_basic_auth(const char *username, const char *password);

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  void set_digest_auth(const char *username, const char *password);
#endif

  void set_follow_location(bool on);

  void set_compress(bool on);

  void set_interface(const char *intf);

  void set_proxy(const char *host, int port);

  void set_proxy_basic_auth(const char *username, const char *password);

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  void set_proxy_digest_auth(const char *username, const char *password);
#endif

  void set_logger(Logger logger);

protected:
  bool process_request(Stream &strm, const Request &req, Response &res,
                       bool last_connection, bool &connection_close);

  const std::string host_;
  const int port_;
  const std::string host_and_port_;

  // Settings
  std::string client_cert_path_;
  std::string client_key_path_;

  time_t timeout_sec_ = 300;
  time_t read_timeout_sec_ = CPPHTTPLIB_READ_TIMEOUT_SECOND;
  time_t read_timeout_usec_ = CPPHTTPLIB_READ_TIMEOUT_USECOND;

  size_t keep_alive_max_count_ = CPPHTTPLIB_KEEPALIVE_MAX_COUNT;

  std::string basic_auth_username_;
  std::string basic_auth_password_;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  std::string digest_auth_username_;
  std::string digest_auth_password_;
#endif

  bool follow_location_ = false;

  bool compress_ = false;

  std::string interface_;

  std::string proxy_host_;
  int proxy_port_;

  std::string proxy_basic_auth_username_;
  std::string proxy_basic_auth_password_;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  std::string proxy_digest_auth_username_;
  std::string proxy_digest_auth_password_;
#endif

  Logger logger_;

  void copy_settings(const Client &rhs) {
    client_cert_path_ = rhs.client_cert_path_;
    client_key_path_ = rhs.client_key_path_;
    timeout_sec_ = rhs.timeout_sec_;
    read_timeout_sec_ = rhs.read_timeout_sec_;
    read_timeout_usec_ = rhs.read_timeout_usec_;
    keep_alive_max_count_ = rhs.keep_alive_max_count_;
    basic_auth_username_ = rhs.basic_auth_username_;
    basic_auth_password_ = rhs.basic_auth_password_;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    digest_auth_username_ = rhs.digest_auth_username_;
    digest_auth_password_ = rhs.digest_auth_password_;
#endif
    follow_location_ = rhs.follow_location_;
    compress_ = rhs.compress_;
    interface_ = rhs.interface_;
    proxy_host_ = rhs.proxy_host_;
    proxy_port_ = rhs.proxy_port_;
    proxy_basic_auth_username_ = rhs.proxy_basic_auth_username_;
    proxy_basic_auth_password_ = rhs.proxy_basic_auth_password_;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    proxy_digest_auth_username_ = rhs.proxy_digest_auth_username_;
    proxy_digest_auth_password_ = rhs.proxy_digest_auth_password_;
#endif
    logger_ = rhs.logger_;
  }

private:
  socket_t create_client_socket() const;
  bool read_response_line(Stream &strm, Response &res);
  bool write_request(Stream &strm, const Request &req, bool last_connection);
  bool redirect(const Request &req, Response &res);
  bool handle_request(Stream &strm, const Request &req, Response &res,
                      bool last_connection, bool &connection_close);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  bool connect(socket_t sock, Response &res, bool &error);
#endif

  std::shared_ptr<Response> send_with_content_provider(
      const char *method, const char *path, const Headers &headers,
      const std::string &body, size_t content_length,
      ContentProvider content_provider, const char *content_type);

  virtual bool process_and_close_socket(
      socket_t sock, size_t request_count,
      std::function<bool(Stream &strm, bool last_connection,
                         bool &connection_close)>
          callback);

  virtual bool is_ssl() const;
};

inline void Get(std::vector<Request> &requests, const char *path,
                const Headers &headers) {
  Request req;
  req.method = "GET";
  req.path = path;
  req.headers = headers;
  requests.emplace_back(std::move(req));
}

inline void Get(std::vector<Request> &requests, const char *path) {
  Get(requests, path, Headers());
}

inline void Post(std::vector<Request> &requests, const char *path,
                 const Headers &headers, const std::string &body,
                 const char *content_type) {
  Request req;
  req.method = "POST";
  req.path = path;
  req.headers = headers;
  req.headers.emplace("Content-Type", content_type);
  req.body = body;
  requests.emplace_back(std::move(req));
}

inline void Post(std::vector<Request> &requests, const char *path,
                 const std::string &body, const char *content_type) {
  Post(requests, path, Headers(), body, content_type);
}

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
class SSLServer : public Server {
public:
  SSLServer(const char *cert_path, const char *private_key_path,
            const char *client_ca_cert_file_path = nullptr,
            const char *client_ca_cert_dir_path = nullptr);

  virtual ~SSLServer();

  virtual bool is_valid() const;

private:
  virtual bool process_and_close_socket(socket_t sock);

  SSL_CTX *ctx_;
  std::mutex ctx_mutex_;
};

class SSLClient : public Client {
public:
  SSLClient(const std::string &host, int port = 443,
            const std::string &client_cert_path = std::string(),
            const std::string &client_key_path = std::string());

  virtual ~SSLClient();

  virtual bool is_valid() const;

  void set_ca_cert_path(const char *ca_ceert_file_path,
                        const char *ca_cert_dir_path = nullptr);

  void enable_server_certificate_verification(bool enabled);

  long get_openssl_verify_result() const;

  SSL_CTX *ssl_context() const noexcept;

private:
  virtual bool process_and_close_socket(
      socket_t sock, size_t request_count,
      std::function<bool(Stream &strm, bool last_connection,
                         bool &connection_close)>
          callback);
  virtual bool is_ssl() const;

  bool verify_host(X509 *server_cert) const;
  bool verify_host_with_subject_alt_name(X509 *server_cert) const;
  bool verify_host_with_common_name(X509 *server_cert) const;
  bool check_host_name(const char *pattern, size_t pattern_len) const;

  SSL_CTX *ctx_;
  std::mutex ctx_mutex_;
  std::vector<std::string> host_components_;

  std::string ca_cert_file_path_;
  std::string ca_cert_dir_path_;
  bool server_certificate_verification_ = false;
  long verify_result_ = 0;
};
#endif

// ----------------------------------------------------------------------------

/*
 * Implementation
 */

namespace detail {

inline bool is_hex(char c, int &v) {
  if (0x20 <= c && isdigit(c)) {
    v = c - '0';
    return true;
  } else if ('A' <= c && c <= 'F') {
    v = c - 'A' + 10;
    return true;
  } else if ('a' <= c && c <= 'f') {
    v = c - 'a' + 10;
    return true;
  }
  return false;
}

inline bool from_hex_to_i(const std::string &s, size_t i, size_t cnt,
                          int &val) {
  if (i >= s.size()) { return false; }

  val = 0;
  for (; cnt; i++, cnt--) {
    if (!s[i]) { return false; }
    int v = 0;
    if (is_hex(s[i], v)) {
      val = val * 16 + v;
    } else {
      return false;
    }
  }
  return true;
}

inline std::string from_i_to_hex(size_t n) {
  const char *charset = "0123456789abcdef";
  std::string ret;
  do {
    ret = charset[n & 15] + ret;
    n >>= 4;
  } while (n > 0);
  return ret;
}

inline size_t to_utf8(int code, char *buff) {
  if (code < 0x0080) {
    buff[0] = (code & 0x7F);
    return 1;
  } else if (code < 0x0800) {
    buff[0] = (0xC0 | ((code >> 6) & 0x1F));
    buff[1] = (0x80 | (code & 0x3F));
    return 2;
  } else if (code < 0xD800) {
    buff[0] = (0xE0 | ((code >> 12) & 0xF));
    buff[1] = (0x80 | ((code >> 6) & 0x3F));
    buff[2] = (0x80 | (code & 0x3F));
    return 3;
  } else if (code < 0xE000) { // D800 - DFFF is invalid...
    return 0;
  } else if (code < 0x10000) {
    buff[0] = (0xE0 | ((code >> 12) & 0xF));
    buff[1] = (0x80 | ((code >> 6) & 0x3F));
    buff[2] = (0x80 | (code & 0x3F));
    return 3;
  } else if (code < 0x110000) {
    buff[0] = (0xF0 | ((code >> 18) & 0x7));
    buff[1] = (0x80 | ((code >> 12) & 0x3F));
    buff[2] = (0x80 | ((code >> 6) & 0x3F));
    buff[3] = (0x80 | (code & 0x3F));
    return 4;
  }

  // NOTREACHED
  return 0;
}

// NOTE: This code came up with the following stackoverflow post:
// https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
inline std::string base64_encode(const std::string &in) {
  static const auto lookup =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  std::string out;
  out.reserve(in.size());

  int val = 0;
  int valb = -6;

  for (uint8_t c : in) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(lookup[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }

  if (valb > -6) { out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]); }

  while (out.size() % 4) {
    out.push_back('=');
  }

  return out;
}

inline bool is_file(const std::string &path) {
  struct stat st;
  return stat(path.c_str(), &st) >= 0 && S_ISREG(st.st_mode);
}

inline bool is_dir(const std::string &path) {
  struct stat st;
  return stat(path.c_str(), &st) >= 0 && S_ISDIR(st.st_mode);
}

inline bool is_valid_path(const std::string &path) {
  size_t level = 0;
  size_t i = 0;

  // Skip slash
  while (i < path.size() && path[i] == '/') {
    i++;
  }

  while (i < path.size()) {
    // Read component
    auto beg = i;
    while (i < path.size() && path[i] != '/') {
      i++;
    }

    auto len = i - beg;
    assert(len > 0);

    if (!path.compare(beg, len, ".")) {
      ;
    } else if (!path.compare(beg, len, "..")) {
      if (level == 0) { return false; }
      level--;
    } else {
      level++;
    }

    // Skip slash
    while (i < path.size() && path[i] == '/') {
      i++;
    }
  }

  return true;
}

inline void read_file(const std::string &path, std::string &out) {
  std::ifstream fs(path, std::ios_base::binary);
  fs.seekg(0, std::ios_base::end);
  auto size = fs.tellg();
  fs.seekg(0);
  out.resize(static_cast<size_t>(size));
  fs.read(&out[0], size);
}

inline std::string file_extension(const std::string &path) {
  std::smatch m;
  static auto re = std::regex("\\.([a-zA-Z0-9]+)$");
  if (std::regex_search(path, m, re)) { return m[1].str(); }
  return std::string();
}

template <class Fn> void split(const char *b, const char *e, char d, Fn fn) {
  int i = 0;
  int beg = 0;

  while (e ? (b + i != e) : (b[i] != '\0')) {
    if (b[i] == d) {
      fn(&b[beg], &b[i]);
      beg = i + 1;
    }
    i++;
  }

  if (i) { fn(&b[beg], &b[i]); }
}

// NOTE: until the read size reaches `fixed_buffer_size`, use `fixed_buffer`
// to store data. The call can set memory on stack for performance.
class stream_line_reader {
public:
  stream_line_reader(Stream &strm, char *fixed_buffer, size_t fixed_buffer_size)
      : strm_(strm), fixed_buffer_(fixed_buffer),
        fixed_buffer_size_(fixed_buffer_size) {}

  const char *ptr() const {
    if (glowable_buffer_.empty()) {
      return fixed_buffer_;
    } else {
      return glowable_buffer_.data();
    }
  }

  size_t size() const {
    if (glowable_buffer_.empty()) {
      return fixed_buffer_used_size_;
    } else {
      return glowable_buffer_.size();
    }
  }

  bool end_with_crlf() const {
    auto end = ptr() + size();
    return size() >= 2 && end[-2] == '\r' && end[-1] == '\n';
  }

  bool getline() {
    fixed_buffer_used_size_ = 0;
    glowable_buffer_.clear();

    for (size_t i = 0;; i++) {
      char byte;
      auto n = strm_.read(&byte, 1);

      if (n < 0) {
        return false;
      } else if (n == 0) {
        if (i == 0) {
          return false;
        } else {
          break;
        }
      }

      append(byte);

      if (byte == '\n') { break; }
    }

    return true;
  }

private:
  void append(char c) {
    if (fixed_buffer_used_size_ < fixed_buffer_size_ - 1) {
      fixed_buffer_[fixed_buffer_used_size_++] = c;
      fixed_buffer_[fixed_buffer_used_size_] = '\0';
    } else {
      if (glowable_buffer_.empty()) {
        assert(fixed_buffer_[fixed_buffer_used_size_] == '\0');
        glowable_buffer_.assign(fixed_buffer_, fixed_buffer_used_size_);
      }
      glowable_buffer_ += c;
    }
  }

  Stream &strm_;
  char *fixed_buffer_;
  const size_t fixed_buffer_size_;
  size_t fixed_buffer_used_size_ = 0;
  std::string glowable_buffer_;
};

inline int close_socket(socket_t sock) {
#ifdef _WIN32
  return closesocket(sock);
#else
  return close(sock);
#endif
}

inline int select_read(socket_t sock, time_t sec, time_t usec) {
#ifdef CPPHTTPLIB_USE_POLL
  struct pollfd pfd_read;
  pfd_read.fd = sock;
  pfd_read.events = POLLIN;

  auto timeout = static_cast<int>(sec * 1000 + usec / 1000);

  return poll(&pfd_read, 1, timeout);
#else
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(sock, &fds);

  timeval tv;
  tv.tv_sec = static_cast<long>(sec);
  tv.tv_usec = static_cast<long>(usec);

  return select(static_cast<int>(sock + 1), &fds, nullptr, nullptr, &tv);
#endif
}

inline int select_write(socket_t sock, time_t sec, time_t usec) {
#ifdef CPPHTTPLIB_USE_POLL
  struct pollfd pfd_read;
  pfd_read.fd = sock;
  pfd_read.events = POLLOUT;

  auto timeout = static_cast<int>(sec * 1000 + usec / 1000);

  return poll(&pfd_read, 1, timeout);
#else
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(sock, &fds);

  timeval tv;
  tv.tv_sec = static_cast<long>(sec);
  tv.tv_usec = static_cast<long>(usec);

  return select(static_cast<int>(sock + 1), nullptr, &fds, nullptr, &tv);
#endif
}

inline bool wait_until_socket_is_ready(socket_t sock, time_t sec, time_t usec) {
#ifdef CPPHTTPLIB_USE_POLL
  struct pollfd pfd_read;
  pfd_read.fd = sock;
  pfd_read.events = POLLIN | POLLOUT;

  auto timeout = static_cast<int>(sec * 1000 + usec / 1000);

  if (poll(&pfd_read, 1, timeout) > 0 &&
      pfd_read.revents & (POLLIN | POLLOUT)) {
    int error = 0;
    socklen_t len = sizeof(error);
    return getsockopt(sock, SOL_SOCKET, SO_ERROR,
                      reinterpret_cast<char *>(&error), &len) >= 0 &&
           !error;
  }
  return false;
#else
  fd_set fdsr;
  FD_ZERO(&fdsr);
  FD_SET(sock, &fdsr);

  auto fdsw = fdsr;
  auto fdse = fdsr;

  timeval tv;
  tv.tv_sec = static_cast<long>(sec);
  tv.tv_usec = static_cast<long>(usec);

  if (select(static_cast<int>(sock + 1), &fdsr, &fdsw, &fdse, &tv) > 0 &&
      (FD_ISSET(sock, &fdsr) || FD_ISSET(sock, &fdsw))) {
    int error = 0;
    socklen_t len = sizeof(error);
    return getsockopt(sock, SOL_SOCKET, SO_ERROR,
                      reinterpret_cast<char *>(&error), &len) >= 0 &&
           !error;
  }
  return false;
#endif
}

class SocketStream : public Stream {
public:
  SocketStream(socket_t sock, time_t read_timeout_sec,
               time_t read_timeout_usec);
  ~SocketStream() override;

  bool is_readable() const override;
  bool is_writable() const override;
  int read(char *ptr, size_t size) override;
  int write(const char *ptr, size_t size) override;
  std::string get_remote_addr() const override;

private:
  socket_t sock_;
  time_t read_timeout_sec_;
  time_t read_timeout_usec_;
};

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
class SSLSocketStream : public Stream {
public:
  SSLSocketStream(socket_t sock, SSL *ssl, time_t read_timeout_sec,
                  time_t read_timeout_usec);
  virtual ~SSLSocketStream();

  bool is_readable() const override;
  bool is_writable() const override;
  int read(char *ptr, size_t size) override;
  int write(const char *ptr, size_t size) override;
  std::string get_remote_addr() const override;

private:
  socket_t sock_;
  SSL *ssl_;
  time_t read_timeout_sec_;
  time_t read_timeout_usec_;
};
#endif

class BufferStream : public Stream {
public:
  BufferStream() = default;
  ~BufferStream() override = default;

  bool is_readable() const override;
  bool is_writable() const override;
  int read(char *ptr, size_t size) override;
  int write(const char *ptr, size_t size) override;
  std::string get_remote_addr() const override;

  const std::string &get_buffer() const;

private:
  std::string buffer;
  int position = 0;
};

template <typename T>
inline bool process_socket(bool is_client_request, socket_t sock,
                           size_t keep_alive_max_count, time_t read_timeout_sec,
                           time_t read_timeout_usec, T callback) {
  assert(keep_alive_max_count > 0);

  auto ret = false;

  if (keep_alive_max_count > 1) {
    auto count = keep_alive_max_count;
    while (count > 0 &&
           (is_client_request ||
            select_read(sock, CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND,
                        CPPHTTPLIB_KEEPALIVE_TIMEOUT_USECOND) > 0)) {
      SocketStream strm(sock, read_timeout_sec, read_timeout_usec);
      auto last_connection = count == 1;
      auto connection_close = false;

      ret = callback(strm, last_connection, connection_close);
      if (!ret || connection_close) { break; }

      count--;
    }
  } else { // keep_alive_max_count  is 0 or 1
    SocketStream strm(sock, read_timeout_sec, read_timeout_usec);
    auto dummy_connection_close = false;
    ret = callback(strm, true, dummy_connection_close);
  }

  return ret;
}

template <typename T>
inline bool process_and_close_socket(bool is_client_request, socket_t sock,
                                     size_t keep_alive_max_count,
                                     time_t read_timeout_sec,
                                     time_t read_timeout_usec, T callback) {
  auto ret = process_socket(is_client_request, sock, keep_alive_max_count,
                            read_timeout_sec, read_timeout_usec, callback);
  close_socket(sock);
  return ret;
}

inline int shutdown_socket(socket_t sock) {
#ifdef _WIN32
  return shutdown(sock, SD_BOTH);
#else
  return shutdown(sock, SHUT_RDWR);
#endif
}

template <typename Fn>
socket_t create_socket(const char *host, int port, Fn fn,
                       int socket_flags = 0) {
#ifdef _WIN32
#define SO_SYNCHRONOUS_NONALERT 0x20
#define SO_OPENTYPE 0x7008

  int opt = SO_SYNCHRONOUS_NONALERT;
  setsockopt(INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE, (char *)&opt,
             sizeof(opt));
#endif

  // Get address info
  struct addrinfo hints;
  struct addrinfo *result;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = socket_flags;
  hints.ai_protocol = 0;

  auto service = std::to_string(port);

  if (getaddrinfo(host, service.c_str(), &hints, &result)) {
    return INVALID_SOCKET;
  }

  for (auto rp = result; rp; rp = rp->ai_next) {
    // Create a socket
#ifdef _WIN32
    auto sock = WSASocketW(rp->ai_family, rp->ai_socktype, rp->ai_protocol,
                           nullptr, 0, WSA_FLAG_NO_HANDLE_INHERIT);
    /**
     * Since the WSA_FLAG_NO_HANDLE_INHERIT is only supported on Windows 7 SP1
     * and above the socket creation fails on older Windows Systems.
     *
     * Let's try to create a socket the old way in this case.
     *
     * Reference:
     * https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsasocketa
     *
     * WSA_FLAG_NO_HANDLE_INHERIT:
     * This flag is supported on Windows 7 with SP1, Windows Server 2008 R2 with
     * SP1, and later
     *
     */
    if (sock == INVALID_SOCKET) {
      sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    }
#else
    auto sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
#endif
    if (sock == INVALID_SOCKET) { continue; }

#ifndef _WIN32
    if (fcntl(sock, F_SETFD, FD_CLOEXEC) == -1) { continue; }
#endif

    // Make 'reuse address' option available
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&yes),
               sizeof(yes));
#ifdef SO_REUSEPORT
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<char *>(&yes),
               sizeof(yes));
#endif

    // bind or connect
    if (fn(sock, *rp)) {
      freeaddrinfo(result);
      return sock;
    }

    close_socket(sock);
  }

  freeaddrinfo(result);
  return INVALID_SOCKET;
}

inline void set_nonblocking(socket_t sock, bool nonblocking) {
#ifdef _WIN32
  auto flags = nonblocking ? 1UL : 0UL;
  ioctlsocket(sock, FIONBIO, &flags);
#else
  auto flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL,
        nonblocking ? (flags | O_NONBLOCK) : (flags & (~O_NONBLOCK)));
#endif
}

inline bool is_connection_error() {
#ifdef _WIN32
  return WSAGetLastError() != WSAEWOULDBLOCK;
#else
  return errno != EINPROGRESS;
#endif
}

inline bool bind_ip_address(socket_t sock, const char *host) {
  struct addrinfo hints;
  struct addrinfo *result;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;

  if (getaddrinfo(host, "0", &hints, &result)) { return false; }

  auto ret = false;
  for (auto rp = result; rp; rp = rp->ai_next) {
    const auto &ai = *rp;
    if (!::bind(sock, ai.ai_addr, static_cast<int>(ai.ai_addrlen))) {
      ret = true;
      break;
    }
  }

  freeaddrinfo(result);
  return ret;
}

inline std::string if2ip(const std::string &ifn) {
#ifndef _WIN32
  struct ifaddrs *ifap;
  getifaddrs(&ifap);
  for (auto ifa = ifap; ifa; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr && ifn == ifa->ifa_name) {
      if (ifa->ifa_addr->sa_family == AF_INET) {
        auto sa = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
        char buf[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &sa->sin_addr, buf, INET_ADDRSTRLEN)) {
          freeifaddrs(ifap);
          return std::string(buf, INET_ADDRSTRLEN);
        }
      }
    }
  }
  freeifaddrs(ifap);
#endif
  return std::string();
}

inline socket_t create_client_socket(const char *host, int port,
                                     time_t timeout_sec,
                                     const std::string &intf) {
  return create_socket(
      host, port, [&](socket_t sock, struct addrinfo &ai) -> bool {
        if (!intf.empty()) {
          auto ip = if2ip(intf);
          if (ip.empty()) { ip = intf; }
          if (!bind_ip_address(sock, ip.c_str())) { return false; }
        }

        set_nonblocking(sock, true);

        auto ret = ::connect(sock, ai.ai_addr, static_cast<int>(ai.ai_addrlen));
        if (ret < 0) {
          if (is_connection_error() ||
              !wait_until_socket_is_ready(sock, timeout_sec, 0)) {
            close_socket(sock);
            return false;
          }
        }

        set_nonblocking(sock, false);
        return true;
      });
}

inline std::string get_remote_addr(socket_t sock) {
  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);

  if (!getpeername(sock, reinterpret_cast<struct sockaddr *>(&addr), &len)) {
    std::array<char, NI_MAXHOST> ipstr{};

    if (!getnameinfo(reinterpret_cast<struct sockaddr *>(&addr), len,
                     ipstr.data(), ipstr.size(), nullptr, 0, NI_NUMERICHOST)) {
      return ipstr.data();
    }
  }

  return std::string();
}

inline const char *
find_content_type(const std::string &path,
                  const std::map<std::string, std::string> &user_data) {
  auto ext = file_extension(path);

  auto it = user_data.find(ext);
  if (it != user_data.end()) { return it->second.c_str(); }

  if (ext == "txt") {
    return "text/plain";
  } else if (ext == "html" || ext == "htm") {
    return "text/html";
  } else if (ext == "css") {
    return "text/css";
  } else if (ext == "jpeg" || ext == "jpg") {
    return "image/jpg";
  } else if (ext == "png") {
    return "image/png";
  } else if (ext == "gif") {
    return "image/gif";
  } else if (ext == "svg") {
    return "image/svg+xml";
  } else if (ext == "ico") {
    return "image/x-icon";
  } else if (ext == "json") {
    return "application/json";
  } else if (ext == "pdf") {
    return "application/pdf";
  } else if (ext == "js") {
    return "application/javascript";
  } else if (ext == "wasm") {
    return "application/wasm";
  } else if (ext == "xml") {
    return "application/xml";
  } else if (ext == "xhtml") {
    return "application/xhtml+xml";
  }
  return nullptr;
}

inline const char *status_message(int status) {
  switch (status) {
  case 100: return "Continue";
  case 200: return "OK";
  case 202: return "Accepted";
  case 204: return "No Content";
  case 206: return "Partial Content";
  case 301: return "Moved Permanently";
  case 302: return "Found";
  case 303: return "See Other";
  case 304: return "Not Modified";
  case 400: return "Bad Request";
  case 401: return "Unauthorized";
  case 403: return "Forbidden";
  case 404: return "Not Found";
  case 413: return "Payload Too Large";
  case 414: return "Request-URI Too Long";
  case 415: return "Unsupported Media Type";
  case 416: return "Range Not Satisfiable";
  case 417: return "Expectation Failed";
  case 503: return "Service Unavailable";

  default:
  case 500: return "Internal Server Error";
  }
}

#ifdef CPPHTTPLIB_ZLIB_SUPPORT
inline bool can_compress(const std::string &content_type) {
  return !content_type.find("text/") || content_type == "image/svg+xml" ||
         content_type == "application/javascript" ||
         content_type == "application/json" ||
         content_type == "application/xml" ||
         content_type == "application/xhtml+xml";
}

inline bool compress(std::string &content) {
  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;

  auto ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8,
                          Z_DEFAULT_STRATEGY);
  if (ret != Z_OK) { return false; }

  strm.avail_in = content.size();
  strm.next_in =
      const_cast<Bytef *>(reinterpret_cast<const Bytef *>(content.data()));

  std::string compressed;

  std::array<char, 16384> buff{};
  do {
    strm.avail_out = buff.size();
    strm.next_out = reinterpret_cast<Bytef *>(buff.data());
    ret = deflate(&strm, Z_FINISH);
    assert(ret != Z_STREAM_ERROR);
    compressed.append(buff.data(), buff.size() - strm.avail_out);
  } while (strm.avail_out == 0);

  assert(ret == Z_STREAM_END);
  assert(strm.avail_in == 0);

  content.swap(compressed);

  deflateEnd(&strm);
  return true;
}

class decompressor {
public:
  decompressor() {
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // 15 is the value of wbits, which should be at the maximum possible value
    // to ensure that any gzip stream can be decoded. The offset of 16 specifies
    // that the stream to decompress will be formatted with a gzip wrapper.
    is_valid_ = inflateInit2(&strm, 16 + 15) == Z_OK;
  }

  ~decompressor() { inflateEnd(&strm); }

  bool is_valid() const { return is_valid_; }

  template <typename T>
  bool decompress(const char *data, size_t data_length, T callback) {
    int ret = Z_OK;

    strm.avail_in = data_length;
    strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(data));

    std::array<char, 16384> buff{};
    do {
      strm.avail_out = buff.size();
      strm.next_out = reinterpret_cast<Bytef *>(buff.data());

      ret = inflate(&strm, Z_NO_FLUSH);
      assert(ret != Z_STREAM_ERROR);
      switch (ret) {
      case Z_NEED_DICT:
      case Z_DATA_ERROR:
      case Z_MEM_ERROR: inflateEnd(&strm); return false;
      }

      if (!callback(buff.data(), buff.size() - strm.avail_out)) {
        return false;
      }
    } while (strm.avail_out == 0);

    return ret == Z_OK || ret == Z_STREAM_END;
  }

private:
  bool is_valid_;
  z_stream strm;
};
#endif

inline bool has_header(const Headers &headers, const char *key) {
  return headers.find(key) != headers.end();
}

inline const char *get_header_value(const Headers &headers, const char *key,
                                    size_t id = 0, const char *def = nullptr) {
  auto it = headers.find(key);
  std::advance(it, id);
  if (it != headers.end()) { return it->second.c_str(); }
  return def;
}

inline uint64_t get_header_value_uint64(const Headers &headers, const char *key,
                                        int def = 0) {
  auto it = headers.find(key);
  if (it != headers.end()) {
    return std::strtoull(it->second.data(), nullptr, 10);
  }
  return def;
}

inline bool read_headers(Stream &strm, Headers &headers) {
  const auto bufsiz = 2048;
  char buf[bufsiz];
  stream_line_reader line_reader(strm, buf, bufsiz);

  for (;;) {
    if (!line_reader.getline()) { return false; }

    // Check if the line ends with CRLF.
    if (line_reader.end_with_crlf()) {
      // Blank line indicates end of headers.
      if (line_reader.size() == 2) { break; }
    } else {
      continue; // Skip invalid line.
    }

    // Skip trailing spaces and tabs.
    auto end = line_reader.ptr() + line_reader.size() - 2;
    while (line_reader.ptr() < end && (end[-1] == ' ' || end[-1] == '\t')) {
      end--;
    }

    // Horizontal tab and ' ' are considered whitespace and are ignored when on
    // the left or right side of the header value:
    //  - https://stackoverflow.com/questions/50179659/
    //  - https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
    static const std::regex re(R"((.+?):[\t ]*(.+))");

    std::cmatch m;
    if (std::regex_match(line_reader.ptr(), end, m, re)) {
      auto key = std::string(m[1]);
      auto val = std::string(m[2]);
      headers.emplace(key, val);
    }
  }

  return true;
}

inline bool read_content_with_length(Stream &strm, uint64_t len,
                                     Progress progress, ContentReceiver out) {
  char buf[CPPHTTPLIB_RECV_BUFSIZ];

  uint64_t r = 0;
  while (r < len) {
    auto read_len = static_cast<size_t>(len - r);
    auto n = strm.read(buf, std::min(read_len, CPPHTTPLIB_RECV_BUFSIZ));
    if (n <= 0) { return false; }

    if (!out(buf, n)) { return false; }

    r += n;

    if (progress) {
      if (!progress(r, len)) { return false; }
    }
  }

  return true;
}

inline void skip_content_with_length(Stream &strm, uint64_t len) {
  char buf[CPPHTTPLIB_RECV_BUFSIZ];
  uint64_t r = 0;
  while (r < len) {
    auto read_len = static_cast<size_t>(len - r);
    auto n = strm.read(buf, std::min(read_len, CPPHTTPLIB_RECV_BUFSIZ));
    if (n <= 0) { return; }
    r += n;
  }
}

inline bool read_content_without_length(Stream &strm, ContentReceiver out) {
  char buf[CPPHTTPLIB_RECV_BUFSIZ];
  for (;;) {
    auto n = strm.read(buf, CPPHTTPLIB_RECV_BUFSIZ);
    if (n < 0) {
      return false;
    } else if (n == 0) {
      return true;
    }
    if (!out(buf, n)) { return false; }
  }

  return true;
}

inline bool read_content_chunked(Stream &strm, ContentReceiver out) {
  const auto bufsiz = 16;
  char buf[bufsiz];

  stream_line_reader line_reader(strm, buf, bufsiz);

  if (!line_reader.getline()) { return false; }

  auto chunk_len = std::stoi(line_reader.ptr(), 0, 16);

  while (chunk_len > 0) {
    if (!read_content_with_length(strm, chunk_len, nullptr, out)) {
      return false;
    }

    if (!line_reader.getline()) { return false; }

    if (strcmp(line_reader.ptr(), "\r\n")) { break; }

    if (!line_reader.getline()) { return false; }

    chunk_len = std::stoi(line_reader.ptr(), 0, 16);
  }

  if (chunk_len == 0) {
    // Reader terminator after chunks
    if (!line_reader.getline() || strcmp(line_reader.ptr(), "\r\n"))
      return false;
  }

  return true;
}

inline bool is_chunked_transfer_encoding(const Headers &headers) {
  return !strcasecmp(get_header_value(headers, "Transfer-Encoding", 0, ""),
                     "chunked");
}

template <typename T>
bool read_content(Stream &strm, T &x, size_t payload_max_length, int &status,
                  Progress progress, ContentReceiver receiver) {

  ContentReceiver out = [&](const char *buf, size_t n) {
    return receiver(buf, n);
  };

#ifdef CPPHTTPLIB_ZLIB_SUPPORT
  decompressor decompressor;

  if (!decompressor.is_valid()) {
    status = 500;
    return false;
  }

  if (x.get_header_value("Content-Encoding") == "gzip") {
    out = [&](const char *buf, size_t n) {
      return decompressor.decompress(
          buf, n, [&](const char *buf, size_t n) { return receiver(buf, n); });
    };
  }
#else
  if (x.get_header_value("Content-Encoding") == "gzip") {
    status = 415;
    return false;
  }
#endif

  auto ret = true;
  auto exceed_payload_max_length = false;

  if (is_chunked_transfer_encoding(x.headers)) {
    ret = read_content_chunked(strm, out);
  } else if (!has_header(x.headers, "Content-Length")) {
    ret = read_content_without_length(strm, out);
  } else {
    auto len = get_header_value_uint64(x.headers, "Content-Length", 0);
    if (len > payload_max_length) {
      exceed_payload_max_length = true;
      skip_content_with_length(strm, len);
      ret = false;
    } else if (len > 0) {
      ret = read_content_with_length(strm, len, progress, out);
    }
  }

  if (!ret) { status = exceed_payload_max_length ? 413 : 400; }

  return ret;
}

template <typename T>
inline int write_headers(Stream &strm, const T &info, const Headers &headers) {
  auto write_len = 0;
  for (const auto &x : info.headers) {
    auto len =
        strm.write_format("%s: %s\r\n", x.first.c_str(), x.second.c_str());
    if (len < 0) { return len; }
    write_len += len;
  }
  for (const auto &x : headers) {
    auto len =
        strm.write_format("%s: %s\r\n", x.first.c_str(), x.second.c_str());
    if (len < 0) { return len; }
    write_len += len;
  }
  auto len = strm.write("\r\n");
  if (len < 0) { return len; }
  write_len += len;
  return write_len;
}

inline ssize_t write_content(Stream &strm, ContentProvider content_provider,
                             size_t offset, size_t length) {
  size_t begin_offset = offset;
  size_t end_offset = offset + length;
  while (offset < end_offset) {
    ssize_t written_length = 0;

    DataSink data_sink;
    data_sink.write = [&](const char *d, size_t l) {
      offset += l;
      written_length = strm.write(d, l);
    };
    data_sink.done = [&](void) { written_length = -1; };
    data_sink.is_writable = [&](void) { return strm.is_writable(); };

    content_provider(offset, end_offset - offset, data_sink);
    if (written_length < 0) { return written_length; }
  }
  return static_cast<ssize_t>(offset - begin_offset);
}

template <typename T>
inline ssize_t write_content_chunked(Stream &strm,
                                     ContentProvider content_provider,
                                     T is_shutting_down) {
  size_t offset = 0;
  auto data_available = true;
  ssize_t total_written_length = 0;
  while (data_available && !is_shutting_down()) {
    ssize_t written_length = 0;

    DataSink data_sink;
    data_sink.write = [&](const char *d, size_t l) {
      data_available = l > 0;
      offset += l;

      // Emit chunked response header and footer for each chunk
      auto chunk = from_i_to_hex(l) + "\r\n" + std::string(d, l) + "\r\n";
      written_length = strm.write(chunk);
    };
    data_sink.done = [&](void) {
      data_available = false;
      written_length = strm.write("0\r\n\r\n");
    };
    data_sink.is_writable = [&](void) { return strm.is_writable(); };

    content_provider(offset, 0, data_sink);

    if (written_length < 0) { return written_length; }
    total_written_length += written_length;
  }
  return total_written_length;
}

template <typename T>
inline bool redirect(T &cli, const Request &req, Response &res,
                     const std::string &path) {
  Request new_req = req;
  new_req.path = path;
  new_req.redirect_count -= 1;

  Response new_res;

  auto ret = cli.send(new_req, new_res);
  if (ret) { res = new_res; }
  return ret;
}

inline std::string encode_url(const std::string &s) {
  std::string result;

  for (auto i = 0; s[i]; i++) {
    switch (s[i]) {
    case ' ': result += "%20"; break;
    case '+': result += "%2B"; break;
    case '\r': result += "%0D"; break;
    case '\n': result += "%0A"; break;
    case '\'': result += "%27"; break;
    case ',': result += "%2C"; break;
    // case ':': result += "%3A"; break; // ok? probably...
    case ';': result += "%3B"; break;
    default:
      auto c = static_cast<uint8_t>(s[i]);
      if (c >= 0x80) {
        result += '%';
        char hex[4];
        size_t len = snprintf(hex, sizeof(hex) - 1, "%02X", c);
        assert(len == 2);
        result.append(hex, len);
      } else {
        result += s[i];
      }
      break;
    }
  }

  return result;
}

inline std::string decode_url(const std::string &s) {
  std::string result;

  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '%' && i + 1 < s.size()) {
      if (s[i + 1] == 'u') {
        int val = 0;
        if (from_hex_to_i(s, i + 2, 4, val)) {
          // 4 digits Unicode codes
          char buff[4];
          size_t len = to_utf8(val, buff);
          if (len > 0) { result.append(buff, len); }
          i += 5; // 'u0000'
        } else {
          result += s[i];
        }
      } else {
        int val = 0;
        if (from_hex_to_i(s, i + 1, 2, val)) {
          // 2 digits hex codes
          result += static_cast<char>(val);
          i += 2; // '00'
        } else {
          result += s[i];
        }
      }
    } else if (s[i] == '+') {
      result += ' ';
    } else {
      result += s[i];
    }
  }

  return result;
}

inline void parse_query_text(const std::string &s, Params &params) {
  split(&s[0], &s[s.size()], '&', [&](const char *b, const char *e) {
    std::string key;
    std::string val;
    split(b, e, '=', [&](const char *b2, const char *e2) {
      if (key.empty()) {
        key.assign(b2, e2);
      } else {
        val.assign(b2, e2);
      }
    });
    params.emplace(key, decode_url(val));
  });
}

inline bool parse_multipart_boundary(const std::string &content_type,
                                     std::string &boundary) {
  auto pos = content_type.find("boundary=");
  if (pos == std::string::npos) { return false; }

  boundary = content_type.substr(pos + 9);
  return true;
}

inline bool parse_range_header(const std::string &s, Ranges &ranges) {
  static auto re_first_range = std::regex(R"(bytes=(\d*-\d*(?:,\s*\d*-\d*)*))");
  std::smatch m;
  if (std::regex_match(s, m, re_first_range)) {
    auto pos = m.position(1);
    auto len = m.length(1);
    bool all_valid_ranges = true;
    split(&s[pos], &s[pos + len], ',', [&](const char *b, const char *e) {
      if (!all_valid_ranges) return;
      static auto re_another_range = std::regex(R"(\s*(\d*)-(\d*))");
      std::cmatch cm;
      if (std::regex_match(b, e, cm, re_another_range)) {
        ssize_t first = -1;
        if (!cm.str(1).empty()) {
          first = static_cast<ssize_t>(std::stoll(cm.str(1)));
        }

        ssize_t last = -1;
        if (!cm.str(2).empty()) {
          last = static_cast<ssize_t>(std::stoll(cm.str(2)));
        }

        if (first != -1 && last != -1 && first > last) {
          all_valid_ranges = false;
          return;
        }
        ranges.emplace_back(std::make_pair(first, last));
      }
    });
    return all_valid_ranges;
  }
  return false;
}

class MultipartFormDataParser {
public:
  MultipartFormDataParser() {}

  void set_boundary(const std::string &boundary) { boundary_ = boundary; }

  bool is_valid() const { return is_valid_; }

  template <typename T, typename U>
  bool parse(const char *buf, size_t n, T content_callback, U header_callback) {
    static const std::regex re_content_type(R"(^Content-Type:\s*(.*?)\s*$)",
                                            std::regex_constants::icase);

    static const std::regex re_content_disposition(
        "^Content-Disposition:\\s*form-data;\\s*name=\"(.*?)\"(?:;\\s*filename="
        "\"(.*?)\")?\\s*$",
        std::regex_constants::icase);

    buf_.append(buf, n); // TODO: performance improvement

    while (!buf_.empty()) {
      switch (state_) {
      case 0: { // Initial boundary
        auto pattern = dash_ + boundary_ + crlf_;
        if (pattern.size() > buf_.size()) { return true; }
        auto pos = buf_.find(pattern);
        if (pos != 0) {
          is_done_ = true;
          return false;
        }
        buf_.erase(0, pattern.size());
        off_ += pattern.size();
        state_ = 1;
        break;
      }
      case 1: { // New entry
        clear_file_info();
        state_ = 2;
        break;
      }
      case 2: { // Headers
        auto pos = buf_.find(crlf_);
        while (pos != std::string::npos) {
          // Empty line
          if (pos == 0) {
            if (!header_callback(file_)) {
              is_valid_ = false;
              is_done_ = false;
              return false;
            }
            buf_.erase(0, crlf_.size());
            off_ += crlf_.size();
            state_ = 3;
            break;
          }

          auto header = buf_.substr(0, pos);
          {
            std::smatch m;
            if (std::regex_match(header, m, re_content_type)) {
              file_.content_type = m[1];
            } else if (std::regex_match(header, m, re_content_disposition)) {
              file_.name = m[1];
              file_.filename = m[2];
            }
          }

          buf_.erase(0, pos + crlf_.size());
          off_ += pos + crlf_.size();
          pos = buf_.find(crlf_);
        }
        break;
      }
      case 3: { // Body
        {
          auto pattern = crlf_ + dash_;
          if (pattern.size() > buf_.size()) { return true; }

          auto pos = buf_.find(pattern);
          if (pos == std::string::npos) { pos = buf_.size(); }
          if (!content_callback(buf_.data(), pos)) {
            is_valid_ = false;
            is_done_ = false;
            return false;
          }

          off_ += pos;
          buf_.erase(0, pos);
        }

        {
          auto pattern = crlf_ + dash_ + boundary_;
          if (pattern.size() > buf_.size()) { return true; }

          auto pos = buf_.find(pattern);
          if (pos != std::string::npos) {
            if (!content_callback(buf_.data(), pos)) {
              is_valid_ = false;
              is_done_ = false;
              return false;
            }

            off_ += pos + pattern.size();
            buf_.erase(0, pos + pattern.size());
            state_ = 4;
          } else {
            if (!content_callback(buf_.data(), pattern.size())) {
              is_valid_ = false;
              is_done_ = false;
              return false;
            }

            off_ += pattern.size();
            buf_.erase(0, pattern.size());
          }
        }
        break;
      }
      case 4: { // Boundary
        if (crlf_.size() > buf_.size()) { return true; }
        if (buf_.find(crlf_) == 0) {
          buf_.erase(0, crlf_.size());
          off_ += crlf_.size();
          state_ = 1;
        } else {
          auto pattern = dash_ + crlf_;
          if (pattern.size() > buf_.size()) { return true; }
          if (buf_.find(pattern) == 0) {
            buf_.erase(0, pattern.size());
            off_ += pattern.size();
            is_valid_ = true;
            state_ = 5;
          } else {
            is_done_ = true;
            return true;
          }
        }
        break;
      }
      case 5: { // Done
        is_valid_ = false;
        return false;
      }
      }
    }

    return true;
  }

private:
  void clear_file_info() {
    file_.name.clear();
    file_.filename.clear();
    file_.content_type.clear();
  }

  const std::string dash_ = "--";
  const std::string crlf_ = "\r\n";
  std::string boundary_;

  std::string buf_;
  size_t state_ = 0;
  size_t is_valid_ = false;
  size_t is_done_ = false;
  size_t off_ = 0;
  MultipartFormData file_;
};

inline std::string to_lower(const char *beg, const char *end) {
  std::string out;
  auto it = beg;
  while (it != end) {
    out += static_cast<char>(::tolower(*it));
    it++;
  }
  return out;
}

inline std::string make_multipart_data_boundary() {
  static const char data[] =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  std::random_device seed_gen;
  std::mt19937 engine(seed_gen());

  std::string result = "--cpp-httplib-multipart-data-";

  for (auto i = 0; i < 16; i++) {
    result += data[engine() % (sizeof(data) - 1)];
  }

  return result;
}

inline std::pair<size_t, size_t>
get_range_offset_and_length(const Request &req, size_t content_length,
                            size_t index) {
  auto r = req.ranges[index];

  if (r.first == -1 && r.second == -1) {
    return std::make_pair(0, content_length);
  }

  if (r.first == -1) {
    r.first = content_length - r.second;
    r.second = content_length - 1;
  }

  if (r.second == -1) { r.second = content_length - 1; }

  return std::make_pair(r.first, r.second - r.first + 1);
}

inline std::string make_content_range_header_field(size_t offset, size_t length,
                                                   size_t content_length) {
  std::string field = "bytes ";
  field += std::to_string(offset);
  field += "-";
  field += std::to_string(offset + length - 1);
  field += "/";
  field += std::to_string(content_length);
  return field;
}

template <typename SToken, typename CToken, typename Content>
bool process_multipart_ranges_data(const Request &req, Response &res,
                                   const std::string &boundary,
                                   const std::string &content_type,
                                   SToken stoken, CToken ctoken,
                                   Content content) {
  for (size_t i = 0; i < req.ranges.size(); i++) {
    ctoken("--");
    stoken(boundary);
    ctoken("\r\n");
    if (!content_type.empty()) {
      ctoken("Content-Type: ");
      stoken(content_type);
      ctoken("\r\n");
    }

    auto offsets = get_range_offset_and_length(req, res.body.size(), i);
    auto offset = offsets.first;
    auto length = offsets.second;

    ctoken("Content-Range: ");
    stoken(make_content_range_header_field(offset, length, res.body.size()));
    ctoken("\r\n");
    ctoken("\r\n");
    if (!content(offset, length)) { return false; }
    ctoken("\r\n");
  }

  ctoken("--");
  stoken(boundary);
  ctoken("--\r\n");

  return true;
}

inline std::string make_multipart_ranges_data(const Request &req, Response &res,
                                              const std::string &boundary,
                                              const std::string &content_type) {
  std::string data;

  process_multipart_ranges_data(
      req, res, boundary, content_type,
      [&](const std::string &token) { data += token; },
      [&](const char *token) { data += token; },
      [&](size_t offset, size_t length) {
        data += res.body.substr(offset, length);
        return true;
      });

  return data;
}

inline size_t
get_multipart_ranges_data_length(const Request &req, Response &res,
                                 const std::string &boundary,
                                 const std::string &content_type) {
  size_t data_length = 0;

  process_multipart_ranges_data(
      req, res, boundary, content_type,
      [&](const std::string &token) { data_length += token.size(); },
      [&](const char *token) { data_length += strlen(token); },
      [&](size_t /*offset*/, size_t length) {
        data_length += length;
        return true;
      });

  return data_length;
}

inline bool write_multipart_ranges_data(Stream &strm, const Request &req,
                                        Response &res,
                                        const std::string &boundary,
                                        const std::string &content_type) {
  return process_multipart_ranges_data(
      req, res, boundary, content_type,
      [&](const std::string &token) { strm.write(token); },
      [&](const char *token) { strm.write(token); },
      [&](size_t offset, size_t length) {
        return write_content(strm, res.content_provider, offset, length) >= 0;
      });
}

inline std::pair<size_t, size_t>
get_range_offset_and_length(const Request &req, const Response &res,
                            size_t index) {
  auto r = req.ranges[index];

  if (r.second == -1) { r.second = res.content_length - 1; }

  return std::make_pair(r.first, r.second - r.first + 1);
}

inline bool expect_content(const Request &req) {
  if (req.method == "POST" || req.method == "PUT" || req.method == "PATCH" ||
      req.method == "PRI") {
    return true;
  }
  // TODO: check if Content-Length is set
  return false;
}

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
template <typename CTX, typename Init, typename Update, typename Final>
inline std::string message_digest(const std::string &s, Init init,
                                  Update update, Final final,
                                  size_t digest_length) {
  using namespace std;

  std::vector<unsigned char> md(digest_length, 0);
  CTX ctx;
  init(&ctx);
  update(&ctx, s.data(), s.size());
  final(md.data(), &ctx);

  stringstream ss;
  for (auto c : md) {
    ss << setfill('0') << setw(2) << hex << (unsigned int)c;
  }
  return ss.str();
}

inline std::string MD5(const std::string &s) {
  return message_digest<MD5_CTX>(s, MD5_Init, MD5_Update, MD5_Final,
                                 MD5_DIGEST_LENGTH);
}

inline std::string SHA_256(const std::string &s) {
  return message_digest<SHA256_CTX>(s, SHA256_Init, SHA256_Update, SHA256_Final,
                                    SHA256_DIGEST_LENGTH);
}

inline std::string SHA_512(const std::string &s) {
  return message_digest<SHA512_CTX>(s, SHA512_Init, SHA512_Update, SHA512_Final,
                                    SHA512_DIGEST_LENGTH);
}
#endif

#ifdef _WIN32
class WSInit {
public:
  WSInit() {
    WSADATA wsaData;
    WSAStartup(0x0002, &wsaData);
  }

  ~WSInit() { WSACleanup(); }
};

static WSInit wsinit_;
#endif

} // namespace detail

// Header utilities
inline std::pair<std::string, std::string> make_range_header(Ranges ranges) {
  std::string field = "bytes=";
  auto i = 0;
  for (auto r : ranges) {
    if (i != 0) { field += ", "; }
    if (r.first != -1) { field += std::to_string(r.first); }
    field += '-';
    if (r.second != -1) { field += std::to_string(r.second); }
    i++;
  }
  return std::make_pair("Range", field);
}

inline std::pair<std::string, std::string>
make_basic_authentication_header(const std::string &username,
                                 const std::string &password,
                                 bool is_proxy = false) {
  auto field = "Basic " + detail::base64_encode(username + ":" + password);
  auto key = is_proxy ? "Proxy-Authorization" : "Authorization";
  return std::make_pair(key, field);
}

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
inline std::pair<std::string, std::string> make_digest_authentication_header(
    const Request &req, const std::map<std::string, std::string> &auth,
    size_t cnonce_count, const std::string &cnonce, const std::string &username,
    const std::string &password, bool is_proxy = false) {
  using namespace std;

  string nc;
  {
    stringstream ss;
    ss << setfill('0') << setw(8) << hex << cnonce_count;
    nc = ss.str();
  }

  auto qop = auth.at("qop");
  if (qop.find("auth-int") != std::string::npos) {
    qop = "auth-int";
  } else {
    qop = "auth";
  }

  std::string algo = "MD5";
  if (auth.find("algorithm") != auth.end()) { algo = auth.at("algorithm"); }

  string response;
  {
    auto H = algo == "SHA-256"
                 ? detail::SHA_256
                 : algo == "SHA-512" ? detail::SHA_512 : detail::MD5;

    auto A1 = username + ":" + auth.at("realm") + ":" + password;

    auto A2 = req.method + ":" + req.path;
    if (qop == "auth-int") { A2 += ":" + H(req.body); }

    response = H(H(A1) + ":" + auth.at("nonce") + ":" + nc + ":" + cnonce +
                 ":" + qop + ":" + H(A2));
  }

  auto field = "Digest username=\"hello\", realm=\"" + auth.at("realm") +
               "\", nonce=\"" + auth.at("nonce") + "\", uri=\"" + req.path +
               "\", algorithm=" + algo + ", qop=" + qop + ", nc=\"" + nc +
               "\", cnonce=\"" + cnonce + "\", response=\"" + response + "\"";

  auto key = is_proxy ? "Proxy-Authorization" : "Authorization";
  return std::make_pair(key, field);
}
#endif

inline bool parse_www_authenticate(const httplib::Response &res,
                                   std::map<std::string, std::string> &auth,
                                   bool is_proxy) {
  auto auth_key = is_proxy ? "Proxy-Authenticate" : "WWW-Authenticate";
  if (res.has_header(auth_key)) {
    static auto re = std::regex(R"~((?:(?:,\s*)?(.+?)=(?:"(.*?)"|([^,]*))))~");
    auto s = res.get_header_value(auth_key);
    auto pos = s.find(' ');
    if (pos != std::string::npos) {
      auto type = s.substr(0, pos);
      if (type == "Basic") {
        return false;
      } else if (type == "Digest") {
        s = s.substr(pos + 1);
        auto beg = std::sregex_iterator(s.begin(), s.end(), re);
        for (auto i = beg; i != std::sregex_iterator(); ++i) {
          auto m = *i;
          auto key = s.substr(m.position(1), m.length(1));
          auto val = m.length(2) > 0 ? s.substr(m.position(2), m.length(2))
                                     : s.substr(m.position(3), m.length(3));
          auth[key] = val;
        }
        return true;
      }
    }
  }
  return false;
}

// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c/440240#answer-440240
inline std::string random_string(size_t length) {
  auto randchar = []() -> char {
    const char charset[] = "0123456789"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz";
    const size_t max_index = (sizeof(charset) - 1);
    return charset[rand() % max_index];
  };
  std::string str(length, 0);
  std::generate_n(str.begin(), length, randchar);
  return str;
}

// Request implementation
inline bool Request::has_header(const char *key) const {
  return detail::has_header(headers, key);
}

inline std::string Request::get_header_value(const char *key, size_t id) const {
  return detail::get_header_value(headers, key, id, "");
}

inline size_t Request::get_header_value_count(const char *key) const {
  auto r = headers.equal_range(key);
  return std::distance(r.first, r.second);
}

inline void Request::set_header(const char *key, const char *val) {
  headers.emplace(key, val);
}

inline void Request::set_header(const char *key, const std::string &val) {
  headers.emplace(key, val);
}

inline bool Request::has_param(const char *key) const {
  return params.find(key) != params.end();
}

inline std::string Request::get_param_value(const char *key, size_t id) const {
  auto it = params.find(key);
  std::advance(it, id);
  if (it != params.end()) { return it->second; }
  return std::string();
}

inline size_t Request::get_param_value_count(const char *key) const {
  auto r = params.equal_range(key);
  return std::distance(r.first, r.second);
}

inline bool Request::is_multipart_form_data() const {
  const auto &content_type = get_header_value("Content-Type");
  return !content_type.find("multipart/form-data");
}

inline bool Request::has_file(const char *key) const {
  return files.find(key) != files.end();
}

inline MultipartFormData Request::get_file_value(const char *key) const {
  auto it = files.find(key);
  if (it != files.end()) { return it->second; }
  return MultipartFormData();
}

// Response implementation
inline bool Response::has_header(const char *key) const {
  return headers.find(key) != headers.end();
}

inline std::string Response::get_header_value(const char *key,
                                              size_t id) const {
  return detail::get_header_value(headers, key, id, "");
}

inline size_t Response::get_header_value_count(const char *key) const {
  auto r = headers.equal_range(key);
  return std::distance(r.first, r.second);
}

inline void Response::set_header(const char *key, const char *val) {
  headers.emplace(key, val);
}

inline void Response::set_header(const char *key, const std::string &val) {
  headers.emplace(key, val);
}

inline void Response::set_redirect(const char *url) {
  set_header("Location", url);
  status = 302;
}

inline void Response::set_content(const char *s, size_t n,
                                  const char *content_type) {
  body.assign(s, n);
  set_header("Content-Type", content_type);
}

inline void Response::set_content(const std::string &s,
                                  const char *content_type) {
  body = s;
  set_header("Content-Type", content_type);
}

inline void Response::set_content_provider(
    size_t in_length,
    std::function<void(size_t offset, size_t length, DataSink &sink)> provider,
    std::function<void()> resource_releaser) {
  assert(in_length > 0);
  content_length = in_length;
  content_provider = [provider](size_t offset, size_t length, DataSink &sink) {
    provider(offset, length, sink);
  };
  content_provider_resource_releaser = resource_releaser;
}

inline void Response::set_chunked_content_provider(
    std::function<void(size_t offset, DataSink &sink)> provider,
    std::function<void()> resource_releaser) {
  content_length = 0;
  content_provider = [provider](size_t offset, size_t, DataSink &sink) {
    provider(offset, sink);
  };
  content_provider_resource_releaser = resource_releaser;
}

// Rstream implementation
inline int Stream::write(const char *ptr) { return write(ptr, strlen(ptr)); }

inline int Stream::write(const std::string &s) {
  return write(s.data(), s.size());
}

template <typename... Args>
inline int Stream::write_format(const char *fmt, const Args &... args) {
  std::array<char, 2048> buf;

#if defined(_MSC_VER) && _MSC_VER < 1900
  auto n = _snprintf_s(buf, bufsiz, buf.size() - 1, fmt, args...);
#else
  auto n = snprintf(buf.data(), buf.size() - 1, fmt, args...);
#endif
  if (n <= 0) { return n; }

  if (n >= static_cast<int>(buf.size()) - 1) {
    std::vector<char> glowable_buf(buf.size());

    while (n >= static_cast<int>(glowable_buf.size() - 1)) {
      glowable_buf.resize(glowable_buf.size() * 2);
#if defined(_MSC_VER) && _MSC_VER < 1900
      n = _snprintf_s(&glowable_buf[0], glowable_buf.size(),
                      glowable_buf.size() - 1, fmt, args...);
#else
      n = snprintf(&glowable_buf[0], glowable_buf.size() - 1, fmt, args...);
#endif
    }
    return write(&glowable_buf[0], n);
  } else {
    return write(buf.data(), n);
  }
}

namespace detail {

// Socket stream implementation
inline SocketStream::SocketStream(socket_t sock, time_t read_timeout_sec,
                                  time_t read_timeout_usec)
    : sock_(sock), read_timeout_sec_(read_timeout_sec),
      read_timeout_usec_(read_timeout_usec) {}

inline SocketStream::~SocketStream() {}

inline bool SocketStream::is_readable() const {
  return detail::select_read(sock_, read_timeout_sec_, read_timeout_usec_) > 0;
}

inline bool SocketStream::is_writable() const {
  return detail::select_write(sock_, 0, 0) > 0;
}

inline int SocketStream::read(char *ptr, size_t size) {
  if (is_readable()) { return recv(sock_, ptr, static_cast<int>(size), 0); }
  return -1;
}

inline int SocketStream::write(const char *ptr, size_t size) {
  if (is_writable()) { return send(sock_, ptr, static_cast<int>(size), 0); }
  return -1;
}

inline std::string SocketStream::get_remote_addr() const {
  return detail::get_remote_addr(sock_);
}

// Buffer stream implementation
inline bool BufferStream::is_readable() const { return true; }

inline bool BufferStream::is_writable() const { return true; }

inline int BufferStream::read(char *ptr, size_t size) {
#if defined(_MSC_VER) && _MSC_VER < 1900
  int len_read = static_cast<int>(buffer._Copy_s(ptr, size, size, position));
#else
  int len_read = static_cast<int>(buffer.copy(ptr, size, position));
#endif
  position += len_read;
  return len_read;
}

inline int BufferStream::write(const char *ptr, size_t size) {
  buffer.append(ptr, size);
  return static_cast<int>(size);
}

inline std::string BufferStream::get_remote_addr() const { return ""; }

inline const std::string &BufferStream::get_buffer() const { return buffer; }

} // namespace detail

// HTTP server implementation
inline Server::Server()
    : keep_alive_max_count_(CPPHTTPLIB_KEEPALIVE_MAX_COUNT),
      read_timeout_sec_(CPPHTTPLIB_READ_TIMEOUT_SECOND),
      read_timeout_usec_(CPPHTTPLIB_READ_TIMEOUT_USECOND),
      payload_max_length_(CPPHTTPLIB_PAYLOAD_MAX_LENGTH), is_running_(false),
      svr_sock_(INVALID_SOCKET) {
#ifndef _WIN32
  signal(SIGPIPE, SIG_IGN);
#endif
  new_task_queue = [] { return new ThreadPool(CPPHTTPLIB_THREAD_POOL_COUNT); };
}

inline Server::~Server() {}

inline Server &Server::Get(const char *pattern, Handler handler) {
  get_handlers_.push_back(std::make_pair(std::regex(pattern), handler));
  return *this;
}

inline Server &Server::Post(const char *pattern, Handler handler) {
  post_handlers_.push_back(std::make_pair(std::regex(pattern), handler));
  return *this;
}

inline Server &Server::Post(const char *pattern,
                            HandlerWithContentReader handler) {
  post_handlers_for_content_reader_.push_back(
      std::make_pair(std::regex(pattern), handler));
  return *this;
}

inline Server &Server::Put(const char *pattern, Handler handler) {
  put_handlers_.push_back(std::make_pair(std::regex(pattern), handler));
  return *this;
}

inline Server &Server::Put(const char *pattern,
                           HandlerWithContentReader handler) {
  put_handlers_for_content_reader_.push_back(
      std::make_pair(std::regex(pattern), handler));
  return *this;
}

inline Server &Server::Patch(const char *pattern, Handler handler) {
  patch_handlers_.push_back(std::make_pair(std::regex(pattern), handler));
  return *this;
}

inline Server &Server::Patch(const char *pattern,
                             HandlerWithContentReader handler) {
  patch_handlers_for_content_reader_.push_back(
      std::make_pair(std::regex(pattern), handler));
  return *this;
}

inline Server &Server::Delete(const char *pattern, Handler handler) {
  delete_handlers_.push_back(std::make_pair(std::regex(pattern), handler));
  return *this;
}

inline Server &Server::Options(const char *pattern, Handler handler) {
  options_handlers_.push_back(std::make_pair(std::regex(pattern), handler));
  return *this;
}

inline bool Server::set_base_dir(const char *dir, const char *mount_point) {
  return set_mount_point(mount_point, dir);
}

inline bool Server::set_mount_point(const char *mount_point, const char *dir) {
  if (detail::is_dir(dir)) {
    std::string mnt = mount_point ? mount_point : "/";
    if (!mnt.empty() && mnt[0] == '/') {
      base_dirs_.emplace_back(mnt, dir);
      return true;
    }
  }
  return false;
}

inline bool Server::remove_mount_point(const char *mount_point) {
  for (auto it = base_dirs_.begin(); it != base_dirs_.end(); ++it) {
    if (it->first == mount_point) {
      base_dirs_.erase(it);
      return true;
    }
  }
  return false;
}

inline void Server::set_file_extension_and_mimetype_mapping(const char *ext,
                                                            const char *mime) {
  file_extension_and_mimetype_map_[ext] = mime;
}

inline void Server::set_file_request_handler(Handler handler) {
  file_request_handler_ = std::move(handler);
}

inline void Server::set_error_handler(Handler handler) {
  error_handler_ = std::move(handler);
}

inline void Server::set_logger(Logger logger) { logger_ = std::move(logger); }

inline void
Server::set_expect_100_continue_handler(Expect100ContinueHandler handler) {
  expect_100_continue_handler_ = std::move(handler);
}

inline void Server::set_keep_alive_max_count(size_t count) {
  keep_alive_max_count_ = count;
}

inline void Server::set_read_timeout(time_t sec, time_t usec) {
  read_timeout_sec_ = sec;
  read_timeout_usec_ = usec;
}

inline void Server::set_payload_max_length(size_t length) {
  payload_max_length_ = length;
}

inline bool Server::bind_to_port(const char *host, int port, int socket_flags) {
  if (bind_internal(host, port, socket_flags) < 0) return false;
  return true;
}
inline int Server::bind_to_any_port(const char *host, int socket_flags) {
  return bind_internal(host, 0, socket_flags);
}

inline bool Server::listen_after_bind() { return listen_internal(); }

inline bool Server::listen(const char *host, int port, int socket_flags) {
  return bind_to_port(host, port, socket_flags) && listen_internal();
}

inline bool Server::is_running() const { return is_running_; }

inline void Server::stop() {
  if (is_running_) {
    assert(svr_sock_ != INVALID_SOCKET);
    std::atomic<socket_t> sock(svr_sock_.exchange(INVALID_SOCKET));
    detail::shutdown_socket(sock);
    detail::close_socket(sock);
  }
}

inline bool Server::parse_request_line(const char *s, Request &req) {
  const static std::regex re(
      "(GET|HEAD|POST|PUT|DELETE|CONNECT|OPTIONS|TRACE|PATCH|PRI) "
      "(([^?]+)(?:\\?(.*?))?) (HTTP/1\\.[01])\r\n");

  std::cmatch m;
  if (std::regex_match(s, m, re)) {
    req.version = std::string(m[5]);
    req.method = std::string(m[1]);
    req.target = std::string(m[2]);
    req.path = detail::decode_url(m[3]);

    // Parse query text
    auto len = std::distance(m[4].first, m[4].second);
    if (len > 0) { detail::parse_query_text(m[4], req.params); }

    return true;
  }

  return false;
}

inline bool Server::write_response(Stream &strm, bool last_connection,
                                   const Request &req, Response &res) {
  assert(res.status != -1);

  if (400 <= res.status && error_handler_) { error_handler_(req, res); }

  detail::BufferStream bstrm;

  // Response line
  if (!bstrm.write_format("HTTP/1.1 %d %s\r\n", res.status,
                          detail::status_message(res.status))) {
    return false;
  }

  // Headers
  if (last_connection || req.get_header_value("Connection") == "close") {
    res.set_header("Connection", "close");
  }

  if (!last_connection && req.get_header_value("Connection") == "Keep-Alive") {
    res.set_header("Connection", "Keep-Alive");
  }

  if (!res.has_header("Content-Type") &&
      (!res.body.empty() || res.content_length > 0)) {
    res.set_header("Content-Type", "text/plain");
  }

  if (!res.has_header("Accept-Ranges") && req.method == "HEAD") {
    res.set_header("Accept-Ranges", "bytes");
  }

  std::string content_type;
  std::string boundary;

  if (req.ranges.size() > 1) {
    boundary = detail::make_multipart_data_boundary();

    auto it = res.headers.find("Content-Type");
    if (it != res.headers.end()) {
      content_type = it->second;
      res.headers.erase(it);
    }

    res.headers.emplace("Content-Type",
                        "multipart/byteranges; boundary=" + boundary);
  }

  if (res.body.empty()) {
    if (res.content_length > 0) {
      size_t length = 0;
      if (req.ranges.empty()) {
        length = res.content_length;
      } else if (req.ranges.size() == 1) {
        auto offsets =
            detail::get_range_offset_and_length(req, res.content_length, 0);
        auto offset = offsets.first;
        length = offsets.second;
        auto content_range = detail::make_content_range_header_field(
            offset, length, res.content_length);
        res.set_header("Content-Range", content_range);
      } else {
        length = detail::get_multipart_ranges_data_length(req, res, boundary,
                                                          content_type);
      }
      res.set_header("Content-Length", std::to_string(length));
    } else {
      if (res.content_provider) {
        res.set_header("Transfer-Encoding", "chunked");
      } else {
        res.set_header("Content-Length", "0");
      }
    }
  } else {
    if (req.ranges.empty()) {
      ;
    } else if (req.ranges.size() == 1) {
      auto offsets =
          detail::get_range_offset_and_length(req, res.body.size(), 0);
      auto offset = offsets.first;
      auto length = offsets.second;
      auto content_range = detail::make_content_range_header_field(
          offset, length, res.body.size());
      res.set_header("Content-Range", content_range);
      res.body = res.body.substr(offset, length);
    } else {
      res.body =
          detail::make_multipart_ranges_data(req, res, boundary, content_type);
    }

#ifdef CPPHTTPLIB_ZLIB_SUPPORT
    // TODO: 'Accept-Encoding' has gzip, not gzip;q=0
    const auto &encodings = req.get_header_value("Accept-Encoding");
    if (encodings.find("gzip") != std::string::npos &&
        detail::can_compress(res.get_header_value("Content-Type"))) {
      if (detail::compress(res.body)) {
        res.set_header("Content-Encoding", "gzip");
      }
    }
#endif

    auto length = std::to_string(res.body.size());
    res.set_header("Content-Length", length);
  }

  if (!detail::write_headers(bstrm, res, Headers())) { return false; }

  // Flush buffer
  auto &data = bstrm.get_buffer();
  strm.write(data.data(), data.size());

  // Body
  if (req.method != "HEAD") {
    if (!res.body.empty()) {
      if (!strm.write(res.body)) { return false; }
    } else if (res.content_provider) {
      if (!write_content_with_provider(strm, req, res, boundary,
                                       content_type)) {
        return false;
      }
    }
  }

  // Log
  if (logger_) { logger_(req, res); }

  return true;
}

inline bool
Server::write_content_with_provider(Stream &strm, const Request &req,
                                    Response &res, const std::string &boundary,
                                    const std::string &content_type) {
  if (res.content_length) {
    if (req.ranges.empty()) {
      if (detail::write_content(strm, res.content_provider, 0,
                                res.content_length) < 0) {
        return false;
      }
    } else if (req.ranges.size() == 1) {
      auto offsets =
          detail::get_range_offset_and_length(req, res.content_length, 0);
      auto offset = offsets.first;
      auto length = offsets.second;
      if (detail::write_content(strm, res.content_provider, offset, length) <
          0) {
        return false;
      }
    } else {
      if (!detail::write_multipart_ranges_data(strm, req, res, boundary,
                                               content_type)) {
        return false;
      }
    }
  } else {
    auto is_shutting_down = [this]() {
      return this->svr_sock_ == INVALID_SOCKET;
    };
    if (detail::write_content_chunked(strm, res.content_provider,
                                      is_shutting_down) < 0) {
      return false;
    }
  }
  return true;
}

inline bool Server::read_content(Stream &strm, bool last_connection,
                                 Request &req, Response &res) {
  MultipartFormDataMap::iterator cur;
  auto ret = read_content_core(
      strm, last_connection, req, res,
      // Regular
      [&](const char *buf, size_t n) {
        if (req.body.size() + n > req.body.max_size()) { return false; }
        req.body.append(buf, n);
        return true;
      },
      // Multipart
      [&](const MultipartFormData &file) {
        cur = req.files.emplace(file.name, file);
        return true;
      },
      [&](const char *buf, size_t n) {
        auto &content = cur->second.content;
        if (content.size() + n > content.max_size()) { return false; }
        content.append(buf, n);
        return true;
      });

  const auto &content_type = req.get_header_value("Content-Type");
  if (!content_type.find("application/x-www-form-urlencoded")) {
    detail::parse_query_text(req.body, req.params);
  }

  return ret;
}

inline bool Server::read_content_with_content_receiver(
    Stream &strm, bool last_connection, Request &req, Response &res,
    ContentReceiver receiver, MultipartContentHeader multipart_header,
    ContentReceiver multipart_receiver) {
  return read_content_core(strm, last_connection, req, res, receiver,
                           multipart_header, multipart_receiver);
}

inline bool Server::read_content_core(Stream &strm, bool last_connection,
                                      Request &req, Response &res,
                                      ContentReceiver receiver,
                                      MultipartContentHeader mulitpart_header,
                                      ContentReceiver multipart_receiver) {
  detail::MultipartFormDataParser multipart_form_data_parser;
  ContentReceiver out;

  if (req.is_multipart_form_data()) {
    const auto &content_type = req.get_header_value("Content-Type");
    std::string boundary;
    if (!detail::parse_multipart_boundary(content_type, boundary)) {
      res.status = 400;
      return write_response(strm, last_connection, req, res);
    }

    multipart_form_data_parser.set_boundary(boundary);
    out = [&](const char *buf, size_t n) {
      return multipart_form_data_parser.parse(buf, n, multipart_receiver,
                                              mulitpart_header);
    };
  } else {
    out = receiver;
  }

  if (!detail::read_content(strm, req, payload_max_length_, res.status,
                            Progress(), out)) {
    return write_response(strm, last_connection, req, res);
  }

  if (req.is_multipart_form_data()) {
    if (!multipart_form_data_parser.is_valid()) {
      res.status = 400;
      return write_response(strm, last_connection, req, res);
    }
  }

  return true;
}

inline bool Server::handle_file_request(Request &req, Response &res,
                                        bool head) {
  for (const auto &kv : base_dirs_) {
    const auto &mount_point = kv.first;
    const auto &base_dir = kv.second;

    // Prefix match
    if (!req.path.find(mount_point)) {
      std::string sub_path = "/" + req.path.substr(mount_point.size());
      if (detail::is_valid_path(sub_path)) {
        auto path = base_dir + sub_path;
        if (path.back() == '/') { path += "index.html"; }

        if (detail::is_file(path)) {
          detail::read_file(path, res.body);
          auto type =
              detail::find_content_type(path, file_extension_and_mimetype_map_);
          if (type) { res.set_header("Content-Type", type); }
          res.status = 200;
          if (!head && file_request_handler_) {
            file_request_handler_(req, res);
          }
          return true;
        }
      }
    }
  }
  return false;
}

inline socket_t Server::create_server_socket(const char *host, int port,
                                             int socket_flags) const {
  return detail::create_socket(
      host, port,
      [](socket_t sock, struct addrinfo &ai) -> bool {
        if (::bind(sock, ai.ai_addr, static_cast<int>(ai.ai_addrlen))) {
          return false;
        }
        if (::listen(sock, 5)) { // Listen through 5 channels
          return false;
        }
        return true;
      },
      socket_flags);
}

inline int Server::bind_internal(const char *host, int port, int socket_flags) {
  if (!is_valid()) { return -1; }

  svr_sock_ = create_server_socket(host, port, socket_flags);
  if (svr_sock_ == INVALID_SOCKET) { return -1; }

  if (port == 0) {
    struct sockaddr_storage address;
    socklen_t len = sizeof(address);
    if (getsockname(svr_sock_, reinterpret_cast<struct sockaddr *>(&address),
                    &len) == -1) {
      return -1;
    }
    if (address.ss_family == AF_INET) {
      return ntohs(reinterpret_cast<struct sockaddr_in *>(&address)->sin_port);
    } else if (address.ss_family == AF_INET6) {
      return ntohs(
          reinterpret_cast<struct sockaddr_in6 *>(&address)->sin6_port);
    } else {
      return -1;
    }
  } else {
    return port;
  }
}

inline bool Server::listen_internal() {
  auto ret = true;
  is_running_ = true;

  {
    std::unique_ptr<TaskQueue> task_queue(new_task_queue());

    for (;;) {
      if (svr_sock_ == INVALID_SOCKET) {
        // The server socket was closed by 'stop' method.
        break;
      }

      auto val = detail::select_read(svr_sock_, 0, 100000);

      if (val == 0) { // Timeout
        continue;
      }

      socket_t sock = accept(svr_sock_, nullptr, nullptr);

      if (sock == INVALID_SOCKET) {
        if (errno == EMFILE) {
          // The per-process limit of open file descriptors has been reached.
          // Try to accept new connections after a short sleep.
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          continue;
        }
        if (svr_sock_ != INVALID_SOCKET) {
          detail::close_socket(svr_sock_);
          ret = false;
        } else {
          ; // The server socket was closed by user.
        }
        break;
      }

      task_queue->enqueue([=]() { process_and_close_socket(sock); });
    }

    task_queue->shutdown();
  }

  is_running_ = false;
  return ret;
}

inline bool Server::routing(Request &req, Response &res, Stream &strm,
                            bool last_connection) {
  // File handler
  bool is_head_request = req.method == "HEAD";
  if ((req.method == "GET" || is_head_request) &&
      handle_file_request(req, res, is_head_request)) {
    return true;
  }

  if (detail::expect_content(req)) {
    // Content reader handler
    {
      ContentReader reader(
          [&](ContentReceiver receiver) {
            return read_content_with_content_receiver(
                strm, last_connection, req, res, receiver, nullptr, nullptr);
          },
          [&](MultipartContentHeader header, ContentReceiver receiver) {
            return read_content_with_content_receiver(
                strm, last_connection, req, res, nullptr, header, receiver);
          });

      if (req.method == "POST") {
        if (dispatch_request_for_content_reader(
                req, res, reader, post_handlers_for_content_reader_)) {
          return true;
        }
      } else if (req.method == "PUT") {
        if (dispatch_request_for_content_reader(
                req, res, reader, put_handlers_for_content_reader_)) {
          return true;
        }
      } else if (req.method == "PATCH") {
        if (dispatch_request_for_content_reader(
                req, res, reader, patch_handlers_for_content_reader_)) {
          return true;
        }
      }
    }

    // Read content into `req.body`
    if (!read_content(strm, last_connection, req, res)) { return false; }
  }

  // Regular handler
  if (req.method == "GET" || req.method == "HEAD") {
    return dispatch_request(req, res, get_handlers_);
  } else if (req.method == "POST") {
    return dispatch_request(req, res, post_handlers_);
  } else if (req.method == "PUT") {
    return dispatch_request(req, res, put_handlers_);
  } else if (req.method == "DELETE") {
    return dispatch_request(req, res, delete_handlers_);
  } else if (req.method == "OPTIONS") {
    return dispatch_request(req, res, options_handlers_);
  } else if (req.method == "PATCH") {
    return dispatch_request(req, res, patch_handlers_);
  }

  res.status = 400;
  return false;
}

inline bool Server::dispatch_request(Request &req, Response &res,
                                     Handlers &handlers) {
  for (const auto &x : handlers) {
    const auto &pattern = x.first;
    const auto &handler = x.second;

    if (std::regex_match(req.path, req.matches, pattern)) {
      handler(req, res);
      return true;
    }
  }
  return false;
}

inline bool Server::dispatch_request_for_content_reader(
    Request &req, Response &res, ContentReader content_reader,
    HandlersForContentReader &handlers) {
  for (const auto &x : handlers) {
    const auto &pattern = x.first;
    const auto &handler = x.second;

    if (std::regex_match(req.path, req.matches, pattern)) {
      handler(req, res, content_reader);
      return true;
    }
  }
  return false;
}

inline bool
Server::process_request(Stream &strm, bool last_connection,
                        bool &connection_close,
                        const std::function<void(Request &)> &setup_request) {
  std::array<char, 2048> buf{};

  detail::stream_line_reader line_reader(strm, buf.data(), buf.size());

  // Connection has been closed on client
  if (!line_reader.getline()) { return false; }

  Request req;
  Response res;

  res.version = "HTTP/1.1";

  // Check if the request URI doesn't exceed the limit
  if (line_reader.size() > CPPHTTPLIB_REQUEST_URI_MAX_LENGTH) {
    Headers dummy;
    detail::read_headers(strm, dummy);
    res.status = 414;
    return write_response(strm, last_connection, req, res);
  }

  // Request line and headers
  if (!parse_request_line(line_reader.ptr(), req) ||
      !detail::read_headers(strm, req.headers)) {
    res.status = 400;
    return write_response(strm, last_connection, req, res);
  }

  if (req.get_header_value("Connection") == "close") {
    connection_close = true;
  }

  if (req.version == "HTTP/1.0" &&
      req.get_header_value("Connection") != "Keep-Alive") {
    connection_close = true;
  }

  req.set_header("REMOTE_ADDR", strm.get_remote_addr());

  if (req.has_header("Range")) {
    const auto &range_header_value = req.get_header_value("Range");
    if (!detail::parse_range_header(range_header_value, req.ranges)) {
      // TODO: error
    }
  }

  if (setup_request) { setup_request(req); }

  if (req.get_header_value("Expect") == "100-continue") {
    auto status = 100;
    if (expect_100_continue_handler_) {
      status = expect_100_continue_handler_(req, res);
    }
    switch (status) {
    case 100:
    case 417:
      strm.write_format("HTTP/1.1 %d %s\r\n\r\n", status,
                        detail::status_message(status));
      break;
    default: return write_response(strm, last_connection, req, res);
    }
  }

  // Rounting
  if (routing(req, res, strm, last_connection)) {
    if (res.status == -1) { res.status = req.ranges.empty() ? 200 : 206; }
  } else {
    if (res.status == -1) { res.status = 404; }
  }

  return write_response(strm, last_connection, req, res);
}

inline bool Server::is_valid() const { return true; }

inline bool Server::process_and_close_socket(socket_t sock) {
  return detail::process_and_close_socket(
      false, sock, keep_alive_max_count_, read_timeout_sec_, read_timeout_usec_,
      [this](Stream &strm, bool last_connection, bool &connection_close) {
        return process_request(strm, last_connection, connection_close,
                               nullptr);
      });
}

// HTTP client implementation
inline Client::Client(const std::string &host, int port,
                      const std::string &client_cert_path,
                      const std::string &client_key_path)
    : host_(host), port_(port),
      host_and_port_(host_ + ":" + std::to_string(port_)),
      client_cert_path_(client_cert_path), client_key_path_(client_key_path) {}

inline Client::~Client() {}

inline bool Client::is_valid() const { return true; }

inline socket_t Client::create_client_socket() const {
  if (!proxy_host_.empty()) {
    return detail::create_client_socket(proxy_host_.c_str(), proxy_port_,
                                        timeout_sec_, interface_);
  }
  return detail::create_client_socket(host_.c_str(), port_, timeout_sec_,
                                      interface_);
}

inline bool Client::read_response_line(Stream &strm, Response &res) {
  std::array<char, 2048> buf;

  detail::stream_line_reader line_reader(strm, buf.data(), buf.size());

  if (!line_reader.getline()) { return false; }

  const static std::regex re("(HTTP/1\\.[01]) (\\d+?) .*\r\n");

  std::cmatch m;
  if (std::regex_match(line_reader.ptr(), m, re)) {
    res.version = std::string(m[1]);
    res.status = std::stoi(std::string(m[2]));
  }

  return true;
}

inline bool Client::send(const Request &req, Response &res) {
  auto sock = create_client_socket();
  if (sock == INVALID_SOCKET) { return false; }

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  if (is_ssl() && !proxy_host_.empty()) {
    bool error;
    if (!connect(sock, res, error)) { return error; }
  }
#endif

  return process_and_close_socket(
      sock, 1, [&](Stream &strm, bool last_connection, bool &connection_close) {
        return handle_request(strm, req, res, last_connection,
                              connection_close);
      });
}

inline bool Client::send(const std::vector<Request> &requests,
                         std::vector<Response> &responses) {
  size_t i = 0;
  while (i < requests.size()) {
    auto sock = create_client_socket();
    if (sock == INVALID_SOCKET) { return false; }

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (is_ssl() && !proxy_host_.empty()) {
      Response res;
      bool error;
      if (!connect(sock, res, error)) { return false; }
    }
#endif

    if (!process_and_close_socket(sock, requests.size() - i,
                                  [&](Stream &strm, bool last_connection,
                                      bool &connection_close) -> bool {
                                    auto &req = requests[i++];
                                    auto res = Response();
                                    auto ret = handle_request(strm, req, res,
                                                              last_connection,
                                                              connection_close);
                                    if (ret) {
                                      responses.emplace_back(std::move(res));
                                    }
                                    return ret;
                                  })) {
      return false;
    }
  }

  return true;
}

inline bool Client::handle_request(Stream &strm, const Request &req,
                                   Response &res, bool last_connection,
                                   bool &connection_close) {
  if (req.path.empty()) { return false; }

  bool ret;

  if (!is_ssl() && !proxy_host_.empty()) {
    auto req2 = req;
    req2.path = "http://" + host_and_port_ + req.path;
    ret = process_request(strm, req2, res, last_connection, connection_close);
  } else {
    ret = process_request(strm, req, res, last_connection, connection_close);
  }

  if (!ret) { return false; }

  if (300 < res.status && res.status < 400 && follow_location_) {
    ret = redirect(req, res);
  }

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  if (res.status == 401 || res.status == 407) {
    auto is_proxy = res.status == 407;
    const auto &username =
        is_proxy ? proxy_digest_auth_username_ : digest_auth_username_;
    const auto &password =
        is_proxy ? proxy_digest_auth_password_ : digest_auth_password_;

    if (!username.empty() && !password.empty()) {
      std::map<std::string, std::string> auth;
      if (parse_www_authenticate(res, auth, is_proxy)) {
        Request new_req = req;
        auto key = is_proxy ? "Proxy-Authorization" : "WWW-Authorization";
        new_req.headers.erase(key);
        new_req.headers.insert(make_digest_authentication_header(
            req, auth, 1, random_string(10), username, password, is_proxy));

        Response new_res;

        ret = send(new_req, new_res);
        if (ret) { res = new_res; }
      }
    }
  }
#endif

  return ret;
}

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
inline bool Client::connect(socket_t sock, Response &res, bool &error) {
  error = true;
  Response res2;

  if (!detail::process_socket(
          true, sock, 1, read_timeout_sec_, read_timeout_usec_,
          [&](Stream &strm, bool /*last_connection*/, bool &connection_close) {
            Request req2;
            req2.method = "CONNECT";
            req2.path = host_and_port_;
            return process_request(strm, req2, res2, false, connection_close);
          })) {
    detail::close_socket(sock);
    error = false;
    return false;
  }

  if (res2.status == 407) {
    if (!proxy_digest_auth_username_.empty() &&
        !proxy_digest_auth_password_.empty()) {
      std::map<std::string, std::string> auth;
      if (parse_www_authenticate(res2, auth, true)) {
        Response res3;
        if (!detail::process_socket(
                true, sock, 1, read_timeout_sec_, read_timeout_usec_,
                [&](Stream &strm, bool /*last_connection*/,
                    bool &connection_close) {
                  Request req3;
                  req3.method = "CONNECT";
                  req3.path = host_and_port_;
                  req3.headers.insert(make_digest_authentication_header(
                      req3, auth, 1, random_string(10),
                      proxy_digest_auth_username_, proxy_digest_auth_password_,
                      true));
                  return process_request(strm, req3, res3, false,
                                         connection_close);
                })) {
          detail::close_socket(sock);
          error = false;
          return false;
        }
      }
    } else {
      res = res2;
      return false;
    }
  }

  return true;
}
#endif

inline bool Client::redirect(const Request &req, Response &res) {
  if (req.redirect_count == 0) { return false; }

  auto location = res.get_header_value("location");
  if (location.empty()) { return false; }

  const static std::regex re(
      R"(^(?:([^:/?#]+):)?(?://([^/?#]*))?([^?#]*(?:\?[^#]*)?)(?:#.*)?)");

  std::smatch m;
  if (!regex_match(location, m, re)) { return false; }

  auto scheme = is_ssl() ? "https" : "http";

  auto next_scheme = m[1].str();
  auto next_host = m[2].str();
  auto next_path = m[3].str();
  if (next_scheme.empty()) { next_scheme = scheme; }
  if (next_scheme.empty()) { next_scheme = scheme; }
  if (next_host.empty()) { next_host = host_; }
  if (next_path.empty()) { next_path = "/"; }

  if (next_scheme == scheme && next_host == host_) {
    return detail::redirect(*this, req, res, next_path);
  } else {
    if (next_scheme == "https") {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
      SSLClient cli(next_host.c_str());
      cli.copy_settings(*this);
      return detail::redirect(cli, req, res, next_path);
#else
      return false;
#endif
    } else {
      Client cli(next_host.c_str());
      cli.copy_settings(*this);
      return detail::redirect(cli, req, res, next_path);
    }
  }
}

inline bool Client::write_request(Stream &strm, const Request &req,
                                  bool last_connection) {
  detail::BufferStream bstrm;

  // Request line
  const auto &path = detail::encode_url(req.path);

  bstrm.write_format("%s %s HTTP/1.1\r\n", req.method.c_str(), path.c_str());

  // Additonal headers
  Headers headers;
  if (last_connection) { headers.emplace("Connection", "close"); }

  if (!req.has_header("Host")) {
    if (is_ssl()) {
      if (port_ == 443) {
        headers.emplace("Host", host_);
      } else {
        headers.emplace("Host", host_and_port_);
      }
    } else {
      if (port_ == 80) {
        headers.emplace("Host", host_);
      } else {
        headers.emplace("Host", host_and_port_);
      }
    }
  }

  if (!req.has_header("Accept")) { headers.emplace("Accept", "*/*"); }

  if (!req.has_header("User-Agent")) {
    headers.emplace("User-Agent", "cpp-httplib/0.5");
  }

  if (req.body.empty()) {
    if (req.content_provider) {
      auto length = std::to_string(req.content_length);
      headers.emplace("Content-Length", length);
    } else {
      headers.emplace("Content-Length", "0");
    }
  } else {
    if (!req.has_header("Content-Type")) {
      headers.emplace("Content-Type", "text/plain");
    }

    if (!req.has_header("Content-Length")) {
      auto length = std::to_string(req.body.size());
      headers.emplace("Content-Length", length);
    }
  }

  if (!basic_auth_username_.empty() && !basic_auth_password_.empty()) {
    headers.insert(make_basic_authentication_header(
        basic_auth_username_, basic_auth_password_, false));
  }

  if (!proxy_basic_auth_username_.empty() &&
      !proxy_basic_auth_password_.empty()) {
    headers.insert(make_basic_authentication_header(
        proxy_basic_auth_username_, proxy_basic_auth_password_, true));
  }

  detail::write_headers(bstrm, req, headers);

  // Flush buffer
  auto &data = bstrm.get_buffer();
  strm.write(data.data(), data.size());

  // Body
  if (req.body.empty()) {
    if (req.content_provider) {
      size_t offset = 0;
      size_t end_offset = req.content_length;

      DataSink data_sink;
      data_sink.write = [&](const char *d, size_t l) {
        auto written_length = strm.write(d, l);
        offset += written_length;
      };
      data_sink.is_writable = [&](void) { return strm.is_writable(); };

      while (offset < end_offset) {
        req.content_provider(offset, end_offset - offset, data_sink);
      }
    }
  } else {
    strm.write(req.body);
  }

  return true;
}

inline std::shared_ptr<Response> Client::send_with_content_provider(
    const char *method, const char *path, const Headers &headers,
    const std::string &body, size_t content_length,
    ContentProvider content_provider, const char *content_type) {
  Request req;
  req.method = method;
  req.headers = headers;
  req.path = path;

  req.headers.emplace("Content-Type", content_type);

#ifdef CPPHTTPLIB_ZLIB_SUPPORT
  if (compress_) {
    if (content_provider) {
      size_t offset = 0;

      DataSink data_sink;
      data_sink.write = [&](const char *data, size_t data_len) {
        req.body.append(data, data_len);
        offset += data_len;
      };
      data_sink.is_writable = [&](void) { return true; };

      while (offset < content_length) {
        content_provider(offset, content_length - offset, data_sink);
      }
    } else {
      req.body = body;
    }

    if (!detail::compress(req.body)) { return nullptr; }
    req.headers.emplace("Content-Encoding", "gzip");
  } else
#endif
  {
    if (content_provider) {
      req.content_length = content_length;
      req.content_provider = content_provider;
    } else {
      req.body = body;
    }
  }

  auto res = std::make_shared<Response>();

  return send(req, *res) ? res : nullptr;
}

inline bool Client::process_request(Stream &strm, const Request &req,
                                    Response &res, bool last_connection,
                                    bool &connection_close) {
  // Send request
  if (!write_request(strm, req, last_connection)) { return false; }

  // Receive response and headers
  if (!read_response_line(strm, res) ||
      !detail::read_headers(strm, res.headers)) {
    return false;
  }

  if (res.get_header_value("Connection") == "close" ||
      res.version == "HTTP/1.0") {
    connection_close = true;
  }

  if (req.response_handler) {
    if (!req.response_handler(res)) { return false; }
  }

  // Body
  if (req.method != "HEAD" && req.method != "CONNECT") {
    ContentReceiver out = [&](const char *buf, size_t n) {
      if (res.body.size() + n > res.body.max_size()) { return false; }
      res.body.append(buf, n);
      return true;
    };

    if (req.content_receiver) {
      out = [&](const char *buf, size_t n) {
        return req.content_receiver(buf, n);
      };
    }

    int dummy_status;
    if (!detail::read_content(strm, res, std::numeric_limits<size_t>::max(),
                              dummy_status, req.progress, out)) {
      return false;
    }
  }

  // Log
  if (logger_) { logger_(req, res); }

  return true;
}

inline bool Client::process_and_close_socket(
    socket_t sock, size_t request_count,
    std::function<bool(Stream &strm, bool last_connection,
                       bool &connection_close)>
        callback) {
  request_count = std::min(request_count, keep_alive_max_count_);
  return detail::process_and_close_socket(true, sock, request_count,
                                          read_timeout_sec_, read_timeout_usec_,
                                          callback);
}

inline bool Client::is_ssl() const { return false; }

inline std::shared_ptr<Response> Client::Get(const char *path) {
  return Get(path, Headers(), Progress());
}

inline std::shared_ptr<Response> Client::Get(const char *path,
                                             Progress progress) {
  return Get(path, Headers(), std::move(progress));
}

inline std::shared_ptr<Response> Client::Get(const char *path,
                                             const Headers &headers) {
  return Get(path, headers, Progress());
}

inline std::shared_ptr<Response>
Client::Get(const char *path, const Headers &headers, Progress progress) {
  Request req;
  req.method = "GET";
  req.path = path;
  req.headers = headers;
  req.progress = std::move(progress);

  auto res = std::make_shared<Response>();
  return send(req, *res) ? res : nullptr;
}

inline std::shared_ptr<Response> Client::Get(const char *path,
                                             ContentReceiver content_receiver) {
  return Get(path, Headers(), nullptr, std::move(content_receiver), Progress());
}

inline std::shared_ptr<Response> Client::Get(const char *path,
                                             ContentReceiver content_receiver,
                                             Progress progress) {
  return Get(path, Headers(), nullptr, std::move(content_receiver),
             std::move(progress));
}

inline std::shared_ptr<Response> Client::Get(const char *path,
                                             const Headers &headers,
                                             ContentReceiver content_receiver) {
  return Get(path, headers, nullptr, std::move(content_receiver), Progress());
}

inline std::shared_ptr<Response> Client::Get(const char *path,
                                             const Headers &headers,
                                             ContentReceiver content_receiver,
                                             Progress progress) {
  return Get(path, headers, nullptr, std::move(content_receiver),
             std::move(progress));
}

inline std::shared_ptr<Response> Client::Get(const char *path,
                                             const Headers &headers,
                                             ResponseHandler response_handler,
                                             ContentReceiver content_receiver) {
  return Get(path, headers, std::move(response_handler), content_receiver,
             Progress());
}

inline std::shared_ptr<Response> Client::Get(const char *path,
                                             const Headers &headers,
                                             ResponseHandler response_handler,
                                             ContentReceiver content_receiver,
                                             Progress progress) {
  Request req;
  req.method = "GET";
  req.path = path;
  req.headers = headers;
  req.response_handler = std::move(response_handler);
  req.content_receiver = std::move(content_receiver);
  req.progress = std::move(progress);

  auto res = std::make_shared<Response>();
  return send(req, *res) ? res : nullptr;
}

inline std::shared_ptr<Response> Client::Head(const char *path) {
  return Head(path, Headers());
}

inline std::shared_ptr<Response> Client::Head(const char *path,
                                              const Headers &headers) {
  Request req;
  req.method = "HEAD";
  req.headers = headers;
  req.path = path;

  auto res = std::make_shared<Response>();

  return send(req, *res) ? res : nullptr;
}

inline std::shared_ptr<Response> Client::Post(const char *path,
                                              const std::string &body,
                                              const char *content_type) {
  return Post(path, Headers(), body, content_type);
}

inline std::shared_ptr<Response> Client::Post(const char *path,
                                              const Headers &headers,
                                              const std::string &body,
                                              const char *content_type) {
  return send_with_content_provider("POST", path, headers, body, 0, nullptr,
                                    content_type);
}

inline std::shared_ptr<Response> Client::Post(const char *path,
                                              const Params &params) {
  return Post(path, Headers(), params);
}

inline std::shared_ptr<Response> Client::Post(const char *path,
                                              size_t content_length,
                                              ContentProvider content_provider,
                                              const char *content_type) {
  return Post(path, Headers(), content_length, content_provider, content_type);
}

inline std::shared_ptr<Response>
Client::Post(const char *path, const Headers &headers, size_t content_length,
             ContentProvider content_provider, const char *content_type) {
  return send_with_content_provider("POST", path, headers, std::string(),
                                    content_length, content_provider,
                                    content_type);
}

inline std::shared_ptr<Response>
Client::Post(const char *path, const Headers &headers, const Params &params) {
  std::string query;
  for (auto it = params.begin(); it != params.end(); ++it) {
    if (it != params.begin()) { query += "&"; }
    query += it->first;
    query += "=";
    query += detail::encode_url(it->second);
  }

  return Post(path, headers, query, "application/x-www-form-urlencoded");
}

inline std::shared_ptr<Response>
Client::Post(const char *path, const MultipartFormDataItems &items) {
  return Post(path, Headers(), items);
}

inline std::shared_ptr<Response>
Client::Post(const char *path, const Headers &headers,
             const MultipartFormDataItems &items) {
  auto boundary = detail::make_multipart_data_boundary();

  std::string body;

  for (const auto &item : items) {
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"" + item.name + "\"";
    if (!item.filename.empty()) {
      body += "; filename=\"" + item.filename + "\"";
    }
    body += "\r\n";
    if (!item.content_type.empty()) {
      body += "Content-Type: " + item.content_type + "\r\n";
    }
    body += "\r\n";
    body += item.content + "\r\n";
  }

  body += "--" + boundary + "--\r\n";

  std::string content_type = "multipart/form-data; boundary=" + boundary;
  return Post(path, headers, body, content_type.c_str());
}

inline std::shared_ptr<Response> Client::Put(const char *path,
                                             const std::string &body,
                                             const char *content_type) {
  return Put(path, Headers(), body, content_type);
}

inline std::shared_ptr<Response> Client::Put(const char *path,
                                             const Headers &headers,
                                             const std::string &body,
                                             const char *content_type) {
  return send_with_content_provider("PUT", path, headers, body, 0, nullptr,
                                    content_type);
}

inline std::shared_ptr<Response> Client::Put(const char *path,
                                             size_t content_length,
                                             ContentProvider content_provider,
                                             const char *content_type) {
  return Put(path, Headers(), content_length, content_provider, content_type);
}

inline std::shared_ptr<Response>
Client::Put(const char *path, const Headers &headers, size_t content_length,
            ContentProvider content_provider, const char *content_type) {
  return send_with_content_provider("PUT", path, headers, std::string(),
                                    content_length, content_provider,
                                    content_type);
}

inline std::shared_ptr<Response> Client::Put(const char *path,
                                             const Params &params) {
  return Put(path, Headers(), params);
}

inline std::shared_ptr<Response>
Client::Put(const char *path, const Headers &headers, const Params &params) {
  std::string query;
  for (auto it = params.begin(); it != params.end(); ++it) {
    if (it != params.begin()) { query += "&"; }
    query += it->first;
    query += "=";
    query += detail::encode_url(it->second);
  }

  return Put(path, headers, query, "application/x-www-form-urlencoded");
}

inline std::shared_ptr<Response> Client::Patch(const char *path,
                                               const std::string &body,
                                               const char *content_type) {
  return Patch(path, Headers(), body, content_type);
}

inline std::shared_ptr<Response> Client::Patch(const char *path,
                                               const Headers &headers,
                                               const std::string &body,
                                               const char *content_type) {
  return send_with_content_provider("PATCH", path, headers, body, 0, nullptr,
                                    content_type);
}

inline std::shared_ptr<Response> Client::Patch(const char *path,
                                               size_t content_length,
                                               ContentProvider content_provider,
                                               const char *content_type) {
  return Patch(path, Headers(), content_length, content_provider, content_type);
}

inline std::shared_ptr<Response>
Client::Patch(const char *path, const Headers &headers, size_t content_length,
              ContentProvider content_provider, const char *content_type) {
  return send_with_content_provider("PATCH", path, headers, std::string(),
                                    content_length, content_provider,
                                    content_type);
}

inline std::shared_ptr<Response> Client::Delete(const char *path) {
  return Delete(path, Headers(), std::string(), nullptr);
}

inline std::shared_ptr<Response> Client::Delete(const char *path,
                                                const std::string &body,
                                                const char *content_type) {
  return Delete(path, Headers(), body, content_type);
}

inline std::shared_ptr<Response> Client::Delete(const char *path,
                                                const Headers &headers) {
  return Delete(path, headers, std::string(), nullptr);
}

inline std::shared_ptr<Response> Client::Delete(const char *path,
                                                const Headers &headers,
                                                const std::string &body,
                                                const char *content_type) {
  Request req;
  req.method = "DELETE";
  req.headers = headers;
  req.path = path;

  if (content_type) { req.headers.emplace("Content-Type", content_type); }
  req.body = body;

  auto res = std::make_shared<Response>();

  return send(req, *res) ? res : nullptr;
}

inline std::shared_ptr<Response> Client::Options(const char *path) {
  return Options(path, Headers());
}

inline std::shared_ptr<Response> Client::Options(const char *path,
                                                 const Headers &headers) {
  Request req;
  req.method = "OPTIONS";
  req.path = path;
  req.headers = headers;

  auto res = std::make_shared<Response>();

  return send(req, *res) ? res : nullptr;
}

inline void Client::set_timeout_sec(time_t timeout_sec) {
  timeout_sec_ = timeout_sec;
}

inline void Client::set_read_timeout(time_t sec, time_t usec) {
  read_timeout_sec_ = sec;
  read_timeout_usec_ = usec;
}

inline void Client::set_keep_alive_max_count(size_t count) {
  keep_alive_max_count_ = count;
}

inline void Client::set_basic_auth(const char *username, const char *password) {
  basic_auth_username_ = username;
  basic_auth_password_ = password;
}

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
inline void Client::set_digest_auth(const char *username,
                                    const char *password) {
  digest_auth_username_ = username;
  digest_auth_password_ = password;
}
#endif

inline void Client::set_follow_location(bool on) { follow_location_ = on; }

inline void Client::set_compress(bool on) { compress_ = on; }

inline void Client::set_interface(const char *intf) { interface_ = intf; }

inline void Client::set_proxy(const char *host, int port) {
  proxy_host_ = host;
  proxy_port_ = port;
}

inline void Client::set_proxy_basic_auth(const char *username,
                                         const char *password) {
  proxy_basic_auth_username_ = username;
  proxy_basic_auth_password_ = password;
}

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
inline void Client::set_proxy_digest_auth(const char *username,
                                          const char *password) {
  proxy_digest_auth_username_ = username;
  proxy_digest_auth_password_ = password;
}
#endif

inline void Client::set_logger(Logger logger) { logger_ = std::move(logger); }

/*
 * SSL Implementation
 */
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
namespace detail {

template <typename U, typename V, typename T>
inline bool process_and_close_socket_ssl(
    bool is_client_request, socket_t sock, size_t keep_alive_max_count,
    time_t read_timeout_sec, time_t read_timeout_usec, SSL_CTX *ctx,
    std::mutex &ctx_mutex, U SSL_connect_or_accept, V setup, T callback) {
  assert(keep_alive_max_count > 0);

  SSL *ssl = nullptr;
  {
    std::lock_guard<std::mutex> guard(ctx_mutex);
    ssl = SSL_new(ctx);
  }

  if (!ssl) {
    close_socket(sock);
    return false;
  }

  auto bio = BIO_new_socket(static_cast<int>(sock), BIO_NOCLOSE);
  SSL_set_bio(ssl, bio, bio);

  if (!setup(ssl)) {
    SSL_shutdown(ssl);
    {
      std::lock_guard<std::mutex> guard(ctx_mutex);
      SSL_free(ssl);
    }

    close_socket(sock);
    return false;
  }

  auto ret = false;

  if (SSL_connect_or_accept(ssl) == 1) {
    if (keep_alive_max_count > 1) {
      auto count = keep_alive_max_count;
      while (count > 0 &&
             (is_client_request ||
              detail::select_read(sock, CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND,
                                  CPPHTTPLIB_KEEPALIVE_TIMEOUT_USECOND) > 0)) {
        SSLSocketStream strm(sock, ssl, read_timeout_sec, read_timeout_usec);
        auto last_connection = count == 1;
        auto connection_close = false;

        ret = callback(ssl, strm, last_connection, connection_close);
        if (!ret || connection_close) { break; }

        count--;
      }
    } else {
      SSLSocketStream strm(sock, ssl, read_timeout_sec, read_timeout_usec);
      auto dummy_connection_close = false;
      ret = callback(ssl, strm, true, dummy_connection_close);
    }
  }

  SSL_shutdown(ssl);
  {
    std::lock_guard<std::mutex> guard(ctx_mutex);
    SSL_free(ssl);
  }

  close_socket(sock);

  return ret;
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static std::shared_ptr<std::vector<std::mutex>> openSSL_locks_;

class SSLThreadLocks {
public:
  SSLThreadLocks() {
    openSSL_locks_ =
        std::make_shared<std::vector<std::mutex>>(CRYPTO_num_locks());
    CRYPTO_set_locking_callback(locking_callback);
  }

  ~SSLThreadLocks() { CRYPTO_set_locking_callback(nullptr); }

private:
  static void locking_callback(int mode, int type, const char * /*file*/,
                               int /*line*/) {
    auto &locks = *openSSL_locks_;
    if (mode & CRYPTO_LOCK) {
      locks[type].lock();
    } else {
      locks[type].unlock();
    }
  }
};

#endif

class SSLInit {
public:
  SSLInit() {
#if OPENSSL_VERSION_NUMBER < 0x1010001fL
    SSL_load_error_strings();
    SSL_library_init();
#else
    OPENSSL_init_ssl(
        OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
#endif
  }

  ~SSLInit() {
#if OPENSSL_VERSION_NUMBER < 0x1010001fL
    ERR_free_strings();
#endif
  }

private:
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  SSLThreadLocks thread_init_;
#endif
};

// SSL socket stream implementation
inline SSLSocketStream::SSLSocketStream(socket_t sock, SSL *ssl,
                                        time_t read_timeout_sec,
                                        time_t read_timeout_usec)
    : sock_(sock), ssl_(ssl), read_timeout_sec_(read_timeout_sec),
      read_timeout_usec_(read_timeout_usec) {}

inline SSLSocketStream::~SSLSocketStream() {}

inline bool SSLSocketStream::is_readable() const {
  return detail::select_read(sock_, read_timeout_sec_, read_timeout_usec_) > 0;
}

inline bool SSLSocketStream::is_writable() const {
  return detail::select_write(sock_, 0, 0) > 0;
}

inline int SSLSocketStream::read(char *ptr, size_t size) {
  if (SSL_pending(ssl_) > 0 ||
      select_read(sock_, read_timeout_sec_, read_timeout_usec_) > 0) {
    return SSL_read(ssl_, ptr, static_cast<int>(size));
  }
  return -1;
}

inline int SSLSocketStream::write(const char *ptr, size_t size) {
  if (is_writable()) { return SSL_write(ssl_, ptr, static_cast<int>(size)); }
  return -1;
}

inline std::string SSLSocketStream::get_remote_addr() const {
  return detail::get_remote_addr(sock_);
}

static SSLInit sslinit_;

} // namespace detail

// SSL HTTP server implementation
inline SSLServer::SSLServer(const char *cert_path, const char *private_key_path,
                            const char *client_ca_cert_file_path,
                            const char *client_ca_cert_dir_path) {
  ctx_ = SSL_CTX_new(SSLv23_server_method());

  if (ctx_) {
    SSL_CTX_set_options(ctx_,
                        SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                            SSL_OP_NO_COMPRESSION |
                            SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

    // auto ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    // SSL_CTX_set_tmp_ecdh(ctx_, ecdh);
    // EC_KEY_free(ecdh);

    if (SSL_CTX_use_certificate_chain_file(ctx_, cert_path) != 1 ||
        SSL_CTX_use_PrivateKey_file(ctx_, private_key_path, SSL_FILETYPE_PEM) !=
            1) {
      SSL_CTX_free(ctx_);
      ctx_ = nullptr;
    } else if (client_ca_cert_file_path || client_ca_cert_dir_path) {
      // if (client_ca_cert_file_path) {
      //   auto list = SSL_load_client_CA_file(client_ca_cert_file_path);
      //   SSL_CTX_set_client_CA_list(ctx_, list);
      // }

      SSL_CTX_load_verify_locations(ctx_, client_ca_cert_file_path,
                                    client_ca_cert_dir_path);

      SSL_CTX_set_verify(
          ctx_,
          SSL_VERIFY_PEER |
              SSL_VERIFY_FAIL_IF_NO_PEER_CERT, // SSL_VERIFY_CLIENT_ONCE,
          nullptr);
    }
  }
}

inline SSLServer::~SSLServer() {
  if (ctx_) { SSL_CTX_free(ctx_); }
}

inline bool SSLServer::is_valid() const { return ctx_; }

inline bool SSLServer::process_and_close_socket(socket_t sock) {
  return detail::process_and_close_socket_ssl(
      false, sock, keep_alive_max_count_, read_timeout_sec_, read_timeout_usec_,
      ctx_, ctx_mutex_, SSL_accept, [](SSL * /*ssl*/) { return true; },
      [this](SSL *ssl, Stream &strm, bool last_connection,
             bool &connection_close) {
        return process_request(strm, last_connection, connection_close,
                               [&](Request &req) { req.ssl = ssl; });
      });
}

// SSL HTTP client implementation
inline SSLClient::SSLClient(const std::string &host, int port,
                            const std::string &client_cert_path,
                            const std::string &client_key_path)
    : Client(host, port, client_cert_path, client_key_path) {
  ctx_ = SSL_CTX_new(SSLv23_client_method());

  detail::split(&host_[0], &host_[host_.size()], '.',
                [&](const char *b, const char *e) {
                  host_components_.emplace_back(std::string(b, e));
                });
  if (!client_cert_path.empty() && !client_key_path.empty()) {
    if (SSL_CTX_use_certificate_file(ctx_, client_cert_path.c_str(),
                                     SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_use_PrivateKey_file(ctx_, client_key_path.c_str(),
                                    SSL_FILETYPE_PEM) != 1) {
      SSL_CTX_free(ctx_);
      ctx_ = nullptr;
    }
  }
}

inline SSLClient::~SSLClient() {
  if (ctx_) { SSL_CTX_free(ctx_); }
}

inline bool SSLClient::is_valid() const { return ctx_; }

inline void SSLClient::set_ca_cert_path(const char *ca_cert_file_path,
                                        const char *ca_cert_dir_path) {
  if (ca_cert_file_path) { ca_cert_file_path_ = ca_cert_file_path; }
  if (ca_cert_dir_path) { ca_cert_dir_path_ = ca_cert_dir_path; }
}

inline void SSLClient::enable_server_certificate_verification(bool enabled) {
  server_certificate_verification_ = enabled;
}

inline long SSLClient::get_openssl_verify_result() const {
  return verify_result_;
}

inline SSL_CTX *SSLClient::ssl_context() const noexcept { return ctx_; }

inline bool SSLClient::process_and_close_socket(
    socket_t sock, size_t request_count,
    std::function<bool(Stream &strm, bool last_connection,
                       bool &connection_close)>
        callback) {

  request_count = std::min(request_count, keep_alive_max_count_);

  return is_valid() &&
         detail::process_and_close_socket_ssl(
             true, sock, request_count, read_timeout_sec_, read_timeout_usec_,
             ctx_, ctx_mutex_,
             [&](SSL *ssl) {
               if (ca_cert_file_path_.empty()) {
                 SSL_CTX_set_verify(ctx_, SSL_VERIFY_NONE, nullptr);
               } else {
                 if (!SSL_CTX_load_verify_locations(
                         ctx_, ca_cert_file_path_.c_str(), nullptr)) {
                   return false;
                 }
                 SSL_CTX_set_verify(ctx_, SSL_VERIFY_PEER, nullptr);
               }

               if (SSL_connect(ssl) != 1) { return false; }

               if (server_certificate_verification_) {
                 verify_result_ = SSL_get_verify_result(ssl);

                 if (verify_result_ != X509_V_OK) { return false; }

                 auto server_cert = SSL_get_peer_certificate(ssl);

                 if (server_cert == nullptr) { return false; }

                 if (!verify_host(server_cert)) {
                   X509_free(server_cert);
                   return false;
                 }
                 X509_free(server_cert);
               }

               return true;
             },
             [&](SSL *ssl) {
               SSL_set_tlsext_host_name(ssl, host_.c_str());
               return true;
             },
             [&](SSL * /*ssl*/, Stream &strm, bool last_connection,
                 bool &connection_close) {
               return callback(strm, last_connection, connection_close);
             });
}

inline bool SSLClient::is_ssl() const { return true; }

inline bool SSLClient::verify_host(X509 *server_cert) const {
  /* Quote from RFC2818 section 3.1 "Server Identity"

     If a subjectAltName extension of type dNSName is present, that MUST
     be used as the identity. Otherwise, the (most specific) Common Name
     field in the Subject field of the certificate MUST be used. Although
     the use of the Common Name is existing practice, it is deprecated and
     Certification Authorities are encouraged to use the dNSName instead.

     Matching is performed using the matching rules specified by
     [RFC2459].  If more than one identity of a given type is present in
     the certificate (e.g., more than one dNSName name, a match in any one
     of the set is considered acceptable.) Names may contain the wildcard
     character * which is considered to match any single domain name
     component or component fragment. E.g., *.a.com matches foo.a.com but
     not bar.foo.a.com. f*.com matches foo.com but not bar.com.

     In some cases, the URI is specified as an IP address rather than a
     hostname. In this case, the iPAddress subjectAltName must be present
     in the certificate and must exactly match the IP in the URI.

  */
  return verify_host_with_subject_alt_name(server_cert) ||
         verify_host_with_common_name(server_cert);
}

inline bool
SSLClient::verify_host_with_subject_alt_name(X509 *server_cert) const {
  auto ret = false;

  auto type = GEN_DNS;

  struct in6_addr addr6;
  struct in_addr addr;
  size_t addr_len = 0;

#ifndef __MINGW32__
  if (inet_pton(AF_INET6, host_.c_str(), &addr6)) {
    type = GEN_IPADD;
    addr_len = sizeof(struct in6_addr);
  } else if (inet_pton(AF_INET, host_.c_str(), &addr)) {
    type = GEN_IPADD;
    addr_len = sizeof(struct in_addr);
  }
#endif

  auto alt_names = static_cast<const struct stack_st_GENERAL_NAME *>(
      X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));

  if (alt_names) {
    auto dsn_matched = false;
    auto ip_mached = false;

    auto count = sk_GENERAL_NAME_num(alt_names);

    for (auto i = 0; i < count && !dsn_matched; i++) {
      auto val = sk_GENERAL_NAME_value(alt_names, i);
      if (val->type == type) {
        auto name = (const char *)ASN1_STRING_get0_data(val->d.ia5);
        auto name_len = (size_t)ASN1_STRING_length(val->d.ia5);

        if (strlen(name) == name_len) {
          switch (type) {
          case GEN_DNS: dsn_matched = check_host_name(name, name_len); break;

          case GEN_IPADD:
            if (!memcmp(&addr6, name, addr_len) ||
                !memcmp(&addr, name, addr_len)) {
              ip_mached = true;
            }
            break;
          }
        }
      }
    }

    if (dsn_matched || ip_mached) { ret = true; }
  }

  GENERAL_NAMES_free((STACK_OF(GENERAL_NAME) *)alt_names);

  return ret;
}

inline bool SSLClient::verify_host_with_common_name(X509 *server_cert) const {
  const auto subject_name = X509_get_subject_name(server_cert);

  if (subject_name != nullptr) {
    char name[BUFSIZ];
    auto name_len = X509_NAME_get_text_by_NID(subject_name, NID_commonName,
                                              name, sizeof(name));

    if (name_len != -1) { return check_host_name(name, name_len); }
  }

  return false;
}

inline bool SSLClient::check_host_name(const char *pattern,
                                       size_t pattern_len) const {
  if (host_.size() == pattern_len && host_ == pattern) { return true; }

  // Wildcard match
  // https://bugs.launchpad.net/ubuntu/+source/firefox-3.0/+bug/376484
  std::vector<std::string> pattern_components;
  detail::split(&pattern[0], &pattern[pattern_len], '.',
                [&](const char *b, const char *e) {
                  pattern_components.emplace_back(std::string(b, e));
                });

  if (host_components_.size() != pattern_components.size()) { return false; }

  auto itr = pattern_components.begin();
  for (const auto &h : host_components_) {
    auto &p = *itr;
    if (p != h && p != "*") {
      auto partial_match = (p.size() > 0 && p[p.size() - 1] == '*' &&
                            !p.compare(0, p.size() - 1, h));
      if (!partial_match) { return false; }
    }
    ++itr;
  }

  return true;
}
#endif

// ----------------------------------------------------------------------------

} // namespace httplib

#endif // CPPHTTPLIB_HTTPLIB_H
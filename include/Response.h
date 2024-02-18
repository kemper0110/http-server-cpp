//
// Created by danil on 07.01.24.
//

#ifndef HTTP_SERVER_CPP_RESPONSE_H
#define HTTP_SERVER_CPP_RESPONSE_H

#include <variant>
#include <string>


struct EmptyBodyResponse {
    int status_code;
};

struct StringResponse {
    int status_code;
    std::string string;
    std::string content_type;
};

struct JsonResponse {
    inline static const char* content_type = "Content-Type: application/json";
    int status_code;
    std::string json;
};

struct FileResponse {
    std::string filename;
};

struct SseResponse {
    inline static const char* cache_control = "Cache-Control: no-store";
    inline static const char* content_type = "Content-Type: text/event-stream";
    /*
     * "id: 1\n" - opt
     * "event: aboba\n" - opt
     * "retry: 1000\n" - ms - opt
     * "data: data-aboba\n" - req
     * ": test stream message\n" - ping or keep-alive mechanism - opt
     * "\n" - message end
     */
};

struct ChunkedResponse {
    inline static const char* transfer_encoding = "Transfer-Encoding: chunked";
    inline static const char* content_type = "Content-Type: text/plain";
    /*
     * HTTP/1.1 200 OK
     * Content-Type: text/plain
     * Transfer-Encoding: chunked
     *
     * 7\r\n - size
     * Mozilla\r\n - message
     * 11\r\n
     * Developer Network\r\n
     * 0\r\n - end size
     * \r\n - end message
     */
};

using Response = std::variant<
        EmptyBodyResponse,
        StringResponse,
        JsonResponse,
        std::monostate
>;

#endif //HTTP_SERVER_CPP_RESPONSE_H

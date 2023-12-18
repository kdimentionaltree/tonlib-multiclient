#pragma once
#include "auto/tl/tonlib_api.h"


namespace tonlib_api = ton::tonlib_api;

class MultiClient {
public:
    MultiClient();
    ~MultiClient();

    MultiClient(MultiClient&& other);
    MultiClient& operator=(MultiClient&& other);

    struct RequestOptions {
        std::int32_t ls_index = -1;
        std::int32_t archival = -1;
    };

    struct Request {
        std::uint64_t id;
        MultiClient::RequestOptions options;
        tonlib_api::object_ptr<tonlib_api::Function> function;
    };

    struct ResponseMeta {
        std::int32_t ls_index;
    };

    struct Response {
        std::uint64_t id;
        MultiClient::ResponseMeta meta;
        tonlib_api::object_ptr<tonlib_api::Object> object;
    };

    void send(Request&& request);
    Response receive(double timeout);
    static Response execute(Request&& request);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

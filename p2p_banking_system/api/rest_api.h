#ifndef REST_API_H
#define REST_API_H

#include <string>
#include <map>
#include <functional>
#include <mutex>
#include <vector>
#include <sstream>

class RESTAPI
{
    using Handler = std::function<std::string(const std::map<std::string, std::string> &, const std::string &body)>;

    std::map<std::string, Handler> getHandlers;
    std::map<std::string, Handler> postHandlers;
    std::map<std::string, Handler> putHandlers;
    std::map<std::string, Handler> deleteHandlers;
    std::mutex apiMutex;
    std::string apiPrefix;

public:
    explicit RESTAPI(const std::string &prefix = "/api/v1");

    void onGet(const std::string &path, Handler handler);
    void onPost(const std::string &path, Handler handler);
    void onPut(const std::string &path, Handler handler);
    void onDelete(const std::string &path, Handler handler);

    std::string handleGet(const std::string &path, const std::map<std::string, std::string> &params);
    std::string handlePost(const std::string &path, const std::map<std::string, std::string> &params, const std::string &body);
    std::string handlePut(const std::string &path, const std::map<std::string, std::string> &params, const std::string &body);
    std::string handleDelete(const std::string &path, const std::map<std::string, std::string> &params);

    std::string getApiPrefix() const;
    void setApiPrefix(const std::string &prefix);
    void removeHandler(const std::string &method, const std::string &path);
    bool hasHandler(const std::string &method, const std::string &path) const;
    std::vector<std::string> listEndpoints() const;
};

class APIResponse
{
public:
    int statusCode;
    std::string body;
    std::map<std::string, std::string> headers;

    APIResponse();
    APIResponse(int code, const std::string &data);

    std::string serialize() const;
    static APIResponse ok(const std::string &data);
    static APIResponse error(const std::string &message, int code = 400);
    static APIResponse notFound();
    static APIResponse unauthorized();
    static APIResponse forbidden();
    static APIResponse created(const std::string &location);
};

#endif

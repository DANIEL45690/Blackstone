#include "rest_api.h"
#include "../core/sha1.h"

RESTAPI::RESTAPI(const std::string &prefix) : apiPrefix(prefix) {}

void RESTAPI::onGet(const std::string &path, Handler handler)
{
    std::lock_guard<std::mutex> lock(apiMutex);
    getHandlers[apiPrefix + path] = handler;
}

void RESTAPI::onPost(const std::string &path, Handler handler)
{
    std::lock_guard<std::mutex> lock(apiMutex);
    postHandlers[apiPrefix + path] = handler;
}

void RESTAPI::onPut(const std::string &path, Handler handler)
{
    std::lock_guard<std::mutex> lock(apiMutex);
    putHandlers[apiPrefix + path] = handler;
}

void RESTAPI::onDelete(const std::string &path, Handler handler)
{
    std::lock_guard<std::mutex> lock(apiMutex);
    deleteHandlers[apiPrefix + path] = handler;
}

std::string RESTAPI::handleGet(const std::string &path, const std::map<std::string, std::string> &params)
{
    std::lock_guard<std::mutex> lock(apiMutex);
    auto it = getHandlers.find(path);
    if (it != getHandlers.end())
    {
        return it->second(params, "");
    }
    return APIResponse::notFound().serialize();
}

std::string RESTAPI::handlePost(const std::string &path, const std::map<std::string, std::string> &params, const std::string &body)
{
    std::lock_guard<std::mutex> lock(apiMutex);
    auto it = postHandlers.find(path);
    if (it != postHandlers.end())
    {
        return it->second(params, body);
    }
    return APIResponse::notFound().serialize();
}

std::string RESTAPI::handlePut(const std::string &path, const std::map<std::string, std::string> &params, const std::string &body)
{
    std::lock_guard<std::mutex> lock(apiMutex);
    auto it = putHandlers.find(path);
    if (it != putHandlers.end())
    {
        return it->second(params, body);
    }
    return APIResponse::notFound().serialize();
}

std::string RESTAPI::handleDelete(const std::string &path, const std::map<std::string, std::string> &params)
{
    std::lock_guard<std::mutex> lock(apiMutex);
    auto it = deleteHandlers.find(path);
    if (it != deleteHandlers.end())
    {
        return it->second(params, "");
    }
    return APIResponse::notFound().serialize();
}

std::string RESTAPI::getApiPrefix() const
{
    return apiPrefix;
}

void RESTAPI::setApiPrefix(const std::string &prefix)
{
    std::lock_guard<std::mutex> lock(apiMutex);
    apiPrefix = prefix;
}

void RESTAPI::removeHandler(const std::string &method, const std::string &path)
{
    std::lock_guard<std::mutex> lock(apiMutex);
    if (method == "GET")
        getHandlers.erase(apiPrefix + path);
    else if (method == "POST")
        postHandlers.erase(apiPrefix + path);
    else if (method == "PUT")
        putHandlers.erase(apiPrefix + path);
    else if (method == "DELETE")
        deleteHandlers.erase(apiPrefix + path);
}

bool RESTAPI::hasHandler(const std::string &method, const std::string &path) const
{
    std::lock_guard<std::mutex> lock(apiMutex);
    if (method == "GET")
        return getHandlers.find(apiPrefix + path) != getHandlers.end();
    if (method == "POST")
        return postHandlers.find(apiPrefix + path) != postHandlers.end();
    if (method == "PUT")
        return putHandlers.find(apiPrefix + path) != putHandlers.end();
    if (method == "DELETE")
        return deleteHandlers.find(apiPrefix + path) != deleteHandlers.end();
    return false;
}

std::vector<std::string> RESTAPI::listEndpoints() const
{
    std::lock_guard<std::mutex> lock(apiMutex);
    std::vector<std::string> endpoints;
    for (const auto &[path, _] : getHandlers)
        endpoints.push_back("GET " + path);
    for (const auto &[path, _] : postHandlers)
        endpoints.push_back("POST " + path);
    for (const auto &[path, _] : putHandlers)
        endpoints.push_back("PUT " + path);
    for (const auto &[path, _] : deleteHandlers)
        endpoints.push_back("DELETE " + path);
    return endpoints;
}

APIResponse::APIResponse() : statusCode(200) {}

APIResponse::APIResponse(int code, const std::string &data) : statusCode(code), body(data) {}

std::string APIResponse::serialize() const
{
    std::stringstream ss;
    ss << "HTTP/1.1 " << statusCode << "\r\n";
    for (const auto &[key, value] : headers)
    {
        ss << key << ": " << value << "\r\n";
    }
    ss << "Content-Length: " << body.size() << "\r\n";
    ss << "\r\n";
    ss << body;
    return ss.str();
}

APIResponse APIResponse::ok(const std::string &data)
{
    APIResponse resp(200, data);
    resp.headers["Content-Type"] = "application/json";
    return resp;
}

APIResponse APIResponse::error(const std::string &message, int code)
{
    APIResponse resp(code, "{\"error\":\"" + message + "\"}");
    resp.headers["Content-Type"] = "application/json";
    return resp;
}

APIResponse APIResponse::notFound()
{
    return error("Not Found", 404);
}

APIResponse APIResponse::unauthorized()
{
    return error("Unauthorized", 401);
}

APIResponse APIResponse::forbidden()
{
    return error("Forbidden", 403);
}

APIResponse APIResponse::created(const std::string &location)
{
    APIResponse resp(201, "{\"location\":\"" + location + "\"}");
    resp.headers["Location"] = location;
    resp.headers["Content-Type"] = "application/json";
    return resp;
}

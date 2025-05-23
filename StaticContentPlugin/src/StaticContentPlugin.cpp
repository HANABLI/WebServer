/**
 * @file StaticContentPlugin.cpp
 *
 * This is a plug-in for the Excalibur web server, designed
 * to serve static content (e.g. files) in response to resource requests.
 *
 * © 2024 by Hatem Nabli
 */

#include <inttypes.h>
#include <Http/IServer.hpp>
#include <Json/Json.hpp>
#include <StringUtils/StringUtils.hpp>
#include <SystemUtils/File.hpp>
#include <WebServer/PluginEntryPoint.hpp>
#include <functional>

#ifdef _WIN32
#    define API __declspec(dllexport)
#else /* POSIX */
#    define API
#endif /* _WIN32 / POSIX */

/**
 * This is the type expected for the entry point function
 * for all server plug-ins
 *
 * @param[in, out] server
 *      This is the server to which to add the plugin.
 *
 * @param[in] configuration
 *      This holds the configuration items of the plugin.
 *
 * @param[in] diagnosticMessageDelegate
 *      This is the function to call to delliver à diagnostic message.
 *
 * @param[in, out] unloadDelegate
 *      This is the funcntion to call in order to unloade the plugin.
 *      if this is set to nullptr on return, it means the plug-in was
 *      unable to load successfully.
 */
extern "C" API void LoadPlugin(
    Http::IServer* server, Json::Value configuration,
    SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate,
    std::function<void()>& unloadDelegate) {
    Uri::Uri uri;
    if (!configuration.Has("space"))
    {
        diagnosticMessageDelegate("", SystemUtils::DiagnosticsSender::Levels::ERROR,
                                  "no 'space' Uri in the configuration");
        return;
    }
    if (!uri.ParseFromString(configuration["space"]))
    {
        diagnosticMessageDelegate("", SystemUtils::DiagnosticsSender::Levels::ERROR,
                                  "unable to parse 'space' uri in the configuration file");
        return;
    }
    auto space = uri.GetPath();
    (void)space.erase(space.begin());
    if (!configuration.Has("root"))
    {
        diagnosticMessageDelegate("", SystemUtils::DiagnosticsSender::Levels::ERROR,
                                  "no 'root' Uri in the configuration");
        return;
    }
    const std::string root = configuration["root"];

    const auto unregistrationDelegate = server->RegisterResource(
        space,
        [root](std::shared_ptr<Http::IServer::Request> request,
               std::shared_ptr<Http::Connection> connection, const std::string& trailer)
        {
            const auto path =
                StringUtils::Join({root, StringUtils::Join(request->target.GetPath(), "/")}, "/");
            SystemUtils::File file(path);
            const auto response = std::make_shared<Http::Client::Response>();
            if (file.IsExisting() && (!file.IsDirectory()))
            {
                if (file.OpenReadOnly())
                {
                    SystemUtils::File::Buffer buffer(file.GetSize());
                    if (file.Read(buffer) == buffer.size())
                    {
                        // TODO replace it with something that gives
                        // a strong entity tag -- this one is weak
                        uint32_t sum = 0;
                        for (auto b : buffer)
                        { sum += b; }
                        const auto etag = StringUtils::sprintf("%" PRIu32, sum);
                        if (request->headers.HasHeader("If-None-Match") &&
                            (request->headers.GetHeaderValue("If-None-Match") == etag))
                        {
                            response->statusCode = 304;
                            response->status = "Not Modified";
                        } else
                        {
                            response->statusCode = 200;
                            response->status = "OK";
                            response->body.assign(buffer.begin(), buffer.end());
                        }
                        response->headers.AddHeader("Content-Type", "text/html");
                        response->headers.AddHeader("Etag", etag);
                    } else
                    {
                        response->statusCode = 204;
                        response->status = "No Content";
                        response->headers.AddHeader("Content-Type", "text/plain");
                        response->body = "ooops can't read the file...!";
                    };
                } else
                {
                    response->statusCode = 500;
                    response->status = "Internal Server Error";
                    response->headers.AddHeader("Content-Type", "text/plain");
                    response->body = "ooops can't open the file...!";
                }
            } else
            {
                response->statusCode = 404;
                response->status = "Not Found";
                response->headers.AddHeader("Content-Type", "text/plain");
                response->body = "Sorry, resource not found...!";
            }

            return response;
        });

    unloadDelegate = [unregistrationDelegate] { unregistrationDelegate(); };
}

/**
 * This checks to make sure the plug-in entry point signature
 * matches the entry point type declared in the web server API.
 */
namespace
{
    PluginEntryPoint EntryPoint = &LoadPlugin;
}  // namespace

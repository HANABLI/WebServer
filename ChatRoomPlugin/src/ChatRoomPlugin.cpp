/**
 * @file ChatRoomPlugin.cpp
 *
 * This is a plug-in for the Excalibur web server, designed
 * to make a Chat Room .
 *
 * © 2025 by Hatem Nabli
 */

#include <inttypes.h>
#include <Http/IServer.hpp>
#include <Json/Json.hpp>
#include <StringUtils/StringUtils.hpp>
#include <SystemUtils/File.hpp>
#include <WebServer/PluginEntryPoint.hpp>
#include <functional>
#include <mutex>

#ifdef _WIN32
#    define API __declspec(dllexport)
#else /* POSIX */
#    define API
#endif /* _WIN32 / POSIX */

namespace
{
    /**
     * This is an user struct to use to test the chat room
     */
    struct User
    {
        /**
         * This is the user name to show for the user.
         */
        std::string userName;

        /**
         * This is the websocket connection to the user.
         */
        WebSocket::WebSocket ws;
    };
    /**
     * This represent the state of the chat room
     */
    struct Room
    {
        /**
         * This synchronise the access to the chat room.
         */
        std::mutex mutex;

        /**
         * These are the users currently connected to the chat room,
         * keyed by session Id.
         */
        std::map<unsigned int, User> users;

        /**
         * This is the next session id that my be assigned to a new
         * user.
         */
        unsigned int nextSessionId = 1;

        // Methods

        /**
         * This method is used whenever a new user is tries to
         * connect to the chat room.
         *
         * @param[in] request
         *      This is the request to connect to the chat room.
         * @param[in] connection
         *      This is the connection on which the request was made.
         * @return
         *      The response to be returned to the client is returned.
         */
        std::shared_ptr<Http::Client::Response> AddUser(
            std::shared_ptr<Http::IServer::Request> request,
            std::shared_ptr<Http::Connection> connection) {
            std::lock_guard<decltype(mutex)> lock(mutex);
            const auto response = std::make_shared<Http::Client::Response>();
            const auto sessionId = ++nextSessionId;
            auto& user = users[sessionId];
            if (!user.ws.OpenAsServer(connection, *request, *response))
            { (void)users.erase(sessionId); }
            return response;
        }
    } room;
}  // namespace

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
    Http::IServer* server, Json::Json configuration,
    SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate,
    std::function<void()>& unloadDelegate) {
    Uri::Uri uri;
    if (!configuration.Has("space"))
    {
        diagnosticMessageDelegate("", SystemUtils::DiagnosticsSender::Levels::ERROR,
                                  "no 'space' Uri in the configuration");
        return;
    }
    if (!uri.ParseFromString(*configuration["space"]))
    {
        diagnosticMessageDelegate("", SystemUtils::DiagnosticsSender::Levels::ERROR,
                                  "unable to parse 'space' uri in the configuration file");
        return;
    }
    auto space = uri.GetPath();
    (void)space.erase(space.begin());

    const auto unregistrationDelegate =
        server->RegisterResource(space,
                                 [](std::shared_ptr<Http::IServer::Request> request,
                                    std::shared_ptr<Http::Connection> connection)
                                 {
                                     auto response = room.AddUser(request, connection);

                                     response->statusCode = 200;
                                     response->status = "OK";
                                     response->headers.AddHeader("Content-Type", "text/plain");
                                     response->body = "Coming soon...!";

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

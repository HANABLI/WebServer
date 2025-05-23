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
#include <WebSocket/WebSocket.hpp>
#include <functional>
#include <condition_variable>
#include <thread>
#include <mutex>

#ifdef _WIN32
#    define API __declspec(dllexport)
#else /* POSIX */
#    define API
#endif /* _WIN32 / POSIX */

namespace
{
    /**
     * This is the number of milliseconds to wait between rounds of polling
     * in the worker thread of the chat room.
     */
    constexpr unsigned int WORKER_POLLING_PERIOD_MILLISECONDS = 50;
    /**
     * This is a registred user of the chat room
     */
    struct Account
    {
        /**
         * This is the string that the user needs to present
         * for the value of "Password" when setting their nickname
         * in order to link the user to the account
         */
        std::string password;
    };

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

        /**
         * This indicate whether or not the user is connected to the room.
         */
        bool open = true;

        /**
         * These are the diagnostic sender name of the user.
         */
        std::string diagnosticSenderName;
        /**
         * This is the delegate obtained when subscribing
         * to receive diagnostic messages from the unit under test.
         * It's called to terminate the subscription.
         */
        SystemUtils::DiagnosticsSender::UnsubscribeDelegate wsDiagnosticsUnsubscribeDelegate;
    };

    struct ChatMessage
    {
        std::string timestamp;
        std::string sender;
        std::string message;

        bool operator<(const ChatMessage& other) const { return timestamp < other.timestamp; }
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
         * This is used to notify the worker for any change that
         * should cause it to wake up.
         */
        std::condition_variable workerWakeCondition;

        /**
         * This is used to perform housekeeping in the backgroud.
         */
        std::thread workerThread;

        /**
         * This indicates whether or not the worker thread should
         * stop working.
         */
        bool stopWorker = false;

        /**
         * This indicates whether an user have to close the ws.
         */
        bool usersHaveClosed = false;

        /**
         * This indicates whether an user join the room with an user name.
         */
        bool userJoinRoom = false;

        /**
         * These are the users currently connected to the chat room,
         * keyed by session Id.
         */
        std::map<unsigned int, User> users;

        /**
         * These are the registered users of the chat room, keyed by
         * userName
         */
        std::map<std::string, Account> accounts;

        /**
         *
         */
        std::set<ChatMessage> chatLog;
        /**
         * This is the next session id that my be assigned to a new
         * user.
         */
        unsigned int nextSessionId = 1;

        /**
         * This is the delegate obtained when subscribing
         * to receive diagnostic messages from the unit under test.
         * It's called to terminate the subscription.
         */
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticsMessageDelegate;

        // Methods
        /**
         * This is called before the chat room is connected into the web server
         * in order to prepare it for operating.
         */
        void Start() {
            if (workerThread.joinable())
            { return; }
            stopWorker = false;
            workerThread = std::thread(&Room::Worker, this);
        }

        /**
         * This is called when the chat room is disconnected frome the web
         * server in order to cleanly shut it down.
         */
        void Stop() {
            if (!workerThread.joinable())
            { return; }
            {
                std::lock_guard<decltype(mutex)> lock(mutex);
                stopWorker = true;
                workerWakeCondition.notify_all();
            }
            workerThread.join();
        }

        /**
         * This method handles the "GetUserNames" message from users
         * in the chat room.
         *
         * @param[in] userEntry
         *      This is the entry of the user who sent the message.
         */
        void GetUsersName(std::map<unsigned int, User>::iterator userEntry) {
            Json::Value response(Json::Value::Type::Object);
            response.Set("Type", "UserNames");
            std::set<std::string> userNamesSet;
            for (const auto& user : users)
            {
                if (!user.second.userName.empty())
                { userNamesSet.insert(user.second.userName); }
            }
            Json::Value userNames(Json::Value::Type::Array);
            for (const auto& userName : userNamesSet)
            {
                { userNames.Add(userName); }
            }
            response.Set("UserNames", userNames);
            userEntry->second.ws.SendText(response.ToEncoding());
        }

        /**
         * This is called in a separate thread to perform
         * housekeeping in the background for the chat room.
         */
        void Worker() {
            std::unique_lock<decltype(mutex)> lock(mutex);
            while (!stopWorker)
            {
                workerWakeCondition.wait_for(
                    lock, std::chrono::milliseconds(WORKER_POLLING_PERIOD_MILLISECONDS),
                    [this] { return stopWorker || usersHaveClosed || userJoinRoom; });
                if (usersHaveClosed)
                {
                    std::vector<User> closedUsers;
                    for (auto user = users.begin(); user != users.end();)
                    {
                        if (user->second.open)
                        {
                            ++user;
                        } else
                        {
                            const auto userName = user->second.userName;
                            closedUsers.push_back(std::move(user->second));
                            user = users.erase(user);
                            bool stillInTheRoom = false;
                            for (const auto& userEntry : users)
                            {
                                if (userEntry.second.userName == userName)
                                {
                                    stillInTheRoom = true;
                                    break;
                                }
                            }
                            if (!stillInTheRoom)
                            {
                                Json::Value response(Json::Value::Type::Object);
                                response.Set("Type", "Leave");
                                response.Set("UserName", userName);
                                const auto responseToEncoding = response.ToEncoding();
                                for (auto user = users.begin(); user != users.end();)
                                {
                                    user->second.ws.SendText(responseToEncoding);
                                    if (user->second.open)
                                    { GetUsersName(user); }
                                    ++user;
                                }
                            }
                        }
                    }
                    usersHaveClosed = false;
                    {
                        lock.unlock();
                        closedUsers.clear();
                        lock.lock();
                    }
                }
                if (userJoinRoom)
                {
                    for (auto user = users.begin(); user != users.end();)
                    {
                        GetUsersName(user);
                        ++user;
                    }
                    userJoinRoom = false;
                }
            }
        }

        /**
         * This method handles the "SetUserName" message from users
         * in the chat room.
         *
         * @param[in] message
         *      This is the content of the user message.
         * @param[in] userEntry
         *      This is the entry of the user who sent the message.
         */
        void SetUserName(const Json::Value& message,
                         std::map<unsigned int, User>::iterator userEntry) {
            const std::string userName = message["UserName"];
            const std::string password = message["Password"];
            Json::Value response(Json::Value::Type::Object);
            response.Set("Type", "SetUserNameResult");
            auto accountEntry = accounts.find(userName);
            if (!userName.empty() &&
                (accountEntry == accounts.end() || accountEntry->second.password == password))
            {
                const auto oldUserName = userEntry->second.userName;
                userEntry->second.userName = userName;
                auto& account = accounts[userName];
                account.password = password;
                response.Set("Success", true);
                diagnosticsMessageDelegate(
                    userEntry->second.diagnosticSenderName, 1,
                    StringUtils::sprintf("User name changed from '%s' to '%s'", oldUserName.c_str(),
                                         userName.c_str()));
            } else
            { response.Set("Success", false); }
            userEntry->second.ws.SendText(response.ToEncoding());
        }
        /**
         *
         */
        void JoinChatRoom(const Json::Value& message,
                          std::map<unsigned int, User>::iterator userEntry) {
            const std::string Sender = message["userSender"];
            Json::Value response(Json::Value::Type::Object);

            response.Set("Type", "JoinChatRoomResponse");
            response.Set("Success", true);
            Json::Value chatLogToSend(Json::Value::Type::Array);
            for (const auto& chat : chatLog)
            {
                Json::Value chatObj(Json::Value::Type::Object);
                chatObj.Set("Time", chat.timestamp);
                chatObj.Set("Sender", chat.sender);
                chatObj.Set("Chat", chat.message);
                chatLogToSend.Add(chatObj);
            }
            response.Set("ChatLog", chatLogToSend);
            Json::Value userNames(Json::Value::Type::Array);
            std::set<std::string> userNamesSet;
            for (const auto& user : users)
            {
                if (!user.second.userName.empty())
                { userNamesSet.insert(user.second.userName); }
            }
            for (const auto& userName : userNamesSet)
            {
                { userNames.Add(userName); }
            }
            response.Set("UserNames", userNames);
            userEntry->second.ws.SendText(response.ToEncoding());
            userJoinRoom = true;
            workerWakeCondition.notify_all();
        }
        /**
         * This method handles the "Chat" message from
         * users in the chat room.
         *
         * @param[in] message
         *      This is the content of the user message.
         * @param[in] userEntry
         *      This is the entry of the user whos ent the message.
         */
        void Chat(const Json::Value& message, std::map<unsigned int, User>::iterator userEntry) {
            const std::string chat = message["Chat"];
            const std::string timeIn = message["Time"];
            if (chat.empty())
            { return; }
            ChatMessage msg = {timeIn, userEntry->second.userName, chat};
            chatLog.insert(msg);
            Json::Value response(Json::Value::Type::Object);
            response.Set("Type", "PostChatResult");
            response.Set("Sender", userEntry->second.userName);
            response.Set("Chat", chat);
            response.Set("Time", timeIn);
            std::string respEncod = response.ToEncoding();
            for (auto& user : users)
            { user.second.ws.SendText(respEncod); }
            diagnosticsMessageDelegate(
                userEntry->second.diagnosticSenderName, 1,
                StringUtils::sprintf("User '%s' sent '%s' to the room",
                                     userEntry->second.userName.c_str(), chat.c_str()));
        }

        /**
         * This is called whenever a text message is received from an user
         * in the chat room.
         *
         * @param[in] sessionId
         *      This is the session ID of the user who sent the message.
         *
         * @param[in] data
         *      This is the content of the received message from the user.
         *
         */
        void ReceiveMessage(unsigned int sessionId, const std::string& data) {
            std::lock_guard<decltype(mutex)> lock(mutex);
            const auto userEntry = users.find(sessionId);
            if (userEntry == users.end())
            { return; }
            const auto message = Json::Value::FromEncoding(data);
            if ((message["Type"] == "SetUserName") && message.Has("UserName"))
            {
                SetUserName(message, userEntry);

            } else if (message["Type"] == "GetUserNames")
            {
                GetUsersName(userEntry);
            } else if (message["Type"] == "PostChat" && message.Has("Chat"))
            {
                Chat(message, userEntry);
            } else if (message["Type"] == "JoinChatRoom")
            { JoinChatRoom(message, userEntry); }
        }

        /**
         * This is called to remove user from the chat room when websocket
         * connection is closed.
         *
         * @param[in] sessionId
         *      This is the session ID of the user to remove from chat room.
         */
        void RemoveUser(unsigned int sessionId, unsigned int code, const std::string& reason) {
            std::lock_guard<decltype(mutex)> lock(mutex);
            const auto user = users.find(sessionId);
            if (user == users.end())
            { return; }
            user->second.ws.Close(code, reason);
            user->second.open = false;
            usersHaveClosed = true;
            workerWakeCondition.notify_all();
        }

        /**
         * This method is used whenever a new user is tries to
         * connect to the chat room.
         *
         * @param[in] request
         *      This is the request to connect to the chat room.
         * @param[in] connection
         *      This is the connection on which the request was made.
         * @param[in] trailer
         *      This holds any characters that have already been received
         *      by the server and come after the end of the request.
         *      A handler that upgrades this connection might want to interpret
         *      these characters whithin the context of the upgraded connection
         * @return
         *      The response to be returned to the client is returned.
         */
        std::shared_ptr<Http::Client::Response> AddUser(
            std::shared_ptr<Http::IServer::Request> request,
            std::shared_ptr<Http::Connection> connection, const std::string& trailer) {
            std::lock_guard<decltype(mutex)> lock(mutex);
            const auto response = std::make_shared<Http::Client::Response>();
            const auto sessionId = nextSessionId++;
            auto& user = users[sessionId];
            const auto diagnosticSenderName = StringUtils::sprintf(" Session #%zu", sessionId);
            user.diagnosticSenderName = diagnosticSenderName;
            user.wsDiagnosticsUnsubscribeDelegate = user.ws.SubscribeToDiagnostics(
                [this, diagnosticSenderName](std::string senderName, size_t level,
                                             std::string message)
                { diagnosticsMessageDelegate(diagnosticSenderName, level, message); });
            user.ws.SetTextDelegate([this, sessionId](const std::string& data)
                                    { ReceiveMessage(sessionId, data); });
            user.ws.SetCloseDelegate([this, sessionId](unsigned int code, const std::string& reason)
                                     { RemoveUser(sessionId, code, reason); });
            if (!user.ws.OpenAsServer(connection, *request, *response, trailer))
            {
                (void)users.erase(sessionId);
                response->statusCode = 200;
                response->headers.SetHeader("Content-Type", "Text/plain");
                response->body = "Try again, but next time use a WebSocket. thxbye!";
            }
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

    room.diagnosticsMessageDelegate = diagnosticMessageDelegate;
    room.Start();
    const auto unregistrationDelegate = server->RegisterResource(
        space, [](std::shared_ptr<Http::IServer::Request> request,
                  std::shared_ptr<Http::Connection> connection, const std::string& trailer)
        { return room.AddUser(request, connection, trailer); });

    unloadDelegate = [unregistrationDelegate]
    {
        unregistrationDelegate();
        room.Stop();
        room.users.clear();
        room.accounts.clear();
        room.usersHaveClosed = false;
        room.diagnosticsMessageDelegate = nullptr;
        room.nextSessionId = 1;
    };
}

/**
 * This checks to make sure the plug-in entry point signature
 * matches the entry point type declared in the web server API.
 */
namespace
{
    PluginEntryPoint EntryPoint = &LoadPlugin;
}  // namespace

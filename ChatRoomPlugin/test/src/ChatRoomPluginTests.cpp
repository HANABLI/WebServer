/**
 * @file ChatRoomPluginTests.cpp
 *
 * This module contains unit tests of the
 * ChatRoomPlugin class.
 *
 * © 2025 by Hatem Nabli
 */

#include <gtest/gtest.h>
#include <mutex>
#include <condition_variable>
#include <SystemUtils/File.hpp>
#include <StringUtils/StringUtils.hpp>
#include <WebServer/PluginEntryPoint.hpp>
#include <WebSocket/WebSocket.hpp>
#include <functional>

#ifdef _WIN32
#    define API __declspec(dllimport)
#else /* POSIX */
#    define API
#endif /* _WIN32 / POSIX */
extern "C" API void LoadPlugin(
    Http::IServer* server, Json::Value configuration,
    SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessagedelegate,
    std::function<void()>& unloadDelegate);

namespace
{
    /**
     * This is the path in the server at which to place the plug-in.
     */
    const std::string CHAT_ROOM_PATH = "/chat";

    /**
     * This is the number of mock clients to connect to the chat room.
     */
    constexpr size_t NUM_MOCK_CLIENTS = 3;

    /**
     * This simulates the actual web server hosting the chat room.
     */
    struct MockServer : public Http::IServer
    {
        // Properties
        /**
         * This is the resource subspace path that the unit under
         * test has registered.
         */
        std::vector<std::string> registeredResourcesSubspacePath;
        /**
         * This is the delegate that the unit under test has registered
         * to be called to handel resource requests.
         */
        ResourceDelegate registredResourceDelegate;

        // Methods

        // IServer
    public:
        virtual std::string GetConfigurationItem(const std::string& password) override {
            return "";
        }
        virtual void SetConfigurationItem(const std::string& password,
                                          const std::string& value) override {
            return;
        }
        virtual SystemUtils::DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate delegate,
            size_t minLevel = 0) override {
            return []() {};
        }
        virtual UnregistrationDelegate RegisterResource(
            const std::vector<std::string>& resourceSubspacePath,
            ResourceDelegate resourceDelegate) override {
            registeredResourcesSubspacePath = resourceSubspacePath;
            registredResourceDelegate = resourceDelegate;
            return []() {};
        }
    };

    /**
     * This is a fake connection which is used with both ends of
     * the WebSockets going between the chat room and the test framework.
     */
    struct MockConnection : public Http::Connection
    {
        // Properies

        /**
         * This is the delegate to call whenever data is received from
         * the remote peer.
         */
        DataReceivedDelegate dataReceivedDelegate;

        /**
         * This is the delegate to call whenever data will be sent to
         * the remote peer.
         */
        DataReceivedDelegate sendDataDelegate;

        /**
         * This is the delegate to call whenever connection has been broken.
         */
        BrokenDelegate brokenDelegate;

        /**
         * This is the data received from the remote peer.
         */
        std::vector<uint8_t> dataReceived;

        /**
         * This flag is set if  the remote close the connection
         */
        bool broken = false;

        /**
         * This is the identifier to report for the peer of the connection.
         */
        std::string peerId;

        // Methods

        explicit MockConnection(const std::string& peerId) : peerId(peerId){};

        // Http::Connection

        virtual std::string GetPeerId() override { return "mock-client"; }

        virtual void SetDataReceivedDelegate(
            DataReceivedDelegate newDataReceivedDelegate) override {
            dataReceivedDelegate = newDataReceivedDelegate;
        }

        virtual void SetConnectionBrokenDelegate(BrokenDelegate newBrokenDelegate) override {
            brokenDelegate = newBrokenDelegate;
        }

        virtual void SendData(const std::vector<uint8_t>& data) override { sendDataDelegate(data); }

        virtual void Break(bool clean) override { broken = true; }
    };
}  // namespace

struct ChatRoomPluginTests : public ::testing::Test
{
    // Properties

    /**
     * This simulates the actual web server hosting the chat room.
     */
    MockServer server;

    /**
     * This is the function to call to unload the chat room plugin.
     */
    std::function<void()> unloadDelegate;

    /**
     * This is used to synchronize access to the wsClosed flags
     * and the wsWaitCondition variable
     */
    std::mutex wsMutex;

    /**
     * This is used to wait for, or signal, the condition of one or more
     * wsClosed flags being set.
     */
    std::condition_variable wsWaitCondition;

    /**
     * This is used to connect with the chat room and communicate with it.
     */
    WebSocket::WebSocket ws[NUM_MOCK_CLIENTS];

    /**
     * This indicate whether or not the corresponding  webSocket have been closed
     */
    bool wsClosed[NUM_MOCK_CLIENTS];

    /**
     * This simulate the client side of the HTTP connection between the client
     * and the chat room.
     */
    std::shared_ptr<MockConnection> clientConnection[NUM_MOCK_CLIENTS];

    /**
     * This simulate the client side of the HTTP connection between the server
     * and the chat room.
     */
    std::shared_ptr<MockConnection> serverConnection[NUM_MOCK_CLIENTS];
    /**
     * This store all messages received from the chat room.
     */
    std::vector<Json::Value> messagesReceived[NUM_MOCK_CLIENTS];
    // Methods

    void InitilizeClientWebsocket(size_t i) {
        clientConnection[i] =
            std::make_shared<MockConnection>(StringUtils::sprintf("mock-client-%zu", i));
        serverConnection[i] =
            std::make_shared<MockConnection>(StringUtils::sprintf("mock-server-%zu", i));

        clientConnection[i]->sendDataDelegate = [this, i](const std::vector<uint8_t>& data)
        { serverConnection[i]->dataReceivedDelegate(data); };
        serverConnection[i]->sendDataDelegate = [this, i](const std::vector<uint8_t>& data)
        { clientConnection[i]->dataReceivedDelegate(data); };
        wsClosed[i] = false;
        ws[i].SetTextDelegate(
            [this, i](const std::string& data)
            {
                std::lock_guard<decltype(wsMutex)> lock(wsMutex);
                messagesReceived[i].push_back(Json::Value::FromEncoding(data));
                wsWaitCondition.notify_all();
            });
        ws[i].SetCloseDelegate(
            [this, i](unsigned int code, const std::string& reason)
            {
                std::lock_guard<decltype(wsMutex)> lock(wsMutex);
                wsClosed[i] = true;
                wsWaitCondition.notify_all();
            });
    }
    // ::testing::Test

    virtual void SetUp() {
        Json::Value config(Json::Value::Type::Object);
        config.Set("space", CHAT_ROOM_PATH);
        LoadPlugin(
            &server, config,
            [](std::string senderName, size_t level, std::string message)
            { printf("[%s:%zu] %s\n", senderName.c_str(), level, message.c_str()); },
            unloadDelegate);
        for (size_t i = 0; i < NUM_MOCK_CLIENTS; ++i)
        {
            InitilizeClientWebsocket(i);
            const auto openRequest = std::make_shared<Http::Server::Request>();
            openRequest->method = "GET";
            (void)openRequest->target.ParseFromString("/chat");
            ws[i].StartOpenAsClient(*openRequest);
            const auto openResponse =
                server.registredResourceDelegate(openRequest, serverConnection[i], "");
            ASSERT_TRUE(ws[i].CompleteOpenAsClient(clientConnection[i], *openResponse));
        }
    }

    virtual void TearDown() { unloadDelegate(); }
};

TEST_F(ChatRoomPluginTests, ChatRoomPluginTests_Load_Test) {
    ASSERT_FALSE(unloadDelegate == nullptr);
    ASSERT_FALSE(server.registredResourceDelegate == nullptr);
    ASSERT_EQ((std::vector<std::string>{
                  "chat",
              }),
              server.registeredResourcesSubspacePath);
}

TEST_F(ChatRoomPluginTests, ChatRoomPluginTests_SetUserName_Test) {
    const std::string password = "PopChamp";
    ws[0].SendText(StringUtils::sprintf(
        "{\"Type\": \"SetUserName\", \"UserName\": \"Hatem\", \"Password\": \"%s\"}",
        password.c_str()));

    ASSERT_EQ((std::vector<Json::Value>{
                  Json::Value::FromEncoding("{\"Type\": \"SetUserNameResult\", \"Success\": true}"),
              }),
              messagesReceived[0]);
    messagesReceived[0].clear();
    ws[0].SendText("{\"Type\": \"GetUserNames\"}");
    ASSERT_EQ(
        (std::vector<Json::Value>{
            Json::Value::FromEncoding("{\"Type\": \"UserNames\", \"UserNames\": [\"Hatem\"]}"),
        }),
        messagesReceived[0]);
    messagesReceived[0].clear();
}

TEST_F(ChatRoomPluginTests, ChatRoomPluginTests_SetUserNameTwice_Test) {
    // Set userName for first client
    const std::string password1 = "PopChamp";
    Json::Value message(Json::Value::Type::Object);
    message.Set("Type", "SetUserName");
    message.Set("UserName", "Hatem");
    message.Set("Password", password1);
    ws[0].SendText(message.ToEncoding());
    Json::Value expectedResponse(Json::Value::Type::Object);
    expectedResponse.Set("Type", "SetUserNameResult");
    expectedResponse.Set("Success", true);
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[0]);
    messagesReceived[0].clear();
    // Set user name for seconde client with same name as the first one.
    const std::string password2 = "PopOps";
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "SetUserName");
    message.Set("UserName", "Hatem");
    message.Set("Password", password2);
    ws[1].SendText(message.ToEncoding());
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "SetUserNameResult");
    expectedResponse.Set("Success", false);
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[1]);
    messagesReceived[1].clear();
    // Set userName for third client, trying to
    // grab the same userName as the first client,
    // with the correct password. this is allowed, and
    // in a real-world, might be the same user joining
    // the chat room from two separate browser windows.
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "SetUserName");
    message.Set("UserName", "Hatem");
    message.Set("Password", password1);
    ws[2].SendText(message.ToEncoding());
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "SetUserNameResult");
    expectedResponse.Set("Success", true);
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[2]);
    messagesReceived[2].clear();
    // get userNames list
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "GetUserNames");
    ws[1].SendText(message.ToEncoding());
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "UserNames");
    expectedResponse.Set("UserNames", {"Hatem"});
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[1]);
    messagesReceived[1].clear();
}

TEST_F(ChatRoomPluginTests, ChatRoomPluginTests_RoomCleanAfterUnload_Test) {
    // Set userName for first client
    const std::string password1 = "PopChamp";
    Json::Value message(Json::Value::Type::Object);
    message.Set("Type", "SetUserName");
    message.Set("UserName", "Hatem");
    message.Set("Password", password1);
    ws[0].SendText(message.ToEncoding());
    Json::Value expectedResponse(Json::Value::Type::Object);
    expectedResponse.Set("Type", "SetUserNameResult");
    expectedResponse.Set("Success", true);
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[0]);
    messagesReceived[0].clear();
    // Reload chat room.
    TearDown();
    SetUp();

    // Check if user name list is empty.
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "GetUserNames");
    ws[0].SendText(message.ToEncoding());
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "UserNames");
    expectedResponse.Set("UserNames", Json::Value::Type::Array);
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[0]);
    messagesReceived[0].clear();
}

TEST_F(ChatRoomPluginTests, ChatRoomPluginTests_TellSomeThings_Test) {
    // Hatem join the chat room.
    const std::string password1 = "PopChamp";
    Json::Value message(Json::Value::Type::Object);
    message.Set("Type", "SetUserName");
    message.Set("UserName", "Hatem");
    message.Set("Password", password1);
    ws[0].SendText(message.ToEncoding());
    Json::Value expectedResponse(Json::Value::Type::Object);
    expectedResponse.Set("Type", "SetUserNameResult");
    expectedResponse.Set("Success", true);
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[0]);
    messagesReceived[0].clear();
    // Maya join the chat room.
    const std::string password2 = "PopOps";
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "SetUserName");
    message.Set("UserName", "Maya");
    message.Set("Password", password2);
    ws[1].SendText(message.ToEncoding());
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "SetUserNameResult");
    expectedResponse.Set("Success", true);
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[1]);
    messagesReceived[1].clear();

    // Check if users was in the room.
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "GetUserNames");
    ws[0].SendText(message.ToEncoding());
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "UserNames");
    expectedResponse.Set("UserNames", {"Hatem", "Maya"});
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[0]);
    messagesReceived[0].clear();

    // Maya says some things.
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "PostChat");
    message.Set("Chat", "Hello");
    ws[1].SendText(message.ToEncoding());
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "PostChatResult");
    expectedResponse.Set("Sender", "Maya");
    expectedResponse.Set("Chat", "Hello");
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[1]);
    messagesReceived[1].clear();
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[0]);
    messagesReceived[0].clear();
}

TEST_F(ChatRoomPluginTests, ChatRoomPluginTests_LeaveTheRoom_Test) {
    // Hatem join the chat room.
    const std::string password1 = "PopChamp";
    Json::Value message(Json::Value::Type::Object);
    message.Set("Type", "SetUserName");
    message.Set("UserName", "Hatem");
    message.Set("Password", password1);
    ws[0].SendText(message.ToEncoding());
    Json::Value expectedResponse(Json::Value::Type::Object);
    expectedResponse.Set("Type", "SetUserNameResult");
    expectedResponse.Set("Success", true);
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[0]);
    messagesReceived[0].clear();
    // Maya join the chat room.
    const std::string password2 = "PopOps";
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "SetUserName");
    message.Set("UserName", "Maya");
    message.Set("Password", password2);
    ws[1].SendText(message.ToEncoding());
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "SetUserNameResult");
    expectedResponse.Set("Success", true);
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[1]);
    messagesReceived[1].clear();

    // Check if users was in the room.
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "GetUserNames");
    ws[0].SendText(message.ToEncoding());
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "UserNames");
    expectedResponse.Set("UserNames", {"Hatem", "Maya"});
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[0]);
    messagesReceived[0].clear();

    // Maya says some things.
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "PostChat");
    message.Set("Chat", "Hello");
    ws[1].SendText(message.ToEncoding());
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "PostChatResult");
    expectedResponse.Set("Sender", "Maya");
    expectedResponse.Set("Chat", "Hello");
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[1]);
    messagesReceived[1].clear();
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[0]);
    messagesReceived[0].clear();

    // Maya leaves the room.
    ws[1].Close();
    {
        std::unique_lock<decltype(wsMutex)> lock(wsMutex);
        wsWaitCondition.wait(lock,
                             [this] { return (!messagesReceived[0].empty() && wsClosed[1]); });
    }

    // Hatem peeks at the chat room member list.
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "GetUserNames");
    ws[0].SendText(message.ToEncoding());
    std::vector<Json::Value> expectedResponses;
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "Leave");
    expectedResponse.Set("UserName", "Maya");
    expectedResponses.push_back(expectedResponse);
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "UserNames");
    expectedResponse.Set("UserNames", {"Hatem"});
    expectedResponses.push_back(expectedResponse);
    ASSERT_EQ(expectedResponses, messagesReceived[0]);
    messagesReceived[0].clear();
}

TEST_F(ChatRoomPluginTests, ChatRoomPluginTests_SetUserNameInTrailer_test) {
    // Reopen a WebSocket connection with part of a "SetUserName" message
    // captured in the trailer.
    const std::string password1 = "PopChamp";
    Json::Value message(Json::Value::Type::Object);
    message.Set("Type", "SetUserName");
    message.Set("UserName", "Hatem");
    message.Set("Password", password1);
    const auto messageToEncoding = message.ToEncoding();
    const char mask[4] = {0x12, 0x34, 0x56, 0x78};
    std::string frame = "\x81";
    frame += (char)(0X80 + messageToEncoding.length());
    frame += std::string(mask, 4);
    for (size_t i = 0; i < messageToEncoding.length(); ++i)
    { frame += messageToEncoding[i] ^ mask[i % 4]; }
    const auto frameFirstHalf = frame.substr(0, frame.length() / 2);
    const auto frameSecondHalf = frame.substr(frame.length() / 2);
    ws[0].Close();
    {
        std::unique_lock<decltype(wsMutex)> lock(wsMutex);
        wsWaitCondition.wait(lock, [this] { return (wsClosed[0]); });
    }
    const auto openRequest = std::make_shared<Http::Server::Request>();
    openRequest->method = "GET";
    (void)openRequest->target.ParseFromString("/chat");
    ws[0] = WebSocket::WebSocket();
    InitilizeClientWebsocket(0);
    ws[0].StartOpenAsClient(*openRequest);
    const auto openResponse =
        server.registredResourceDelegate(openRequest, serverConnection[0], frameFirstHalf);
    ASSERT_TRUE(ws[0].CompleteOpenAsClient(clientConnection[0], *openResponse));

    serverConnection[0]->dataReceivedDelegate(
        std::vector<uint8_t>(frameSecondHalf.begin(), frameSecondHalf.end()));
    Json::Value expectedResponse(Json::Value::Type::Object);
    expectedResponse.Set("Type", "SetUserNameResult");
    expectedResponse.Set("Success", true);
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[0]);
    messagesReceived[0].clear();
    // get userNames list
    message = Json::Value(Json::Value::Type::Object);
    message.Set("Type", "GetUserNames");
    ws[0].SendText(message.ToEncoding());
    expectedResponse = Json::Value(Json::Value::Type::Object);
    expectedResponse.Set("Type", "UserNames");
    expectedResponse.Set("UserNames", {"Hatem"});
    ASSERT_EQ((std::vector<Json::Value>{expectedResponse}), messagesReceived[0]);
    messagesReceived[0].clear();
}

/**
 * @file ChatRoomPluginTests.cpp
 *
 * This module contains unit tests of the
 * ChatRoomPlugin class.
 *
 * © 2025 by Hatem Nabli
 */

#include <gtest/gtest.h>
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
     * This is used to connect with the chat room and communicate with it.
     */
    WebSocket::WebSocket ws[NUM_MOCK_CLIENTS];

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
            clientConnection[i] =
                std::make_shared<MockConnection>(StringUtils::sprintf("mock-client-%zu", i));
            serverConnection[i] =
                std::make_shared<MockConnection>(StringUtils::sprintf("mock-server-%zu", i));

            clientConnection[i]->sendDataDelegate = [this, i](const std::vector<uint8_t>& data)
            { serverConnection[i]->dataReceivedDelegate(data); };
            serverConnection[i]->sendDataDelegate = [this, i](const std::vector<uint8_t>& data)
            { clientConnection[i]->dataReceivedDelegate(data); };
            ws[i].SetTextDelegate(
                [this, i](const std::string& data)
                { messagesReceived[i].push_back(Json::Value::FromEncoding(data)); });
            const auto openRequest = std::make_shared<Http::Server::Request>();
            openRequest->method = "GET";
            (void)openRequest->target.ParseFromString("/chat");
            ws[i].StartOpenAsClient(*openRequest);
            const auto openResponse =
                server.registredResourceDelegate(openRequest, serverConnection[i]);
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
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
#include <WebServer/PluginEntryPoint.hpp>
#include <WebSocket/WebSocket.hpp>
#include <functional>

#ifdef _WIN32
#    define API __declspec(dllimport)
#else /* POSIX */
#    define API
#endif /* _WIN32 / POSIX */
extern "C" API void LoadPlugin(
    Http::IServer* server, Json::Json configuration,
    SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessagedelegate,
    std::function<void()>& unloadDelegate);

namespace
{
    /**
     * This is the path in the server at which to place the plug-in.
     */
    const std::string CHAT_ROOM_PATH = "/chat";

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
        virtual std::string GetConfigurationItem(const std::string& key) override { return ""; }
        virtual void SetConfigurationItem(const std::string& key,
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

        virtual void SendData(const std::vector<uint8_t>& data) override {
            (void)dataReceived.insert(dataReceived.end(), data.begin(), data.end());
        }

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
    WebSocket::WebSocket ws;

    /**
     * This simulate the client side of the HTTP connection between the client
     * and the chat room.
     */
    std::shared_ptr<MockConnection> clientConnection =
        std::make_shared<MockConnection>("mock-client");

    /**
     * This simulate the client side of the HTTP connection between the server
     * and the chat room.
     */
    std::shared_ptr<MockConnection> serverConnection =
        std::make_shared<MockConnection>("mock-server");
    // Methods

    // ::testing::Test

    virtual void SetUp() {
        Json::Json config(Json::Json::Type::Object);
        config.Set("space", CHAT_ROOM_PATH);
        LoadPlugin(
            &server, config,
            [](std::string senderName, size_t level, std::string message)
            { printf("[%s:%zu] %s\n", senderName.c_str(), level, message.c_str()); },
            unloadDelegate);
        const auto openRequest = std::make_shared<Http::Server::Request>();
        openRequest->method = "GET";
        (void)openRequest->target.ParseFromString("/chat");
        ws.StartOpenAsClient(*openRequest);
        const auto openResponse = server.registredResourceDelegate(openRequest, serverConnection);
        ASSERT_TRUE(ws.CompleteOpenAsClient(clientConnection, *openResponse));
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

TEST_F(ChatRoomPluginTests, ChatRoomPluginTests_Connect_Test) {}
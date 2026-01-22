#ifdef _WIN32
#    define API __declspec(dllexport)
#else /* POSIX */
#    define API
#endif /* _WIN32 / POSIX */

#include <Http/IServer.hpp>
#include <Json/Json.hpp>
#include <StringUtils/StringUtils.hpp>
#include <SystemUtils/File.hpp>
#include <WebServer/PluginEntryPoint.hpp>
#include <MqttNetworkTransport/MqttClientNetworkTransport.hpp>
#include <MqttV5/MqttClient.hpp>
#include <WebSocket/WebSocket.hpp>
#include <condition_variable>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <map>
#include "TimeKeeper.hpp"

namespace
{
    /**
     * This is the number of milliseconds to wait between rounds of polling
     * in the worker thread of the chat room.
     */
    constexpr unsigned int WORKER_POLLING_PERIOD_MILLISECONDS = 50;

    constexpr unsigned int PING_POLLING_PERIOD_MILLISECONDS = 50000;
    /**
     *
     */
    bool topicMatchesfilter(const std::string& filter, const std::string& topic) {
        auto f = StringUtils::Split(filter, "/");
        auto t = StringUtils::Split(topic, "/");
        size_t fi = 0;
        size_t ti = 0;

        while (fi < f.size() && ti < t.size())
        {
            const auto& fl = f[fi];
            if (fl == "#")
            {
                return fi + 1 == f.size();
            } else if (fl == "+")
            {
                ++fi;
                ++ti;
            } else
            {
                if (fl != t[ti])
                    return false;
                ++fi;
                ++ti;
            }
        }

        if (ti == t.size())
        {
            if (fi == f.size())
            { return true; }
            if (fi + 1 == f.size() && f[fi] == "#")
            { return true; }
        }
        return false;
    }

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

    enum class CommandeType
    {
        Subscribe,
        Unsubscribe
    };

    struct EndPointCommande
    {
        CommandeType type;
        unsigned int sessionId;
        std::string brokerId;
        std::string topic;
        MqttV5::QoSDelivery qos;
        MqttV5::RetainHandling retainHandling;
        bool withAutoFeadBack;
        bool retainAsPublished;
    };

    struct BrockerConfig
    {
        std::string host = "localhost";
        uint16_t port = 1883;
        bool useTLS = false;
        std::string userName;
        std::string password;
        std::string clientId = "ws-gateway";
        bool cleanSession = true;
        uint16_t reconnectPeriod = 1;
        uint16_t connectTimeOut = 30;
        bool willRetain = false;
        std::string willTopic;
        std::string willPayload;
        MqttV5::QoSDelivery qos = MqttV5::AtLeastOne;
        uint16_t keepAlive = 10U;
        MqttV5::Properties* props = nullptr;
    };

    struct MqttPoint;

    struct WsAppReceiver;

    struct Broker;

    /**
     * This is a device struct to use to test the mqttClient
     */
    struct MqttPoint
    {
        /**
         * This is the user name to show for the user.
         */
        std::string endPointId;

        /**
         * This is the websocket connection to the client.
         */
        std::unique_ptr<WebSocket::WebSocket> ws;

        /**
         * This is the client connection to the broker
         */
        std::weak_ptr<MqttV5::MqttClient> mqttClient;

        /**
         * This is the vector of subscription topics.
         */
        std::vector<std::string> topics;
        /**
         * This indicate whether or not the device is connected to the broker.
         */

        bool connected = true;

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

    /**
     *
     */
    struct WsAppReceiver : public MqttV5::Storage::MessageReceived
    {
        // pointeur sur le broker
        Broker* broker = nullptr;

        void onMessageReceived(const MqttV5::Storage::DynamicStringView topic,
                               MqttV5::Storage::DynamicBinaryDataView payload,
                               uint16_t packetId) override;

        bool onConnectionLost(const MqttV5::IMqttV5Client::Transaction::State& state) override;

        uint32_t maxPacketSize() const override { return 4096u; }
        uint32_t maxUnAckedPackets() const override { return 16u; }
    };

    /**
     *
     */
    struct Broker
    {
        /**
         * This synchronise the access to the broker.
         */
        std::mutex mutex;

        /**
         * This is used to notify the worker for any change that
         * should cause it to wake up.
         */
        std::condition_variable workerWakeCondition;

        /**
         * This is used to perform housekeeping in the background.
         */
        std::thread workerThread;

        /**
         * This indicates whether or not the worker thread should
         * stop working.
         */
        bool stopWorker = false;
        /**
         *
         */
        bool brokerConfigLoaded = false;
        /**
         *
         */
        bool initialConnectPending = false;

        /**
         * This indicates whether a device have to close the ws.
         */
        bool endPointHaveClosed = false;

        /**
         * This indicates whether an endPoint join the server with .
         */
        bool endPointJoinServer = false;

        /**
         * This indicates whether an endPoint Subscribe to new topic.
         */
        bool subscribeNewTopic = false;

        /**
         * This indicates whether an endpoint unSubscribe from a topic.
         */
        bool unsubscribeTopic = false;

        /**
         * This indicates whether an endpoint ping the broker.
         */
        bool ping = false;

        /**
         * This is the broker configurations loaded when the plugin start.
         */
        BrockerConfig mqttConfiguration;
        /**
         *
         */
        std::queue<EndPointCommande> pendingCommandes;
        /**
         * This is the client connection to the broker
         */
        std::shared_ptr<MqttV5::MqttClient> mqttClient;

        /**
         *
         */
        WsAppReceiver appReceiver;

        /**
         * This indicate whether or not the mqttClient is connected to the broker.
         */
        bool mqttConnected = false;

        /**
         * This is the Mqtt Network transport layer.
         */
        std::shared_ptr<MqttNetworkTransport::MqttClientNetworkTransport> mqttTransport;

        /**
         * These are the mqttPoints currently connected to the server,
         * keyed by session Id.
         */
        std::map<unsigned int, std::shared_ptr<MqttPoint>> mqttPoints;

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
        void Start(const Json::Value& configuration) {
            if (workerThread.joinable())
            { return; }

            if (configuration.Has("Host"))
            { mqttConfiguration.host = (std::string)configuration["Host"]; }
            if (configuration.Has("Port"))
            { mqttConfiguration.port = (uint16_t)(int)configuration["Port"]; }
            if (configuration.Has("UserName"))
            { mqttConfiguration.userName = (std::string)configuration["UserName"]; }
            if (configuration.Has("Password"))
            { mqttConfiguration.password = (std::string)configuration["Password"]; }
            if (configuration.Has("Client-Id"))
            { mqttConfiguration.clientId = (std::string)configuration["Client-Id"]; }
            if (configuration.Has("Clean-Session"))
            { mqttConfiguration.cleanSession = (bool)configuration["Clean-Session"]; }
            if (configuration.Has("Reconnect-Period"))
            {
                mqttConfiguration.reconnectPeriod =
                    (uint16_t)(int)configuration["Reconnect-Period"];
            }
            if (configuration.Has("Connect-Timeout"))
            { mqttConfiguration.connectTimeOut = (uint16_t)(int)configuration["Connect-Timeout"]; }
            if (configuration.Has("KeepAlive"))
            { mqttConfiguration.keepAlive = (uint16_t)(int)configuration["KeepAlive"]; }
            if (configuration.Has("Will-Topic"))
            { mqttConfiguration.willTopic = (bool)configuration["Will-Topic"]; }
            if (configuration.Has("Will-Retain"))
            { mqttConfiguration.willRetain = (bool)configuration["Will-Retain"]; }
            if (configuration.Has("Will-Payload"))
            { mqttConfiguration.willPayload = (std::string)configuration["Will-Payload"]; }
            if (configuration.Has("QoS"))
            { mqttConfiguration.qos = (MqttV5::QoSDelivery)(int)configuration["QoS"]; }

            appReceiver.broker = this;
            brokerConfigLoaded = true;
            initialConnectPending = true;
            stopWorker = false;

            workerThread = std::thread(&Broker::Worker, this);
        }

        /**
         * This is called when the ws-gateway is disconnected frome the web
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

        void GetEndPointId(std::map<unsigned int, std::shared_ptr<MqttPoint>>::iterator endPoint) {
            Json::Value response(Json::Value::Type::Object);
            response.Set("Type", "EndPointId");
            std::set<std::string> endPointsSet;
            for (const auto& endPoint : mqttPoints)
            {
                if (endPoint.second->endPointId.empty())
                { endPointsSet.insert(endPoint.second->endPointId); }
            }
            Json::Value mqttendPoints(Json::Value::Type::Array);
            for (const auto& endPoint : endPointsSet)
            { mqttendPoints.Add(endPoint); }
            response.Set("EndPoints", mqttendPoints);
        }

        void Worker() {
            std::unique_lock<decltype(mutex)> lock(mutex);
            int pingPollingPeriod = PING_POLLING_PERIOD_MILLISECONDS;
            while (!stopWorker)
            {
                /* code */
                workerWakeCondition.wait_for(
                    lock, std::chrono::milliseconds(WORKER_POLLING_PERIOD_MILLISECONDS),
                    [this, &pingPollingPeriod]
                    {
                        pingPollingPeriod -= WORKER_POLLING_PERIOD_MILLISECONDS;
                        if (pingPollingPeriod < 0)
                        { ping = true; }
                        return stopWorker || endPointHaveClosed || initialConnectPending ||
                               !pendingCommandes.empty() || ping || endPointJoinServer;
                    });
                if (stopWorker)
                {
                    if (mqttConnected && mqttClient)
                    { mqttClient->Demobilize(); }
                    break;
                }

                if (mqttConnected && (ping || endPointJoinServer))
                {
                    ping = false;
                    if (endPointJoinServer)
                    { endPointJoinServer = false; }
                    pingPollingPeriod = PING_POLLING_PERIOD_MILLISECONDS;
                    lock.unlock();
                    Ping();
                    lock.lock();
                }

                if (initialConnectPending && brokerConfigLoaded && !mqttConnected)
                {
                    initialConnectPending = false;
                    lock.unlock();
                    DoInitialConnect();
                    lock.lock();
                }

                // 3) Handles SUB/UNSUB Transaction
                if (!pendingCommandes.empty() && mqttConnected && mqttClient)
                {
                    EndPointCommande cmd = pendingCommandes.front();
                    pendingCommandes.pop();
                    lock.unlock();
                    if (cmd.type == CommandeType::Subscribe)
                    {
                        HandleSubscribeCommand(cmd);
                    } else if (cmd.type == CommandeType::Unsubscribe)
                    { HandleUnSubscribeCommand(cmd); }

                    lock.lock();
                }

                if (endPointHaveClosed)
                {
                    std::vector<std::shared_ptr<MqttPoint>> closedEndPoints;
                    for (auto endPoint = mqttPoints.begin(); endPoint != mqttPoints.end();)
                    {
                        if (endPoint->second->connected)
                        {
                            ++endPoint;
                        } else
                        {
                            closedEndPoints.push_back(std::move(endPoint->second));
                            endPoint = mqttPoints.erase(endPoint);
                        }
                    }
                    endPointHaveClosed = false;
                    {
                        lock.unlock();
                        closedEndPoints.clear();
                        lock.lock();
                    }
                }
            }
        }

        void DoInitialConnect() {
            // Creation du clien si necessaire
            if (!mqttClient)
            {
                mqttTransport =
                    std::make_shared<MqttNetworkTransport::MqttClientNetworkTransport>();
                // mqttTransport->SubscribeTodiagnostics(diagnosticsMessageDelegate);
                auto timeKeeper = std::make_shared<TimeKeeper>();
                std::unique_ptr<MqttV5::Storage::PacketStore> store(nullptr);

                mqttClient = std::make_shared<MqttV5::MqttClient>(
                    mqttConfiguration.clientId.c_str(), &appReceiver, nullptr, store.get());
                MqttV5::MqttClient::MqttMobilizationDependencies deps;
                deps.transport = mqttTransport;
                deps.timeKeeper = timeKeeper;
                deps.requestTimeoutSeconds = mqttConfiguration.connectTimeOut;
                deps.inactivityInterval = mqttConfiguration.reconnectPeriod;
                mqttClient->Mobilize(deps);
            }

            std::string userName;
            if (!mqttConfiguration.userName.empty())
            { userName = mqttConfiguration.userName; }
            MqttV5::Common::DynamicBinaryData password;
            Utf8::Utf8 utf;
            if (!mqttConfiguration.password.empty())
            {
                auto encodedPass = utf.Encode(Utf8::AsciiToUnicode(mqttConfiguration.password));
                password.data = encodedPass.data();
                password.size = static_cast<uint32_t>(encodedPass.size());
            }
            MqttV5::Common::DynamicBinaryData will;
            if (!mqttConfiguration.willPayload.empty())
            {
                auto encodedWill = utf.Encode(Utf8::AsciiToUnicode(mqttConfiguration.willPayload));
                will.data = encodedWill.data();
                will.size = static_cast<uint32_t>(encodedWill.size());
            }
            MqttV5::WillMessage willMsg;
            willMsg.topicName = mqttConfiguration.willTopic.c_str();
            willMsg.payload = will;
            mqttConfiguration.props->initialize();

            auto transaction = mqttClient->ConnectTo(
                "broker.test", mqttConfiguration.host, mqttConfiguration.port,
                mqttConfiguration.useTLS, mqttConfiguration.cleanSession,
                mqttConfiguration.keepAlive, userName.c_str(), &password, &willMsg,
                mqttConfiguration.qos, mqttConfiguration.willRetain, mqttConfiguration.props);
            if (!transaction)
            {
                diagnosticsMessageDelegate(
                    "MqttClientPlugin", SystemUtils::DiagnosticsSender::Levels::ERROR,
                    "ConnectTo() return null. Check transport/timekeeper/mobilize;");
                return;
            }
            transaction->SetCompletionDelegate(
                [this](std::vector<MqttV5::ReasonCode>& reasons)
                {
                    std::lock_guard<std::mutex> g(mutex);
                    bool ok =
                        !reasons.empty() && reasons.back() == MqttV5::Storage::ReasonCode::Success;
                    mqttConnected = ok;
                    diagnosticsMessageDelegate("MqttClientPlugin",
                                               ok ? SystemUtils::DiagnosticsSender::Levels::INFO
                                                  : SystemUtils::DiagnosticsSender::Levels::ERROR,
                                               ok ? "Mqtt client connected to the broker."
                                                  : "Mqtt client connection failed");
                });
            if (transaction->transactionState ==
                MqttV5::IMqttV5Client::Transaction::State::WaitingForResult)
            {
                if (transaction->AwaitCompletion(
                        std::chrono::milliseconds(mqttConfiguration.connectTimeOut)))
                {
                    switch (transaction->transactionState)
                    {
                    case MqttV5::IMqttV5Client::Transaction::State::Success:
                        diagnosticsMessageDelegate("MqttClientPlugin", 3,
                                                   "Connection established.");
                        break;
                    case MqttV5::IMqttV5Client::Transaction::State::ShunkedPacket:
                        diagnosticsMessageDelegate("MqttClientPlugin", 2, "ShunkedPacket.");
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        void Ping() {
            auto transaction = mqttClient->Ping("broker.test");
            if (!transaction)
            {
                diagnosticsMessageDelegate(
                    "MqttClientPlugin", SystemUtils::DiagnosticsSender::Levels::ERROR,
                    "Ping() return null. Check transport/timekeeper/mobilize;");
                return;
            }
            // transaction->SetCompletionDelegate(
            //     [this](std::vector<MqttV5::ReasonCode>& reasons)
            //     {
            //         std::lock_guard<std::mutex> g(mutex);
            //         bool ok =
            //             !reasons.empty() && reasons.back() ==
            //             MqttV5::Storage::ReasonCode::Success;
            //         mqttConnected = ok;
            //         diagnosticsMessageDelegate("MqttClientPlugin",
            //                                    ok ? SystemUtils::DiagnosticsSender::Levels::INFO
            //                                       :
            //                                       SystemUtils::DiagnosticsSender::Levels::ERROR,
            //                                    ok ? "Mqtt client connected to the broker."
            //                                       : "Mqtt client connection failed");
            //     });
            if (transaction->transactionState ==
                MqttV5::IMqttV5Client::Transaction::State::WaitingForResult)
            {
                if (transaction->AwaitCompletion(
                        std::chrono::milliseconds(mqttConfiguration.connectTimeOut)))
                {
                    switch (transaction->transactionState)
                    {
                    case MqttV5::IMqttV5Client::Transaction::State::Success:
                        diagnosticsMessageDelegate("MqttClientPlugin", 3,
                                                   "Pong Response Received Successfully.");
                        break;
                    case MqttV5::IMqttV5Client::Transaction::State::ShunkedPacket:
                        diagnosticsMessageDelegate("MqttClientPlugin", 2, "ShunkedPacket.");
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        void HandleSubscribeCommand(EndPointCommande cmd) {
            auto transaction = mqttClient->Subscribe(
                "broker.test", cmd.topic.c_str(), cmd.retainHandling, cmd.withAutoFeadBack, cmd.qos,
                cmd.retainAsPublished, mqttConfiguration.props);

            if (!transaction)
            {
                std::lock_guard<std::mutex> g(mutex);
                auto it = mqttPoints.find(cmd.sessionId);
                if (it != mqttPoints.end() && it->second->ws)
                {
                    Json::Value resp(Json::Value::Type::Object);
                    resp.Set("Type", "SubscribeResult");
                    resp.Set("Topic", cmd.topic);
                    resp.Set("Status", "Error");
                    resp.Set("Message", "Subscribe() returned null");
                    it->second->ws->SendText(resp.ToEncoding());
                }
                return;
            }

            transaction->SetCompletionDelegate(
                [this, cmd](std::vector<MqttV5::ReasonCode>& reasons)
                {
                    std::lock_guard<std::mutex> g(mutex);

                    auto it = mqttPoints.find(cmd.sessionId);
                    if (it == mqttPoints.end() || !it->second->ws)
                        return;
                    bool ok = !reasons.empty() && reasons.back() < 0x80;
                    if (ok)
                    { it->second->topics.push_back(cmd.topic); }
                    Json::Value resp(Json::Value::Type::Object);
                    resp.Set("Type", "SubscribeResult");
                    resp.Set("Topic", cmd.topic);
                    resp.Set("Status", ok ? "Success" : "Error");
                    it->second->ws->SendText(resp.ToEncoding());
                });

            if (transaction->transactionState ==
                MqttV5::IMqttV5Client::Transaction::State::WaitingForResult)
            {
                if (transaction->AwaitCompletion(
                        std::chrono::milliseconds(mqttConfiguration.connectTimeOut)))
                {
                    switch (transaction->transactionState)
                    {
                    case MqttV5::IMqttV5Client::Transaction::State::Success:
                        diagnosticsMessageDelegate("MqttClientPlugin", 3,
                                                   "Subscruption response occured Successfully.");
                        break;
                    case MqttV5::IMqttV5Client::Transaction::State::ShunkedPacket:
                        diagnosticsMessageDelegate("MqttClientPlugin", 2, "ShunkedPacket.");
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        void HandleUnSubscribeCommand(EndPointCommande cmd) {
            MqttV5::UnsubscribeTopic topic(cmd.topic);

            auto transcation = mqttClient->Unsubscribe(&topic);
            if (!transcation)
            {
                std::lock_guard<std::mutex> g(mutex);
                auto it = mqttPoints.find(cmd.sessionId);
                if (it != mqttPoints.end() && it->second->ws)
                {
                    Json::Value resp(Json::Value::Type::Object);
                    resp.Set("Type", "UnSubscribeResult");
                    resp.Set("Topic", cmd.topic);
                    resp.Set("Status", "Error");
                    resp.Set("Message", "UnSubscribe() returned null");
                    it->second->ws->SendText(resp.ToEncoding());
                }
                return;
            }

            transcation->SetCompletionDelegate(
                [this, cmd](std::vector<MqttV5::ReasonCode>& reasons)
                {
                    std::lock_guard<std::mutex> g(mutex);

                    auto it = mqttPoints.find(cmd.sessionId);
                    if (it == mqttPoints.end() || !it->second->ws)
                        return;
                    bool ok = !reasons.empty() && reasons.back() < 0x80;
                    if (ok)
                    { it->second->topics.push_back(cmd.topic); }
                    Json::Value resp(Json::Value::Type::Object);
                    resp.Set("Type", "UnSubscribeResult");
                    resp.Set("Topic", cmd.topic);
                    resp.Set("Status", ok ? "Success" : "Error");
                    it->second->ws->SendText(resp.ToEncoding());
                });
        }

        void JoinServer(unsigned int sessionId, const Json::Value& message) {
            auto it = mqttPoints.find(sessionId);
            if (it == mqttPoints.end() || !it->second->ws)
                return;
            Json::Value response(Json::Value::Type::Object);
            response.Set("Type", "JoinChatRoomResponse");
            response.Set("Success", true);
            Json::Value subscriptions(Json::Value::Type::Array);
            if (mqttClient && mqttConnected)
            {
                for (auto& endPoint : mqttPoints)
                {
                    for (auto& topic : endPoint.second->topics)
                    { subscriptions.Add(topic); }
                }
                response.Set("MqttStatus", "Connected");
                response.Set("Subscription", subscriptions);
            }
            it->second->ws->SendText(response.ToEncoding());
        }

        void PostSubscribeCommand(unsigned int sessionId, const Json::Value& message) {
            if (!mqttClient)
            {
                const auto diagnosticSenderName =
                    StringUtils::sprintf("Session #%zu : UnSubscribtion", sessionId);
                diagnosticsMessageDelegate(diagnosticSenderName,
                                           SystemUtils::DiagnosticsSender::Levels::ERROR,
                                           "mqtt client object is null");
                return;
            }

            const std::string topic = (std::string)message["Topic"];
            auto qos = MqttV5::QoSDelivery::AtLeastOne;
            if (message.Has("QoS"))
            {
                int q = (int)message["QoS"];
                if (q == 0)
                {
                    qos = MqttV5::QoSDelivery::AtMostOne;
                } else if (q == 2)
                { qos = MqttV5::QoSDelivery::ExactlyOne; }
            }

            EndPointCommande cmd;
            cmd.type = CommandeType::Subscribe;
            cmd.sessionId = sessionId;
            cmd.topic = topic;
            cmd.qos = qos;

            pendingCommandes.push(cmd);
            subscribeNewTopic = true;
            workerWakeCondition.notify_all();
        }

        void PostUnSubscribeCommand(unsigned int sessionId, const Json::Value& message) {
            if (!mqttClient)
            {
                const auto diagnosticSenderName =
                    StringUtils::sprintf("Session #%zu : UnSubscribtion", sessionId);
                diagnosticsMessageDelegate(diagnosticSenderName,
                                           SystemUtils::DiagnosticsSender::Levels::ERROR,
                                           "mqtt client object is null");
                return;
            }

            const std::string topic = (std::string)message["Topic"];
            auto qos = MqttV5::QoSDelivery::AtLeastOne;
            if (message.Has("QoS"))
            {
                int q = (int)message["QoS"];
                if (q == 0)
                {
                    qos = MqttV5::QoSDelivery::AtMostOne;
                } else if (q == 2)
                { qos = MqttV5::QoSDelivery::ExactlyOne; }
            }

            EndPointCommande cmd;
            cmd.type = CommandeType::Unsubscribe;
            cmd.sessionId = sessionId;
            cmd.topic = topic;
            cmd.qos = qos;

            pendingCommandes.push(cmd);
            subscribeNewTopic = true;
            workerWakeCondition.notify_all();
        }

        void CloseEndPoint(unsigned int sessionId, unsigned int code, const std::string& reason) {
            std::lock_guard<decltype(mutex)> lock(mutex);
            const auto endPoint = mqttPoints.find(sessionId);
            if (endPoint == mqttPoints.end())
            { return; }
            endPoint->second->ws->Close(code, reason);
            endPoint->second->connected = false;
            mqttPoints.erase(endPoint);
            endPointHaveClosed = true;
            workerWakeCondition.notify_all();
        }
        /**
         *
         */
        void ReceivedMessage(unsigned int sessionId, const std::string& data) {
            std::lock_guard<decltype(mutex)> lock(mutex);

            if (mqttPoints.find(sessionId) == mqttPoints.end())
            { return; }
            const auto message = Json::Value::FromEncoding(data);
            if (message["Type"] == "Subscribe" && message.Has("Topic"))
            {
                PostSubscribeCommand(sessionId, message);
            } else if (message["Type"] == "UnSubscribe" && message.Has("Topic"))
            {
                PostUnSubscribeCommand(sessionId, message);
            } else if (message["Type"] == "JoinServer")
            { JoinServer(sessionId, message); }
        }

        /**
         * This method is used whenever the mqtt client is tried to connect to
         * the broker over ws.
         *
         * @param[in] request
         *      This is the request to connect to broker.
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
        std::shared_ptr<Http::Client::Response> AddMqttPoint(
            std::shared_ptr<Http::IServer::Request> request,
            std::shared_ptr<Http::Connection> connection, const std::string& trailer) {
            const auto response = std::make_shared<Http::Client::Response>();
            const auto sessionId = nextSessionId++;
            auto mqttPoint = std::make_shared<MqttPoint>();
            mqttPoint->ws = std::make_unique<WebSocket::WebSocket>();
            mqttPoints.emplace(sessionId, mqttPoint);
            const auto diagnosticSenderName = StringUtils::sprintf("Session #%zu", sessionId);
            mqttPoint->diagnosticSenderName = diagnosticSenderName;
            mqttPoint->wsDiagnosticsUnsubscribeDelegate = mqttPoint->ws->SubscribeToDiagnostics(
                [this, diagnosticSenderName](std::string senderName, size_t level,
                                             std::string message)
                { diagnosticsMessageDelegate(diagnosticSenderName, level, message); });
            mqttPoint->ws->SetTextDelegate([this, sessionId](const std::string& data)
                                           { ReceivedMessage(sessionId, data); });
            mqttPoint->ws->SetCloseDelegate(
                [this, sessionId](unsigned int code, const std::string& reason)
                { CloseEndPoint(sessionId, code, reason); });
            if (!mqttPoint->ws->OpenAsServer(connection, *request, *response, trailer))
            {
                (void)mqttPoints.erase(sessionId);
                response->headers.SetHeader("Content-Type", "Text/plain");
                response->body = "Try again, but next time use a WebSocket. thxbye!";
            }
            return response;
        }
    } broker;

    void WsAppReceiver::onMessageReceived(const MqttV5::Storage::DynamicStringView topic,
                                          MqttV5::Storage::DynamicBinaryDataView payload,
                                          uint16_t packetId) {
        if (!broker)
        { return; }

        Json::Value msg(Json::Value::Type::Object);
        auto topicStr = std::string(topic.data, topic.size);
        msg.Set("Id", packetId);
        msg.Set("Type", "Publish");
        msg.Set("Topic", topicStr);
        msg.Set("Payload", std::string(reinterpret_cast<const char*>(payload.data), payload.size));
        const auto encoded = msg.ToEncoding();
        std::lock_guard<std::mutex> lock(broker->mutex);
        for (auto& it : broker->mqttPoints)
        {
            auto& endPoint = it.second;
            if (!endPoint || !endPoint->connected || !endPoint->ws)
            { continue; }

            // Check if a WS has a subscriber that match the topic
            bool match = false;
            for (const auto& filter : endPoint->topics)
            {
                if (topicMatchesfilter(filter, topicStr))
                {
                    match = true;
                    break;
                }
            }

            if (!match)
            { continue; }
            endPoint->ws->SendText(encoded);
        }
    }

    bool WsAppReceiver::onConnectionLost(const MqttV5::IMqttV5Client::Transaction::State& state) {
        if (!broker)
            return true;

        {
            std::lock_guard<std::mutex> lock(broker->mutex);
            broker->mqttConnected = false;
            broker->initialConnectPending = true;
        }
        broker->workerWakeCondition.notify_all();
        return true;
    }

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

    broker.diagnosticsMessageDelegate = diagnosticMessageDelegate;
    broker.Start(configuration);
    const auto unregistrationDelegate = server->RegisterResource(
        space, [](std::shared_ptr<Http::IServer::Request> request,
                  std::shared_ptr<Http::Connection> connection, const std::string& trailer)
        { return broker.AddMqttPoint(request, connection, trailer); });

    unloadDelegate = [unregistrationDelegate] { unregistrationDelegate(); };
}

namespace
{
    PluginEntryPoint EntryPoint = &LoadPlugin;
}  // namespace

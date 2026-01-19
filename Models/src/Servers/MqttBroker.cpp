#include <Models/Servers/MqttBroker.hpp>
#include <Utf8/Utf8.hpp>
namespace FalcataIoTServer
{
    struct MqttBroker::Impl
    {
        /**
         * This is the delegate obtained when subscribing
         * to receive diagnostic messages from the unit under test.
         * It's called to terminate the subscription.
         */
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticsMessageDelegate;

        /**
         *
         */
        std::weak_ptr<MqttV5::MqttClient> client;

        bool isReachable = false;

        bool useTLS = false;

        std::string userName;
        std::string password;

        bool cleanSession = true;

        bool willRetain = false;
        std::string willTopic;
        std::string willPayload;
        MqttV5::QoSDelivery qos = MqttV5::AtLeastOne;

        uint16_t reconnectPeriod = 1;
        uint16_t connectTimeOut = 30;

        uint16_t keepAlive = 10U;
        MqttV5::Properties* props = nullptr;

        MqttV5::WillMessage MakeWillMessage() {
            MqttV5::DynamicBinaryData will;
            Utf8::Utf8 utf;
            if (!willPayload.empty())
            {
                auto encodedWill = utf.Encode(Utf8::AsciiToUnicode(willPayload));
                will.data = encodedWill.data();
                will.size = static_cast<uint32_t>(encodedWill.size());
            }
            MqttV5::WillMessage willMsg;
            willMsg.topicName = willTopic.c_str();
            willMsg.payload = will;
            return willMsg;
        }

        MqttV5::DynamicBinaryData MakePassword() {
            MqttV5::DynamicBinaryData binaryPassword;
            Utf8::Utf8 utf;
            if (!password.empty())
            {
                auto encodedPass = utf.Encode(Utf8::AsciiToUnicode(password));
                binaryPassword.data = encodedPass.data();
                binaryPassword.size = static_cast<uint32_t>(encodedPass.size());
            }
            return binaryPassword;
        }
    };

    MqttBroker::MqttBroker() : Server(), broker_(std::make_unique<Impl>()) {}

    MqttBroker::MqttBroker(std::string id, std::string name, std::string host, uint16_t port,
                           std::string protocol, bool enabled, bool useTLS, std::string userName,
                           std::string password, bool cleanSession, bool willRetain,
                           std::string willTopic, std::string willPayload, MqttV5::QoSDelivery qos,
                           uint16_t keepAlive, MqttV5::Properties* props) :
        Server(std::move(id), std::move(name), std::move(host), std::move(port),
               std::move(protocol), std::move(enabled)),
        broker_(std::make_unique<Impl>()) {
        broker_->useTLS = useTLS;
        broker_->userName = userName;
        broker_->password = password;
        broker_->cleanSession = cleanSession;
        broker_->willRetain = willRetain;
        broker_->willTopic = willTopic;
        broker_->willPayload = willPayload;
        broker_->qos = qos;
        broker_->keepAlive = keepAlive;
        broker_->props = props;
    }

    MqttBroker::MqttBroker(std::string name, std::string host, uint16_t port, std::string protocol,
                           bool enabled, bool useTLS, std::string userName, std::string password,
                           bool cleanSession, bool willRetain, std::string willTopic,
                           std::string willPayload, MqttV5::QoSDelivery qos, uint16_t keepAlive,
                           MqttV5::Properties* props) :
        Server(std::move(name), std::move(host), std::move(port), std::move(protocol),
               std::move(enabled)),
        broker_(std::make_unique<Impl>()) {
        broker_->useTLS = useTLS;
        broker_->userName = userName;
        broker_->password = password;
        broker_->cleanSession = cleanSession;
        broker_->willRetain = willRetain;
        broker_->willTopic = willTopic;
        broker_->willPayload = willPayload;
        broker_->qos = qos;
        broker_->keepAlive = keepAlive;
        broker_->props = props;
    }
    MqttBroker::MqttBroker(std::string name, std::string host, uint16_t port, std::string protocol,
                           bool enabled) :
        Server(std::move(name), std::move(host), std::move(port), std::move(protocol),
               std::move(enabled)),
        broker_(std::make_unique<Impl>()) {}

    MqttBroker::~MqttBroker() noexcept = default;

    void MqttBroker::AttachClient(std::shared_ptr<MqttV5::MqttClient>& client) {
        if (broker_->client.lock())
        { return; }
        broker_->client = client;
    }

    std::shared_ptr<MqttV5::MqttClient::Transaction> MqttBroker::Start() {
        auto weakClient = broker_->client.lock();
        if (!weakClient)
        {
            if (broker_->diagnosticsMessageDelegate)
            {
                SetDiagnosticsMessageDelegate(SystemUtils::DiagnosticsSender::Levels::ERROR,
                                              std::string("Client is not properly mobilized!"));
            }
            return nullptr;
        }
        auto password = broker_->MakePassword();
        auto willMsg = broker_->MakeWillMessage();
        auto transaction = broker_->client.lock()->ConnectTo(
            GetId(), GetHost(), GetPort(), broker_->useTLS, broker_->cleanSession,
            broker_->keepAlive, broker_->userName.c_str(), &password, &willMsg, broker_->qos,
            broker_->willRetain, broker_->props);
        if (!transaction)
        {
            SetDiagnosticsMessageDelegate(
                SystemUtils::DiagnosticsSender::Levels::ERROR,
                std::string("ConnectTo() return null. Check transport/timekeeper/mobilize."));

            return nullptr;
        }
        transaction->SetCompletionDelegate(
            [this](std::vector<MqttV5::ReasonCode>& reasons)
            {
                bool ok =
                    !reasons.empty() && reasons.back() == MqttV5::Storage::ReasonCode::Success;
                broker_->isReachable = ok;
                SetDiagnosticsMessageDelegate(
                    ok ? SystemUtils::DiagnosticsSender::Levels::INFO
                       : SystemUtils::DiagnosticsSender::Levels::ERROR,
                    ok ? std::string("Mqtt client connected to the broker.")
                       : std::string("Mqtt client connection failed"));

                // TODO Create an event object to the DB.
                // TODO Update Enabled
            });
        return transaction;
    }
    std::shared_ptr<MqttV5::MqttClient::Transaction> MqttBroker::Stop() { return nullptr; }

    void MqttBroker::SetDiagnosticsMessageDelegate(SystemUtils::DiagnosticsSender::Levels level,
                                                   std::string& msg) {
        if (broker_->diagnosticsMessageDelegate)
        {
            broker_->diagnosticsMessageDelegate(StringUtils::sprintf("Broker #%s", Uuid_s()), level,
                                                msg);
        }
    }

    Json::Value MqttBroker::ToJson() const {
        Json::Value json = Server::ToJson();

        // DDL snake_case
        json.Set("use_tls", broker_->useTLS);
        // legacy
        json.Set("useTLS", broker_->useTLS);

        // credentials are handled by server_credentials in DB,
        // but keep them for API/legacy if needed
        json.Set("mqtt_userName", broker_->userName);
        json.Set("mqtt_password", broker_->password);

        // Put mqtt options into metadata (DDL)
        Json::Value md = GetMetadata();
        md.Set("cleanSession", broker_->cleanSession);
        md.Set("willRetain", broker_->willRetain);
        md.Set("willTopic", broker_->willTopic);
        md.Set("willPayload", broker_->willPayload);
        md.Set("qos", (int)broker_->qos);
        md.Set("keepAlive", (int)broker_->keepAlive);
        json.Set("metadata", md);

        // legacy flat keys (optional)
        json.Set("cleanSession", broker_->cleanSession);
        json.Set("willRetain", broker_->willRetain);
        json.Set("willTopic", broker_->willTopic);
        json.Set("willPayload", broker_->willPayload);
        json.Set("qos", (int)broker_->qos);
        json.Set("keepAlive", (int)broker_->keepAlive);

        return json;
    }

    void MqttBroker::FromJson(const Json::Value& json) {
        Server::FromJson(json);

        // accept snake_case + legacy
        if (json.Has("use_tls"))
            broker_->useTLS = (bool)json["use_tls"];
        if (json.Has("useTLS"))
            broker_->useTLS = (bool)json["useTLS"];

        if (json.Has("mqtt_userName"))
            broker_->userName = json["mqtt_userName"];
        if (json.Has("mqtt_password"))
            broker_->password = json["mqtt_password"];

        // Prefer metadata
        Json::Value md;
        if (json.Has("metadata"))
            md = json["metadata"];

        auto readBool = [&](const char* k, bool& dst)
        {
            if (md.Has(k))
                dst = (bool)md[k];
            else if (json.Has(k))
                dst = (bool)json[k];
        };
        auto readStr = [&](const char* k, std::string& dst)
        {
            if (md.Has(k))
                dst = md[k].ToEncoding();
            else if (json.Has(k))
                dst = json[k].ToEncoding();
        };
        auto readInt = [&](const char* k, int& dst)
        {
            if (md.Has(k))
                dst = (int)md[k];
            else if (json.Has(k))
                dst = (int)json[k];
        };

        readBool("cleanSession", broker_->cleanSession);
        readBool("willRetain", broker_->willRetain);
        readStr("willTopic", broker_->willTopic);
        readStr("willPayload", broker_->willPayload);

        int qos = (int)broker_->qos;
        readInt("qos", qos);
        broker_->qos = (MqttV5::QoSDelivery)qos;

        int keepAlive = (int)broker_->keepAlive;
        readInt("keepAlive", keepAlive);
        broker_->keepAlive = (uint16_t)keepAlive;
    }

}  // namespace FalcataIoTServer
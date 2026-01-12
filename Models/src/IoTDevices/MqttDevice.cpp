/**
 * @file MqttDevice.cpp
 */

#include <Models/IoTDevices/MqttDevice.hpp>

namespace FalcataIoTServer
{
    struct MqttDevice::Impl
    {
        std::set<std::shared_ptr<MqttTopic>> topics;
    };

    MqttDevice::MqttDevice() : IoTDevice(), mqttDevice_(std::make_unique<Impl>()) {}
    MqttDevice::MqttDevice(std::string id, std::string serverId, std::string name, std::string kind,
                           std::string protocol, bool enabled, std::string zoneId) :
        IoTDevice(id, serverId, name, kind, protocol, enabled, zoneId),
        mqttDevice_(std::make_unique<Impl>()) {}

    const std::set<std::shared_ptr<MqttTopic>> MqttDevice::GetTopics() const {
        return mqttDevice_->topics;
    }
    void MqttDevice::SetTopics(std::set<std::shared_ptr<MqttTopic>>& topics) {
        mqttDevice_->topics = topics;
    }
    void MqttDevice::AddTopic(std::shared_ptr<MqttTopic>& topic) {
        mqttDevice_->topics.insert(topic);
    }
    void MqttDevice::DeleteTopic(std::shared_ptr<MqttTopic>& topic) {
        mqttDevice_->topics.erase(topic);
    }

    // Sérialisation
    Json::Value MqttDevice::ToJson() const {
        Json::Value json(Json::Value::Type::Object);
        json = IoTDevice::ToJson();

        if (!mqttDevice_->topics.empty())
        {
            Json::Value arr(Json::Value::Type::Array);
            for (const auto& topic : mqttDevice_->topics)
            { arr.Add(topic->ToJson()); }
            json.Set("topics", arr);
        }
        return json;
    }

    void MqttDevice::FromJson(const Json::Value& json) {
        IoTDevice::FromJson(json);
        if (json.Has("topics") && (json["topics"].GetType() == Json::Value::Type::Array))
        {
            mqttDevice_->topics.clear();
            for (size_t i = 0; i < json["topics"].GetSize(); ++i)
            {
                auto j = json["topics"][i];
                auto topic = std::make_shared<MqttTopic>();
                topic->FromJson(j);
                mqttDevice_->topics.insert(topic);
            }
        }
    }
}  // namespace FalcataIoTServer

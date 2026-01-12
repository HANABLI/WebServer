/**
 * @file mqttTopic.cpp
 */
#include <Models/Data/MqttTopic.hpp>
#include <memory>
namespace FalcataIoTServer
{
    struct MqttTopic::Impl
    {
        std::string id;
        std::string deviceId;
        std::string role;
        std::string topic;
        MqttV5::QoSDelivery qos = MqttV5::QoSDelivery::AtLeastOne;
        MqttV5::RetainHandling retain = MqttV5::RetainHandling::NoRetainedMessage;
        bool autoFeedBack = false;
        bool retainAsPublished = false;
        bool local = false;
        std::string direction = "pubsub";
        bool enabled = true;
        Json::Value metadata = Json::Value(Json::Value::Type::Object);
        std::string createdAt;
        std::string updatedAt;
    };

    MqttTopic::MqttTopic() : CoreObject(), impl_(std::make_unique<Impl>()) {}
    MqttTopic::MqttTopic(const Json::Value& j) : CoreObject(), impl_(std::make_unique<Impl>()) {
        FromJson(j);
    }

    std::shared_ptr<MqttV5::SubscribeTopic> MqttTopic::BuildTopic() const {
        const auto topic = std::make_shared<MqttV5::SubscribeTopic>(
            impl_->topic, impl_->retain, impl_->retainAsPublished, impl_->local, impl_->qos);
        return topic;
    }

    std::shared_ptr<MqttV5::UnsubscribeTopic> MqttTopic::BuildUnsubTopic() const {
        const auto topic = std::make_shared<MqttV5::UnsubscribeTopic>(impl_->topic);
        return topic;
    }

    const std::string MqttTopic::GetId() const { return Uuid_s(); }
    void MqttTopic::SetId(const std::string& id) { UuidFromString(id); }

    const std::string MqttTopic::GetDeviceId() const { return impl_->deviceId; }
    void MqttTopic::SetDeviceId(const std::string& id) { impl_->deviceId = id; }

    const std::string MqttTopic::GetRole() const { return impl_->role; }
    void MqttTopic::SetRole(const std::string& role) { impl_->role = role; }

    const std::string MqttTopic::GetTopic() const { return impl_->topic; }
    void MqttTopic::SetTopic(const std::string& topic) { impl_->topic = topic; }

    MqttV5::QoSDelivery MqttTopic::GetQoS() const { return impl_->qos; }
    void MqttTopic::SetQoS(MqttV5::QoSDelivery qos) { impl_->qos = qos; }

    MqttV5::RetainHandling MqttTopic::GetRetain() const { return impl_->retain; }
    void MqttTopic::SetRetain(MqttV5::RetainHandling retain) { impl_->retain = retain; }

    bool MqttTopic::WithAutoFeedBack() const { return impl_->autoFeedBack; }
    void MqttTopic::SetWithAutoFeedBAck(bool v) { impl_->autoFeedBack = v; }
    bool MqttTopic::GetRetainAsPublished() const { return impl_->retainAsPublished; }
    void MqttTopic::SetRetainAsPublished(bool retainAsPublished) {
        impl_->retainAsPublished = retainAsPublished;
    }
    const std::string& MqttTopic::GetDirection() const { return impl_->direction; }
    void MqttTopic::SetDirection(const std::string& direction) { impl_->direction = direction; }

    bool MqttTopic::IsEnabled() const { return impl_->enabled; }
    void MqttTopic::SetEnabled(bool v) { impl_->enabled = v; }

    const Json::Value& MqttTopic::GetMetaData() const { return impl_->metadata; }
    void MqttTopic::SetMetaData(const Json::Value& v) { impl_->metadata = v; }

    const std::string& MqttTopic::GetCreatedAt() const { return impl_->createdAt; }
    void MqttTopic::SetCreatedAt(const std::string& v) { impl_->createdAt = v; }

    const std::string& MqttTopic::GetUpdatedAt() const { return impl_->updatedAt; }
    void MqttTopic::SetUpdatedAt(const std::string& v) { impl_->updatedAt = v; }

    Json::Value MqttTopic::ToJson() const {
        Json::Value j(Json::Value::Type::Object);
        j.Set("id", impl_->id.empty() ? Uuid_s() : impl_->id);
        j.Set("device_id", impl_->deviceId);
        j.Set("role", impl_->role);
        j.Set("topic", impl_->topic);
        j.Set("qos", static_cast<int>(impl_->qos));
        j.Set("retain", static_cast<int>(impl_->retain));
        j.Set("auto_feedback", impl_->autoFeedBack);
        j.Set("retain_as_published", impl_->retainAsPublished);
        j.Set("direction", impl_->direction);
        j.Set("enabled", impl_->enabled);
        j.Set("metadata", impl_->metadata);
        if (!impl_->createdAt.empty())
            j.Set("created_at", impl_->createdAt);
        if (!impl_->updatedAt.empty())
            j.Set("updated_at", impl_->updatedAt);
        return j;
    }

    void MqttTopic::FromJson(const Json::Value& j) {
        if (j.Has("id"))
        {
            impl_->id = j["id"].ToEncoding();
            UuidFromString(impl_->id);
        }
        if (j.Has("device_id"))
            impl_->deviceId = j["device_id"];
        if (j.Has("role"))
            impl_->role = j["role"];
        if (j.Has("topic"))
            impl_->topic = j["topic"];
        if (j.Has("qos"))
            impl_->qos = static_cast<MqttV5::QoSDelivery>(static_cast<int>(j["qos"]));
        if (j.Has("retain"))
            impl_->retain = static_cast<MqttV5::RetainHandling>(static_cast<int>(j["retain"]));
        if (j.Has("retain_as_published"))
            impl_->retainAsPublished = j["retain_as_published"];
        if (j.Has("auto_feedback"))
            impl_->autoFeedBack = j["auto_feedback"];
        if (j.Has("direction"))
            impl_->direction = j["direction"];
        if (j.Has("enabled"))
            impl_->enabled = (bool)j["enabled"];
        if (j.Has("metadata"))
            impl_->metadata = j["metadata"];
        if (j.Has("created_at"))
            impl_->createdAt = j["created_at"];
        if (j.Has("updated_at"))
            impl_->updatedAt = j["updated_at"];
    }

    const std::vector<std::string> MqttTopic::GetInsertParams() const { return {""}; }
    const std::vector<std::string> MqttTopic::GetUpdateParams() const { return {""}; }
    const std::vector<std::string> MqttTopic::GetRemoveParams() const { return {""}; }
    const std::vector<std::string> MqttTopic::GetDisableParams() const { return {""}; }
}  // namespace FalcataIoTServer
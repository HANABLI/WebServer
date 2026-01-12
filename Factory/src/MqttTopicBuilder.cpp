
#include <Factory/MqttTopicBuilder.hpp>

namespace FalcataIoTServer
{
    std::string MqttTopicBuilder::Discriminator(const Postgresql::PgResult& res, int row) {
        return res.TextRequired(row, "device_id");
    }

    std::unique_ptr<MqttTopicBuilder::Base> MqttTopicBuilder::Build(const std::string&,
                                                                    const Postgresql::PgResult& res,
                                                                    int row) {
        auto t = std::make_unique<MqttTopic>();
        t->SetId(res.TextRequired(row, "id"));
        t->SetDeviceId(res.TextRequired(row, "device_id"));
        t->SetRole(res.TextRequired(row, "role"));
        t->SetTopic(res.TextRequired(row, "topic"));
        t->SetQoS(static_cast<MqttV5::QoSDelivery>(res.Int(row, "qos", 1)));
        t->SetRetain(static_cast<MqttV5::RetainHandling>(res.Int(row, "retain", 1)));
        t->SetWithAutoFeedBAck(res.Bool(row, "auto_feedback", false));
        t->SetRetainAsPublished(res.Bool(row, "retain_as_published", false));
        t->SetDirection(res.Text(row, "direction", "pub"));
        t->SetEnabled(res.Bool(row, "enabled", true));
        t->SetMetaData(res.Json(row, "metadata", Json::Value::Type::Object));
        return t;
    }
}  // namespace FalcataIoTServer

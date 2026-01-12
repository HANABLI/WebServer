/**
 * @file TopicBuilder.hpp
 * @brief This is the definition of the FalcataIotServer::TopicBuilder
 * class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#pragma once

#include <Models/Data/MqttTopic.hpp>
#include <Factory/GenericFactory.hpp>

#include <memory>

namespace FalcataIoTServer
{
    class MqttTopicBuilder
    {
    public:
        using Base = MqttTopic;

    public:
        static std::string Discriminator(const Postgresql::PgResult& res, int row);

        static std::unique_ptr<Base> Build(const std::string& protocol,
                                           const Postgresql::PgResult& res, int row);
    };

    using MqttTopicFactory = GenericFactory<MqttTopicBuilder>;
}  // namespace FalcataIoTServer
#include <Models/Core/Server.hpp>
#include <Models/Core/Protocol.hpp>

namespace FalcataIoTServer
{
    Server::Server(const Protocol& protocol) : CoreObject(), impl_(std::make_unique<Impl>()) {
        impl_->protocol = protocol;
    }
    Server::Server(std::string id, std::string name, std::string host, uint16_t port,
                   std::string protocol, bool enabled) :
        CoreObject(std::move(id)), impl_(std::make_unique<Impl>()) {
        impl_->name = name;
        impl_->host = host;
        impl_->port = port;
        impl_->protocol = protocol;
        impl_->enabled = enabled;
    }
    Server::Server(std::string name, std::string host, uint16_t port, std::string protocol,
                   bool enabled) :
        CoreObject(), impl_(std::make_unique<Impl>()) {
        impl_->name = name;
        impl_->host = host;
        impl_->port = port;
        impl_->protocol = protocol;
        impl_->enabled = enabled;
    }

    // Host / Port communs à tous les serveurs
    const std::string Server::GetHost() const { return impl_->host; }
    void Server::SetHost(const std::string& host) { impl_->host = host; }

    uint16_t Server::GetPort() const { return impl_->port; }
    void Server::SetPort(uint16_t port) { impl_->port = port; }

    bool Server::GetUseTls() const { return impl_->useTls; }
    void Server::SetUseTls(bool v) { impl_->useTls = v; }

    const std::vector<std::string>& Server::GetTags() const { return impl_->tags; }
    void Server::SetTags(std::vector<std::string> v) { impl_->tags = std::move(v); }
    void Server::AddTag(const std::string& tag) { impl_->tags.push_back(tag); }

    const Json::Value& Server::GetMetadata() const { return impl_->metadata; }
    void Server::SetMetadata(const Json::Value& v) { impl_->metadata = v; }

    const std::string& Server::GetCreatedAt() const { return impl_->createdAt; }
    void Server::SetCreatedAt(const std::string& v) { impl_->createdAt = v; }

    const std::string& Server::GetUpdatedAt() const { return impl_->updatedAt; }
    void Server::SetUpdatedAt(const std::string& v) { impl_->updatedAt = v; }

    // std::set<std::shared_ptr<IoTDevice>> Server::GetIotDevices() const { return
    // impl_->ioTdevices; } void Server::SetIotDevices(std::set<std::shared_ptr<IoTDevice>>&
    // devices) {
    //     impl_->ioTdevices = devices;
    // }
    // void Server::AddIotDevice(std::shared_ptr<IoTDevice>& device) {
    //     impl_->ioTdevices.insert(device);
    // }
    // void Server::RemoveIotDevice(std::shared_ptr<IoTDevice>& device) {
    //     impl_->ioTdevices.erase(device);
    // }

    void Server::FromJson(const Json::Value& j) {
        Device<Server>::FromJson(j);

        if (j.Has("host"))
            impl_->host = j["host"];
        if (j.Has("port"))
            impl_->port = (uint16_t)(int)j["port"];

        // accept both snake_case + legacy
        if (j.Has("use_tls"))
            impl_->useTls = (bool)j["use_tls"];
        if (j.Has("useTLS"))
            impl_->useTls = (bool)j["useTLS"];

        if (j.Has("tags") && (j["tags"].GetType() == Json::Value::Type::Array))
        {
            impl_->tags.clear();
            for (size_t i = 0; i < j["tags"].GetSize(); ++i) impl_->tags.push_back(j["tags"][i]);
        }

        if (j.Has("metadata"))
            impl_->metadata = j["metadata"];
        if (j.Has("created_at"))
            impl_->createdAt = j["created_at"];
        if (j.Has("updated_at"))
            impl_->updatedAt = j["updated_at"];
    }

    Json::Value Server::ToJson() const {
        Json::Value j = Device<Server>::ToJson();
        j.Set("host", impl_->host);
        j.Set("port", (int)impl_->port);

        // DDL snake_case
        j.Set("use_tls", impl_->useTls);

        Json::Value tags(Json::Value::Type::Array);
        for (size_t i = 0; i < impl_->tags.size(); ++i)
        { tags.Add(impl_->tags.at(i)); }
        j.Set("tags", tags);

        j.Set("metadata", impl_->metadata);

        if (!impl_->createdAt.empty())
            j.Set("created_at", impl_->createdAt);
        if (!impl_->updatedAt.empty())
            j.Set("updated_at", impl_->updatedAt);

        return j;
    }

    const std::vector<std::string> Server::GetInsertParams() const { return {""}; }
    const std::vector<std::string> Server::GetUpdateParams() const { return {""}; }
    const std::vector<std::string> Server::GetRemoveParams() const { return {""}; }
    const std::vector<std::string> Server::GetDisableParams() const { return {""}; }
}  // namespace FalcataIoTServer
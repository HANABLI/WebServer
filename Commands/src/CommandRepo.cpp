#include <Commands/CommandRepo.hpp>

namespace FalcataIoTServer
{
    struct CommandRepo::Impl
    {
        std::shared_ptr<Postgresql::PgClient> pg;

        explicit Impl(std::shared_ptr<Postgresql::PgClient> pgClient) : pg(pgClient) {}
        ~Impl() noexcept = default;
    };

    CommandRepo::CommandRepo(std::shared_ptr<Postgresql::PgClient> pg) :
        impl_(std::make_unique<Impl>(pg)) {}

    void CommandRepo::Listen(const std::string& channel, const std::function<void()>& fn) {
        impl_->pg->Listen(channel, fn);
    }

    std::string CommandRepo::InsertPending(const std::string& deviceId, const std::string& command,
                                           const Json::Value& params) {
        const std::string sql =
            "INSERT INTO  iot.device_commands(device_id, command, params, status)"
            "VALUES($1::uuid, $2, $3::jsonb, 'pending')"
            "RETURNING id;";
        Postgresql::PgResult res(
            impl_->pg->ExecParams(sql, {deviceId, command, params.ToEncoding()}));
        if (res.Rows() != 1)
            return {};
        return res.TextRequired(0, "id");
    }

    std::vector<std::shared_ptr<Command>> CommandRepo::FetchPending(int limit) {
        const std::string sql =
            "SELECT * FROM commands WHERE status = 'pending' "
            "ORDER BY created_at ASC LIMIT " +
            std::to_string(limit) + ";";
        Postgresql::PgResult result(impl_->pg->Exec(sql));
        std::vector<std::shared_ptr<Command>> commands;
        commands.reserve(result.Rows());
        for (int i = 0; i < result.Rows(); ++i)
        {
            Json::Value cmdJson = result.Json(i, "command_data", Json::Value::Type::Object);
            auto cmd = std::make_shared<Command>();
            cmd->FromJson(cmdJson);
            commands.push_back(cmd);
        }
        // Implementation to fetch pending commands from the database
        return commands;
    }

    std::shared_ptr<Command> CommandRepo::GetById(const std::string& id) {
        const std::string sql = "SELECT * FROM commands WHERE id = $1;";
        Postgresql::PgResult result(impl_->pg->ExecParams(sql, std::vector<std::string>{id}));
        if (result.Rows() == 0)
        { return nullptr; }
        Json::Value cmdJson = result.Json(0, "command_data", Json::Value::Type::Object);
        auto cmd = std::make_shared<Command>();
        cmd->FromJson(cmdJson);
        return cmd;
    }

    void CommandRepo::MarkSent(const std::string& id) {
        const std::string sql =
            "UPDATE commands SET status = 'sent', sent_at = NOW() WHERE id = $1;";
        impl_->pg->ExecParams(sql, std::vector<std::string>{id});
    }

    void CommandRepo::MarkAcked(const std::string& id) {
        const std::string sql =
            "UPDATE commands SET status = 'acked', ack_at = NOW() "
            "WHERE id = $1;";
        impl_->pg->ExecParams(sql, std::vector<std::string>{id});
    }

    void CommandRepo::MarkFailed(const std::string& id, const std::string& err) {
        const std::string sql = "UPDATE commands SET status = 'failed', error = $2 WHERE id = $1;";
        impl_->pg->ExecParams(sql, std::vector<std::string>{id, err});
    }

    void CommandRepo::ScheduleRetry(const std::string& id, int attempts, int delaySec,
                                    const std::string& err) {
        const std::string sql =
            "UPDATE commands SET status = 'retry', attempts = $2, "
            "next_retry_at = NOW() + INTERVAL '" +
            std::to_string(delaySec) + " seconds', error = $3 WHERE id = $1;";
        impl_->pg->ExecParams(sql, std::vector<std::string>{id, std::to_string(attempts), err});
    }

}  // namespace FalcataIoTServer
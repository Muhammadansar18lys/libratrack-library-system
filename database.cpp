#include "libratrack/database.hpp"
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace libratrack {
namespace {
std::string envOr(const char* key, const char* fallback) {
    const char* value = std::getenv(key);
    return value == nullptr ? fallback : value;
}
}

Database::Database()
    : host_(envOr("LIBRATRACK_DB_HOST", "127.0.0.1")),
      port_(static_cast<unsigned>(std::stoul(envOr("LIBRATRACK_DB_PORT", "3306")))),
      user_(envOr("LIBRATRACK_DB_USER", "root")),
      password_(envOr("LIBRATRACK_DB_PASSWORD", "")),
      schema_(envOr("LIBRATRACK_DB_NAME", "libratrack")) {}

std::unique_ptr<sql::Connection> Database::connect() const {
    // Function-local static initialization makes the connector singleton thread-safe.
    static sql::Driver* driver = sql::mysql::get_driver_instance();
    auto connection = std::unique_ptr<sql::Connection>(
        driver->connect("tcp://" + host_ + ":" + std::to_string(port_), user_, password_));
    connection->setSchema(schema_);
    return connection;
}

DbRows Database::query(const std::string& statement,
                       const std::vector<std::string>& params) const {
    auto connection = connect();
    return query(*connection, statement, params);
}

DbRows Database::query(sql::Connection& connection, const std::string& statement,
                       const std::vector<std::string>& params) const {
    std::unique_ptr<sql::PreparedStatement> prepared(connection.prepareStatement(statement));
    for (std::size_t i = 0; i < params.size(); ++i)
        prepared->setString(static_cast<unsigned>(i + 1), params[i]);
    std::unique_ptr<sql::ResultSet> result(prepared->executeQuery());
    auto* metadata = result->getMetaData();
    DbRows rows;
    const auto columns = metadata->getColumnCount();
    while (result->next()) {
        DbRow row;
        for (unsigned column = 1; column <= columns; ++column)
            row[metadata->getColumnLabel(column)] = result->getString(column);
        rows.push_back(std::move(row));
    }
    return rows;
}

int Database::execute(sql::Connection& connection, const std::string& statement,
                      const std::vector<std::string>& params) const {
    std::unique_ptr<sql::PreparedStatement> prepared(connection.prepareStatement(statement));
    for (std::size_t i = 0; i < params.size(); ++i)
        prepared->setString(static_cast<unsigned>(i + 1), params[i]);
    return prepared->executeUpdate();
}

void Database::ping() const {
    const auto rows = query("SELECT 1 AS alive");
    if (rows.empty() || rows.front().at("alive") != "1")
        throw std::runtime_error("Database health check failed.");
}
}



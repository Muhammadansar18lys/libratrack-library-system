#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mysql/jdbc.h>

namespace libratrack {
using DbRow = std::unordered_map<std::string, std::string>;
using DbRows = std::vector<DbRow>;

class Database {
public:
    Database();
    std::unique_ptr<sql::Connection> connect() const;
    DbRows query(const std::string& statement, const std::vector<std::string>& params = {}) const;
    DbRows query(sql::Connection& connection, const std::string& statement,
                 const std::vector<std::string>& params = {}) const;
    int execute(sql::Connection& connection, const std::string& statement,
                const std::vector<std::string>& params = {}) const;
    void ping() const;
private:
    std::string host_;
    unsigned port_{};
    std::string user_;
    std::string password_;
    std::string schema_;
};
}

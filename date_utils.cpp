#include "libratrack/date_utils.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace libratrack::DateUtils {
namespace {
std::chrono::sys_days parseDate(const std::string& value) {
    if (value.size() < 10 || value[4] != '-' || value[7] != '-')
        throw std::invalid_argument("Date must use YYYY-MM-DD format.");
    const int year = std::stoi(value.substr(0, 4));
    const unsigned month = static_cast<unsigned>(std::stoi(value.substr(5, 2)));
    const unsigned day = static_cast<unsigned>(std::stoi(value.substr(8, 2)));
    const std::chrono::year_month_day date{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};
    if (!date.ok()) throw std::invalid_argument("Date is not valid.");
    return std::chrono::sys_days{date};
}
}

std::string todayPlusDays(int days) {
    const auto today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
    const auto due = std::chrono::year_month_day{today + std::chrono::days{days}};
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << int(due.year()) << '-'
        << std::setw(2) << unsigned(due.month()) << '-' << std::setw(2) << unsigned(due.day());
    return out.str();
}

int daysLate(const std::string& isoDueDate) {
    const auto due = parseDate(isoDueDate);
    const auto today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
    return std::max(0, static_cast<int>((today - due).count()));
}
}


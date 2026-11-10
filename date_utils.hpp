#pragma once
#include <string>

namespace libratrack::DateUtils {
std::string todayPlusDays(int days);
int daysLate(const std::string& isoDueDate);
}

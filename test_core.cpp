#include "libratrack/algorithms.hpp"
#include "libratrack/date_utils.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}
int main() {
    try {
        libratrack::Trie trie;
        trie.insert("Clean Code");
        trie.insert("Clean Architecture");
        trie.insert("Effective Java");
        require(trie.suggest("clean").size() == 2, "Trie should return both matching titles.");
        require(trie.suggest("EFF").size() == 1, "Trie prefix matching should ignore case.");
        require(trie.suggest("missing").empty(), "Trie should return no results for an unknown prefix.");

        std::vector<int> values{7, 2, 9, 2, -1, 4};
        libratrack::mergeSort(values, [](int left, int right) { return left < right; });
        require(values == std::vector<int>({-1, 2, 2, 4, 7, 9}), "Merge sort should order values and preserve duplicates.");

        const auto dueDate = libratrack::DateUtils::todayPlusDays(14);
        require(dueDate.size() == 10 && dueDate[4] == '-' && dueDate[7] == '-', "Due date should use YYYY-MM-DD format.");
        require(libratrack::DateUtils::daysLate(dueDate) == 0, "A future due date should not be overdue.");
        require(libratrack::DateUtils::daysLate("2000-01-01") > 0, "A past due date should report overdue days.");
        std::cout << "All LibraTrack core tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}


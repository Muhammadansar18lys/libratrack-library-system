#pragma once
#include "models.hpp"
#include <algorithm>
#include <cctype>
#include <memory>
#include <mutex>
#include <cstddef>
#include <utility>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace libratrack {
// Prefix trie for fast, case-insensitive title suggestions.
class Trie {
    struct Node {
        std::unordered_map<char, std::unique_ptr<Node>> children;
        std::string displayWord;
    };
    Node root_;
    mutable std::shared_mutex mutex_;
    static char fold(char ch) { return static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
    static void collect(const Node& node, std::vector<std::string>& words, std::size_t limit) {
        if (words.size() >= limit) return;
        if (!node.displayWord.empty()) words.push_back(node.displayWord);
        for (const auto& [ch, child] : node.children) {
            (void)ch;
            collect(*child, words, limit);
            if (words.size() >= limit) break;
        }
    }
public:
    void insert(const std::string& word) {
        if (word.empty()) return;
        std::unique_lock lock(mutex_);
        Node* node = &root_;
        for (char ch : word) {
            const char key = fold(ch);
            auto& next = node->children[key];
            if (!next) next = std::make_unique<Node>();
            node = next.get();
        }
        node->displayWord = word;
    }

    std::vector<std::string> suggest(const std::string& prefix, std::size_t limit = 8) const {
        std::shared_lock lock(mutex_);
        const Node* node = &root_;
        for (char ch : prefix) {
            auto found = node->children.find(fold(ch));
            if (found == node->children.end()) return {};
            node = found->second.get();
        }
        std::vector<std::string> result;
        collect(*node, result, limit);
        std::sort(result.begin(), result.end());
        return result;
    }
};

// Stable merge sort demonstrates divide-and-conquer ordering for book results.
template <typename T, typename Compare>
void mergeSort(std::vector<T>& values, Compare before) {
    if (values.size() < 2) return;
    const auto middle = values.size() / 2;
    std::vector<T> left(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(middle));
    std::vector<T> right(values.begin() + static_cast<std::ptrdiff_t>(middle), values.end());
    mergeSort(left, before);
    mergeSort(right, before);
    values.clear();
    values.reserve(left.size() + right.size());
    std::size_t i = 0, j = 0;
    while (i < left.size() && j < right.size()) {
        if (before(right[j], left[i])) values.push_back(std::move(right[j++]));
        else values.push_back(std::move(left[i++]));
    }
    while (i < left.size()) values.push_back(std::move(left[i++]));
    while (j < right.size()) values.push_back(std::move(right[j++]));
}

inline void sortBooksByTitle(std::vector<Book>& books) {
    mergeSort(books, [](const Book& a, const Book& b) {
        std::string left = a.title(), right = b.title();
        std::transform(left.begin(), left.end(), left.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        std::transform(right.begin(), right.end(), right.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return left == right ? a.id() < b.id() : left < right;
    });
}
}



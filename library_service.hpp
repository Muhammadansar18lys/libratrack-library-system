#pragma once
#include "database.hpp"
#include "models.hpp"
#include "algorithms.hpp"
#include <string>
#include <vector>

namespace libratrack {
class LibraryService {
public:
    explicit LibraryService(Database database);
    std::vector<Book> books(const std::string& query = "") const;
    std::vector<Member> members() const;
    std::vector<Loan> activeLoans() const;
    std::vector<Reservation> reservations(unsigned bookId) const;
    LibraryStats stats() const;
    std::vector<std::string> suggest(const std::string& prefix) const;
    Book addBook(const std::string& isbn, const std::string& title, const std::string& author,
                 int year, int copies);
    Member addMember(const std::string& fullName, const std::string& email);
    OperationResult borrow(unsigned memberId, unsigned bookId);
    OperationResult returnBook(unsigned memberId, unsigned bookId);
    void healthCheck() const;
private:
    Database database_;
    Trie titleIndex_;
    void rebuildTitleIndex();
};
}

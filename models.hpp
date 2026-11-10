#pragma once
#include <string>
#include <utility>

namespace libratrack {
class Book {
public:
    Book(unsigned id, std::string isbn, std::string title, std::string author,
         int publishedYear, int copiesTotal, int copiesAvailable)
        : id_(id), isbn_(std::move(isbn)), title_(std::move(title)), author_(std::move(author)),
          publishedYear_(publishedYear), copiesTotal_(copiesTotal), copiesAvailable_(copiesAvailable) {}
    unsigned id() const noexcept { return id_; }
    const std::string& isbn() const noexcept { return isbn_; }
    const std::string& title() const noexcept { return title_; }
    const std::string& author() const noexcept { return author_; }
    int publishedYear() const noexcept { return publishedYear_; }
    int copiesTotal() const noexcept { return copiesTotal_; }
    int copiesAvailable() const noexcept { return copiesAvailable_; }
private:
    unsigned id_;
    std::string isbn_;
    std::string title_;
    std::string author_;
    int publishedYear_;
    int copiesTotal_;
    int copiesAvailable_;
};

class Member {
public:
    Member(unsigned id, std::string code, std::string fullName, std::string email)
        : id_(id), code_(std::move(code)), fullName_(std::move(fullName)), email_(std::move(email)) {}
    unsigned id() const noexcept { return id_; }
    const std::string& code() const noexcept { return code_; }
    const std::string& fullName() const noexcept { return fullName_; }
    const std::string& email() const noexcept { return email_; }
private:
    unsigned id_;
    std::string code_;
    std::string fullName_;
    std::string email_;
};

// Query results kept as simple value records for the API response layer.
struct Loan {
    unsigned long long id{};
    unsigned bookId{};
    unsigned memberId{};
    std::string title;
    std::string memberName;
    std::string checkedOutAt;
    std::string dueDate;
    int daysLate{};
};
struct Reservation {
    unsigned long long id{};
    unsigned bookId{};
    unsigned memberId{};
    std::string bookTitle;
    std::string memberName;
    std::string createdAt;
};
struct LibraryStats {
    int titleCount{};
    int copyCount{};
    int memberCount{};
    int activeLoanCount{};
    int overdueCount{};
    int waitingCount{};
};
struct OperationResult {
    bool success{};
    int httpStatus{200};
    std::string message;
};
}

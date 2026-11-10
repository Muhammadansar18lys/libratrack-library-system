#include "libratrack/library_service.hpp"
#include "libratrack/date_utils.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace libratrack {
namespace {
std::string value(const DbRow& row, const std::string& key) {
    const auto found = row.find(key);
    return found == row.end() ? std::string{} : found->second;
}
int number(const DbRow& row, const std::string& key) {
    const auto text = value(row, key);
    return text.empty() ? 0 : std::stoi(text);
}
unsigned long long bigNumber(const DbRow& row, const std::string& key) {
    const auto text = value(row, key);
    return text.empty() ? 0ULL : std::stoull(text);
}
Book toBook(const DbRow& row) {
    return {static_cast<unsigned>(number(row, "book_id")), value(row, "isbn"), value(row, "title"),
            value(row, "author"), number(row, "published_year"), number(row, "copies_total"),
            number(row, "copies_available")};
}
Member toMember(const DbRow& row) {
    return {static_cast<unsigned>(number(row, "member_id")), value(row, "member_code"),
            value(row, "full_name"), value(row, "email")};
}
}

LibraryService::LibraryService(Database database) : database_(std::move(database)) {
    rebuildTitleIndex();
}

void LibraryService::rebuildTitleIndex() {
    for (const auto& book : books()) titleIndex_.insert(book.title());
}

std::vector<Book> LibraryService::books(const std::string& query) const {
    DbRows rows;
    if (query.empty()) {
        rows = database_.query("SELECT book_id,isbn,title,author,published_year,copies_total,copies_available FROM v_book_availability");
    } else {
        const auto pattern = "%" + query + "%";
        rows = database_.query("SELECT book_id,isbn,title,author,published_year,copies_total,copies_available FROM v_book_availability WHERE title LIKE ? OR author LIKE ? OR isbn LIKE ?",
                               {pattern, pattern, pattern});
    }
    std::vector<Book> result;
    result.reserve(rows.size());
    for (const auto& row : rows) result.push_back(toBook(row));
    sortBooksByTitle(result);
    return result;
}

std::vector<Member> LibraryService::members() const {
    const auto rows = database_.query("SELECT member_id,member_code,full_name,email FROM members WHERE is_active=TRUE ORDER BY full_name");
    std::vector<Member> result;
    result.reserve(rows.size());
    for (const auto& row : rows) result.push_back(toMember(row));
    return result;
}

std::vector<Loan> LibraryService::activeLoans() const {
    const auto rows = database_.query(
        "SELECT l.loan_id,l.book_id,l.member_id,b.title,m.full_name AS member_name,"
        "DATE_FORMAT(l.checked_out_at,'%Y-%m-%d %H:%i') AS checked_out_at,"
        "DATE_FORMAT(l.due_date,'%Y-%m-%d') AS due_date "
        "FROM loans l JOIN books b ON b.book_id=l.book_id JOIN members m ON m.member_id=l.member_id "
        "WHERE l.returned_at IS NULL ORDER BY l.due_date,l.loan_id");
    std::vector<Loan> result;
    result.reserve(rows.size());
    for (const auto& row : rows) {
        Loan loan{bigNumber(row,"loan_id"), static_cast<unsigned>(number(row,"book_id")),
                  static_cast<unsigned>(number(row,"member_id")), value(row,"title"),
                  value(row,"member_name"), value(row,"checked_out_at"), value(row,"due_date"), 0};
        loan.daysLate = DateUtils::daysLate(loan.dueDate);
        result.push_back(std::move(loan));
    }
    return result;
}

std::vector<Reservation> LibraryService::reservations(unsigned bookId) const {
    const auto rows = database_.query(
        "SELECT r.reservation_id,r.book_id,r.member_id,b.title AS book_title,m.full_name AS member_name,"
        "DATE_FORMAT(r.created_at,'%Y-%m-%d %H:%i') AS created_at "
        "FROM reservations r JOIN books b ON b.book_id=r.book_id JOIN members m ON m.member_id=r.member_id "
        "WHERE r.book_id=? AND r.status='waiting' ORDER BY r.created_at,r.reservation_id", {std::to_string(bookId)});
    std::vector<Reservation> result;
    for (const auto& row : rows)
        result.push_back({bigNumber(row,"reservation_id"), static_cast<unsigned>(number(row,"book_id")),
                          static_cast<unsigned>(number(row,"member_id")), value(row,"book_title"),
                          value(row,"member_name"), value(row,"created_at")});
    return result;
}

LibraryStats LibraryService::stats() const {
    const auto rows = database_.query("SELECT * FROM v_library_stats");
    if (rows.empty()) return {};
    const auto& row = rows.front();
    return {number(row,"title_count"), number(row,"copy_count"), number(row,"member_count"),
            number(row,"active_loan_count"), number(row,"overdue_count"), number(row,"waiting_count")};
}

std::vector<std::string> LibraryService::suggest(const std::string& prefix) const {
    return titleIndex_.suggest(prefix, 8);
}

Book LibraryService::addBook(const std::string& isbn, const std::string& title, const std::string& author,
                             int year, int copies) {
    if (isbn.empty() || isbn.size() > 20 || title.empty() || title.size() > 180 || author.empty() || author.size() > 140 || copies < 1 || copies > 500 || year < 0 || year > 2100)
        throw std::invalid_argument("Provide ISBN, title, author, a valid year, and 1-500 copies.");
    auto connection = database_.connect();
    database_.execute(*connection,
        "INSERT INTO books (isbn,title,author,published_year,copies_total) VALUES (?,?,?,?,?)",
        {isbn,title,author,std::to_string(year),std::to_string(copies)});
    const auto inserted = database_.query(*connection,"SELECT LAST_INSERT_ID() AS book_id");
    Book created(static_cast<unsigned>(std::stoul(value(inserted.front(),"book_id"))),isbn,title,author,year,copies,copies);
    titleIndex_.insert(title);
    return created;
}

Member LibraryService::addMember(const std::string& fullName, const std::string& email) {
    if (fullName.empty() || fullName.size() > 140 || email.empty() || email.size() > 180 || email.find('@') == std::string::npos)
        throw std::invalid_argument("Enter a name and a valid email address.");
    auto connection = database_.connect();
    database_.execute(*connection,
        "INSERT INTO members (member_code,full_name,email) VALUES (CONCAT('LIB-',UUID_SHORT()),?,?)",
        {fullName,email});
    const auto inserted = database_.query(*connection,
        "SELECT member_id,member_code,full_name,email FROM members WHERE member_id=LAST_INSERT_ID()");
    if (inserted.empty()) throw std::runtime_error("Could not read the new member record.");
    return toMember(inserted.front());
}

OperationResult LibraryService::borrow(unsigned memberId, unsigned bookId) {
    auto connection = database_.connect();
    connection->setAutoCommit(false);
    try {
        const auto member = database_.query(*connection,
            "SELECT member_id FROM members WHERE member_id=? AND is_active=TRUE FOR UPDATE", {std::to_string(memberId)});
        if (member.empty()) { connection->rollback(); return {false,404,"Active member was not found."}; }
        const auto book = database_.query(*connection,
            "SELECT book_id FROM books WHERE book_id=? FOR UPDATE", {std::to_string(bookId)});
        if (book.empty()) { connection->rollback(); return {false,404,"Book was not found."}; }
        const auto alreadyBorrowed = database_.query(*connection,
            "SELECT loan_id FROM loans WHERE member_id=? AND book_id=? AND returned_at IS NULL LIMIT 1",
            {std::to_string(memberId),std::to_string(bookId)});
        if (!alreadyBorrowed.empty()) { connection->rollback(); return {false,409,"This member already has an active loan for that book."}; }
        const auto count = database_.query(*connection,
            "SELECT COUNT(*) AS loan_count FROM loans WHERE member_id=? AND returned_at IS NULL", {std::to_string(memberId)});
        if (number(count.front(),"loan_count") >= 5) { connection->rollback(); return {false,409,"This member has reached the five-book loan limit."}; }
        const auto queued = database_.query(*connection,
            "SELECT reservation_id FROM reservations WHERE book_id=? AND status='waiting' ORDER BY created_at,reservation_id LIMIT 1 FOR UPDATE",
            {std::to_string(bookId)});
        const auto availability = database_.query(*connection,
            "SELECT copies_available FROM v_book_availability WHERE book_id=?", {std::to_string(bookId)});
        if (number(availability.front(),"copies_available") > 0 && queued.empty()) {
            database_.execute(*connection,
                "INSERT INTO loans (book_id,member_id,due_date) VALUES (?,?,?)",
                {std::to_string(bookId),std::to_string(memberId),DateUtils::todayPlusDays(14)});
            connection->commit();
            return {true,200,"Book checked out. It is due in 14 days."};
        }
        const auto duplicate = database_.query(*connection,
            "SELECT reservation_id FROM reservations WHERE book_id=? AND member_id=? AND status='waiting' LIMIT 1",
            {std::to_string(bookId),std::to_string(memberId)});
        if (!duplicate.empty()) { connection->rollback(); return {false,409,"This member is already on the waiting list."}; }
        database_.execute(*connection,
            "INSERT INTO reservations (book_id,member_id,status) VALUES (?,?,'waiting')",
            {std::to_string(bookId),std::to_string(memberId)});
        connection->commit();
        return {true,201,"No copy is available. Member added to the FIFO waiting list."};
    } catch (...) {
        connection->rollback();
        throw;
    }
}

OperationResult LibraryService::returnBook(unsigned memberId, unsigned bookId) {
    auto connection = database_.connect();
    connection->setAutoCommit(false);
    try {
        const auto bookLock = database_.query(*connection,"SELECT book_id FROM books WHERE book_id=? FOR UPDATE",{std::to_string(bookId)});
        if (bookLock.empty()) { connection->rollback(); return {false,404,"Book was not found."}; }
        const auto loan = database_.query(*connection,
            "SELECT loan_id FROM loans WHERE member_id=? AND book_id=? AND returned_at IS NULL ORDER BY loan_id LIMIT 1 FOR UPDATE",
            {std::to_string(memberId),std::to_string(bookId)});
        if (loan.empty()) { connection->rollback(); return {false,404,"No active loan was found for that member and book."}; }
        const auto loanId = value(loan.front(),"loan_id");
        database_.execute(*connection,"UPDATE loans SET returned_at=CURRENT_TIMESTAMP WHERE loan_id=?",{loanId});
        std::string message = "Book returned successfully.";
        while (true) {
            const auto next = database_.query(*connection,
                "SELECT reservation_id,member_id FROM reservations WHERE book_id=? AND status='waiting' ORDER BY created_at,reservation_id LIMIT 1 FOR UPDATE",
                {std::to_string(bookId)});
            if (next.empty()) break;
            const auto reservationId = value(next.front(),"reservation_id");
            const auto nextMemberId = value(next.front(),"member_id");
            const auto activeCount = database_.query(*connection,
                "SELECT COUNT(*) AS loan_count FROM loans WHERE member_id=? AND returned_at IS NULL",{nextMemberId});
            const auto duplicateLoan = database_.query(*connection,
                "SELECT loan_id FROM loans WHERE member_id=? AND book_id=? AND returned_at IS NULL LIMIT 1",
                {nextMemberId,std::to_string(bookId)});
            const auto activeMember = database_.query(*connection,
                "SELECT member_id FROM members WHERE member_id=? AND is_active=TRUE",{nextMemberId});
            if (number(activeCount.front(),"loan_count") >= 5 || !duplicateLoan.empty() || activeMember.empty()) {
                database_.execute(*connection,"UPDATE reservations SET status='cancelled' WHERE reservation_id=?",{reservationId});
                continue;
            }
            database_.execute(*connection,
                "INSERT INTO loans (book_id,member_id,due_date) VALUES (?,?,?)",
                {std::to_string(bookId),nextMemberId,DateUtils::todayPlusDays(14)});
            database_.execute(*connection,
                "UPDATE reservations SET status='fulfilled',fulfilled_at=CURRENT_TIMESTAMP WHERE reservation_id=?",
                {reservationId});
            message = "Book returned and automatically checked out to the next member on the waiting list.";
            break;
        }
        connection->commit();
        return {true,200,message};
    } catch (...) {
        connection->rollback();
        throw;
    }
}

void LibraryService::healthCheck() const { database_.ping(); }
}





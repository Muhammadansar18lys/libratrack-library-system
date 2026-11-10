#include "libratrack/library_service.hpp"
#include <crow.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <stdexcept>
#include <utility>

namespace {
using namespace libratrack;

crow::response jsonResponse(int status, crow::json::wvalue body) {
    crow::response response(status, body.dump());
    response.set_header("Content-Type", "application/json; charset=utf-8");
    return response;
}
crow::response errorResponse(int status, const std::string& message) {
    crow::json::wvalue body;
    body["success"] = false;
    body["message"] = message;
    return jsonResponse(status, std::move(body));
}
crow::response resultResponse(const OperationResult& result) {
    crow::json::wvalue body;
    body["success"] = result.success;
    body["message"] = result.message;
    return jsonResponse(result.httpStatus, std::move(body));
}
template <typename F>
crow::response safely(F&& action) {
    try {
        return action();
    } catch (const std::invalid_argument& error) {
        return errorResponse(400, error.what());
    } catch (const sql::SQLException& error) {
        if (error.getErrorCode() == 1062) return errorResponse(409, "That unique value is already in use.");
        CROW_LOG_ERROR << "Database request failed: " << error.what();
        return errorResponse(500, "The database request failed. Check the API log for details.");
    } catch (const std::exception& error) {
        CROW_LOG_ERROR << "Request failed: " << error.what();
        return errorResponse(500, "The request could not be completed. Check the API log for details.");
    }
}
bool hasString(const crow::json::rvalue& body, const char* key) {
    return body.has(key) && body[key].t() == crow::json::type::String && !body[key].s().empty();
}
crow::json::wvalue bookJson(const Book& book) {
    crow::json::wvalue json;
    json["id"] = book.id();
    json["isbn"] = book.isbn();
    json["title"] = book.title();
    json["author"] = book.author();
    json["publishedYear"] = book.publishedYear();
    json["copiesTotal"] = book.copiesTotal();
    json["copiesAvailable"] = book.copiesAvailable();
    return json;
}
crow::json::wvalue memberJson(const Member& member) {
    crow::json::wvalue json;
    json["id"] = member.id();
    json["code"] = member.code();
    json["fullName"] = member.fullName();
    json["email"] = member.email();
    return json;
}
crow::response serveFile(const std::filesystem::path& root, const std::string& file, const std::string& type) {
    std::ifstream input(root / file, std::ios::binary);
    if (!input) return crow::response(404, "File not found");
    std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    crow::response response(200, std::move(contents));
    response.set_header("Content-Type", type);
    return response;
}
}

int main() {
    try {
        const auto webRoot = std::filesystem::path(std::getenv("LIBRATRACK_WEB_ROOT") ? std::getenv("LIBRATRACK_WEB_ROOT") : "frontend");
        const auto portText = std::getenv("LIBRATRACK_PORT") ? std::getenv("LIBRATRACK_PORT") : "18080";
        const auto port = static_cast<unsigned short>(std::stoul(portText));
        LibraryService library(Database{});
        library.healthCheck();
        crow::SimpleApp app;

        CROW_ROUTE(app, "/")([webRoot] { return serveFile(webRoot, "index.html", "text/html; charset=utf-8"); });
        CROW_ROUTE(app, "/styles.css")([webRoot] { return serveFile(webRoot, "styles.css", "text/css; charset=utf-8"); });
        CROW_ROUTE(app, "/app.js")([webRoot] { return serveFile(webRoot, "app.js", "text/javascript; charset=utf-8"); });

        CROW_ROUTE(app, "/api/health").methods(crow::HTTPMethod::GET)([&library] {
            return safely([&] { library.healthCheck(); crow::json::wvalue body; body["success"] = true; body["database"] = "connected"; return jsonResponse(200, std::move(body)); });
        });
        CROW_ROUTE(app, "/api/stats").methods(crow::HTTPMethod::GET)([&library] {
            return safely([&] {
                const auto stats = library.stats(); crow::json::wvalue body;
                body["titles"] = stats.titleCount; body["copies"] = stats.copyCount;
                body["members"] = stats.memberCount; body["activeLoans"] = stats.activeLoanCount;
                body["overdue"] = stats.overdueCount; body["waiting"] = stats.waitingCount;
                return jsonResponse(200, std::move(body));
            });
        });
        CROW_ROUTE(app, "/api/books").methods(crow::HTTPMethod::GET)([&library](const crow::request& request) {
            return safely([&] {
                const auto query = request.url_params.get("q");
                const auto books = library.books(query ? query : "");
                crow::json::wvalue::list list;
                for (const auto& book : books) list.emplace_back(bookJson(book));
                crow::json::wvalue body; body["books"] = crow::json::wvalue(list);
                return jsonResponse(200, std::move(body));
            });
        });
        CROW_ROUTE(app, "/api/books/suggest").methods(crow::HTTPMethod::GET)([&library](const crow::request& request) {
            const auto prefix = request.url_params.get("q");
            const auto suggestions = library.suggest(prefix ? prefix : "");
            crow::json::wvalue::list list;
            for (const auto& title : suggestions) list.emplace_back(title);
            crow::json::wvalue body; body["suggestions"] = std::move(list);
            return jsonResponse(200, std::move(body));
        });
        CROW_ROUTE(app, "/api/books").methods(crow::HTTPMethod::POST)([&library](const crow::request& request) {
            return safely([&] {
                const auto body = crow::json::load(request.body);
                if (!body || !hasString(body,"isbn") || !hasString(body,"title") || !hasString(body,"author") || !body.has("copies"))
                    return errorResponse(400,"Provide isbn, title, author, and copies.");
                const int year = body.has("year") ? body["year"].i() : 0;
                const auto book = library.addBook(body["isbn"].s(),body["title"].s(),body["author"].s(),year,body["copies"].i());
                crow::json::wvalue result; result["success"] = true; result["book"] = bookJson(book);
                return jsonResponse(201,std::move(result));
            });
        });
        CROW_ROUTE(app, "/api/members").methods(crow::HTTPMethod::GET)([&library] {
            return safely([&] {
                crow::json::wvalue::list list;
                for (const auto& member : library.members()) list.emplace_back(memberJson(member));
                crow::json::wvalue body; body["members"] = crow::json::wvalue(list);
                return jsonResponse(200,std::move(body));
            });
        });
        CROW_ROUTE(app, "/api/members").methods(crow::HTTPMethod::POST)([&library](const crow::request& request) {
            return safely([&] {
                const auto body = crow::json::load(request.body);
                if (!body || !hasString(body,"fullName") || !hasString(body,"email"))
                    return errorResponse(400,"Provide fullName and email.");
                const auto member = library.addMember(body["fullName"].s(),body["email"].s());
                crow::json::wvalue result; result["success"] = true; result["member"] = memberJson(member);
                return jsonResponse(201,std::move(result));
            });
        });
        CROW_ROUTE(app, "/api/loans").methods(crow::HTTPMethod::GET)([&library] {
            return safely([&] {
                crow::json::wvalue::list list;
                for (const auto& loan : library.activeLoans()) {
                    crow::json::wvalue item; item["id"] = loan.id; item["bookId"] = loan.bookId;
                    item["memberId"] = loan.memberId; item["title"] = loan.title; item["memberName"] = loan.memberName;
                    item["checkedOutAt"] = loan.checkedOutAt; item["dueDate"] = loan.dueDate; item["daysLate"] = loan.daysLate;
                    list.emplace_back(std::move(item));
                }
                crow::json::wvalue body; body["loans"] = crow::json::wvalue(list);
                return jsonResponse(200,std::move(body));
            });
        });
        CROW_ROUTE(app, "/api/loans/borrow").methods(crow::HTTPMethod::POST)([&library](const crow::request& request) {
            return safely([&] {
                const auto body = crow::json::load(request.body);
                if (!body || !body.has("memberId") || !body.has("bookId")) return errorResponse(400,"Provide memberId and bookId.");
                const auto result = library.borrow(static_cast<unsigned>(body["memberId"].u()),static_cast<unsigned>(body["bookId"].u()));
                return resultResponse(result);
            });
        });
        CROW_ROUTE(app, "/api/loans/return").methods(crow::HTTPMethod::POST)([&library](const crow::request& request) {
            return safely([&] {
                const auto body = crow::json::load(request.body);
                if (!body || !body.has("memberId") || !body.has("bookId")) return errorResponse(400,"Provide memberId and bookId.");
                return resultResponse(library.returnBook(static_cast<unsigned>(body["memberId"].u()),static_cast<unsigned>(body["bookId"].u())));
            });
        });
        CROW_ROUTE(app, "/api/reservations").methods(crow::HTTPMethod::GET)([&library](const crow::request& request) {
            return safely([&] {
                const auto id = request.url_params.get("bookId");
                if (!id) return errorResponse(400,"Provide a bookId query parameter.");
                const auto reservations = library.reservations(static_cast<unsigned>(std::stoul(id)));
                crow::json::wvalue::list list;
                for (const auto& reservation : reservations) {
                    crow::json::wvalue item; item["id"] = reservation.id; item["memberId"] = reservation.memberId;
                    item["memberName"] = reservation.memberName; item["createdAt"] = reservation.createdAt;
                    list.emplace_back(std::move(item));
                }
                crow::json::wvalue body; body["reservations"] = crow::json::wvalue(list);
                return jsonResponse(200,std::move(body));
            });
        });

        CROW_LOG_INFO << "LibraTrack is running at http://localhost:" << port;
        app.port(port).multithreaded().run();
    } catch (const std::exception& error) {
        std::cerr << "LibraTrack startup failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}





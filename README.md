# LibraTrack - Smart Library Management System

A full-stack learning project that combines a C++20 REST API, MySQL, and a vanilla HTML/CSS/JavaScript dashboard. It demonstrates object-oriented design, a prefix trie, stable merge sort, prepared SQL statements, transactions, and FIFO reservation promotion.

## Features

- Search and browse books; title-prefix suggestions use a trie
- Add books and register library members
- Check out books or join their FIFO reservation queue
- Return books and automatically issue an available copy to the next eligible member
- Enforce a five-active-loan limit and calculate overdue days
- View current loans, waitlists, copy availability, and library statistics
- Persist data in MySQL and serve the dashboard and API from the same C++ server

## Stack and design

- **C++20:** Crow web framework, service layer, date helpers, models, and DSA utilities
- **MySQL 8.4:** books, members, loans, reservations, availability and statistics views
- **HTML/CSS/JavaScript:** responsive dashboard; no frontend build process
- **DSA:** trie for prefix completion, FIFO queue order in database rows, merge sort for catalog results, hash maps for indexes

The database layer uses MySQL Connector/C++'s classic JDBC API. Queries use prepared statements, and request values are bound as parameters.

## Requirements

- MySQL Server 8.4 or Docker Desktop
- C++20 compiler and CMake 3.24+
- vcpkg for Crow and MySQL Connector/C++ dependencies

## 1. Start MySQL

From the project folder:

```powershell
docker compose up -d mysql
```

On a fresh database volume, Docker runs `sql/01_schema.sql` and then `sql/02_seed.sql`. Local development connection: `127.0.0.1:3306`, user `root`, password `localdevpass`, database `libratrack`. The example password is for local learning only.

If you use MySQL Workbench instead, open and run the schema file first, then the seed file.

## 2. Install dependencies and build (Windows)

Install vcpkg and set `VCPKG_ROOT` to its folder. Use Visual Studio with the C++ desktop workload.

```powershell
$env:VCPKG_ROOT = "C:\src\vcpkg"
cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static `
  -DVCPKG_OVERLAY_TRIPLETS="$PWD/triplets"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Vcpkg reads `vcpkg.json` and installs Crow and the MySQL Connector/C++ JDBC feature during CMake configuration.

## 3. Run

The sample Docker credentials are defaults. Set a password for the database account before launching:

```powershell
$env:LIBRATRACK_DB_PASSWORD = "localdevpass"
.\build\Release\libratrack-api.exe
```

Optional environment variables: `LIBRATRACK_DB_HOST` (127.0.0.1), `LIBRATRACK_DB_PORT` (3306), `LIBRATRACK_DB_USER` (root), `LIBRATRACK_DB_PASSWORD` (empty), `LIBRATRACK_DB_NAME` (libratrack), and `LIBRATRACK_PORT` (18080).

Open <http://localhost:18080>. The C++ server hosts both the API and dashboard, so browser requests are same-origin.

## API routes

| Method | Route | Purpose |
| --- | --- | --- |
| `GET` | `/api/health` | Check API and database connection |
| `GET` | `/api/stats` | Read title, copy, member, loan, overdue, and queue counts |
| `GET` | `/api/books?q=clean` | Search the catalog |
| `GET` | `/api/books/suggest?q=cle` | Get title-prefix suggestions from the trie |
| `POST` | `/api/books` | Add a book |
| `GET` | `/api/members` | List active members |
| `POST` | `/api/members` | Register a member |
| `GET` | `/api/loans` | List active loans |
| `POST` | `/api/loans/borrow` | Borrow a copy or join the waitlist |
| `POST` | `/api/loans/return` | Return a book and promote the next eligible reservation |
| `GET` | `/api/reservations?bookId=1` | View a book's FIFO waiting list |

Checkout request example: `{"memberId":1,"bookId":1}`.

## Tests

The CTest target checks trie prefix matching and case handling, merge-sort ordering, and date calculations without a database connection. A GitHub Actions workflow runs these core tests on pushes and pull requests. The CMake and CTest commands above run the same suite locally.

## Project structure

```text
backend/include/libratrack/   Models, DSA helpers, and service/database interfaces
backend/src/                  Crow API, MySQL connector, library service, date implementation
frontend/                     Dashboard HTML, responsive CSS, browser JavaScript
sql/                          MySQL schema and sample seed records
tests/                        Core C++ unit tests
triplets/                     vcpkg static Windows triplet
```

## Security and scope

This is a local portfolio/learning application. It has no login or role-based authorization, so do not expose it publicly without adding authentication, authorization, HTTPS, and production-grade database credentials.

## References

- [Crow C++ web framework](https://github.com/CrowCpp/Crow)
- [MySQL Connector/C++ usage](https://dev.mysql.com/doc/dev/connector-cpp/latest/usage.html)
- [MySQL Connector/C++ JDBC example](https://dev.mysql.com/doc/dev/connector-cpp/latest/jdbc_example.html)
- [vcpkg quickstart](https://learn.microsoft.com/vcpkg/get_started/get-started)

## GitHub details

**Repository name:** `libratrack-library-system`

**Description:** `A smart library management system using a C++20 REST API, MySQL, and a vanilla JavaScript dashboard.`

**Commit title:** `Add LibraTrack library management system`

**Commit description:** `Build a C++ REST API and MySQL library system with book search, member management, loans, FIFO reservations, and a responsive web dashboard.`




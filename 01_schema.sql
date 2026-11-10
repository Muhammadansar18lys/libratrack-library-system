CREATE DATABASE IF NOT EXISTS libratrack CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;
USE libratrack;

CREATE TABLE IF NOT EXISTS books (
  book_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  isbn VARCHAR(20) NOT NULL UNIQUE,
  title VARCHAR(180) NOT NULL,
  author VARCHAR(140) NOT NULL,
  published_year SMALLINT UNSIGNED NULL,
  copies_total SMALLINT UNSIGNED NOT NULL DEFAULT 1,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT chk_books_copies CHECK (copies_total > 0),
  INDEX idx_books_title (title),
  INDEX idx_books_author (author)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS members (
  member_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  member_code VARCHAR(32) NOT NULL UNIQUE,
  full_name VARCHAR(140) NOT NULL,
  email VARCHAR(180) NOT NULL UNIQUE,
  joined_at DATE NOT NULL DEFAULT (CURRENT_DATE),
  is_active BOOLEAN NOT NULL DEFAULT TRUE,
  INDEX idx_members_name (full_name)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS loans (
  loan_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  book_id INT UNSIGNED NOT NULL,
  member_id INT UNSIGNED NOT NULL,
  checked_out_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  due_date DATE NOT NULL,
  returned_at DATETIME NULL,
  CONSTRAINT fk_loans_book FOREIGN KEY (book_id) REFERENCES books(book_id)
    ON UPDATE CASCADE ON DELETE RESTRICT,
  CONSTRAINT fk_loans_member FOREIGN KEY (member_id) REFERENCES members(member_id)
    ON UPDATE CASCADE ON DELETE RESTRICT,
  INDEX idx_loans_active_book (book_id, returned_at),
  INDEX idx_loans_active_member (member_id, returned_at),
  INDEX idx_loans_due (due_date, returned_at)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS reservations (
  reservation_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  book_id INT UNSIGNED NOT NULL,
  member_id INT UNSIGNED NOT NULL,
  status ENUM('waiting','fulfilled','cancelled') NOT NULL DEFAULT 'waiting',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  fulfilled_at DATETIME NULL,
  CONSTRAINT fk_reservations_book FOREIGN KEY (book_id) REFERENCES books(book_id)
    ON UPDATE CASCADE ON DELETE RESTRICT,
  CONSTRAINT fk_reservations_member FOREIGN KEY (member_id) REFERENCES members(member_id)
    ON UPDATE CASCADE ON DELETE RESTRICT,
  INDEX idx_reservations_fifo (book_id, status, created_at, reservation_id),
  INDEX idx_reservations_member (member_id, status)
) ENGINE=InnoDB;

CREATE OR REPLACE VIEW v_book_availability AS
SELECT b.book_id, b.isbn, b.title, b.author, b.published_year,
       b.copies_total,
       GREATEST(b.copies_total - COUNT(l.loan_id), 0) AS copies_available
FROM books AS b
LEFT JOIN loans AS l ON l.book_id = b.book_id AND l.returned_at IS NULL
GROUP BY b.book_id, b.isbn, b.title, b.author, b.published_year, b.copies_total;

CREATE OR REPLACE VIEW v_library_stats AS
SELECT (SELECT COUNT(*) FROM books) AS title_count,
       (SELECT COALESCE(SUM(copies_total), 0) FROM books) AS copy_count,
       (SELECT COUNT(*) FROM members WHERE is_active = TRUE) AS member_count,
       (SELECT COUNT(*) FROM loans WHERE returned_at IS NULL) AS active_loan_count,
       (SELECT COUNT(*) FROM loans WHERE returned_at IS NULL AND due_date < CURRENT_DATE) AS overdue_count,
       (SELECT COUNT(*) FROM reservations WHERE status = 'waiting') AS waiting_count;

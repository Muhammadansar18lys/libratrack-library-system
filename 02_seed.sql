USE libratrack;

INSERT INTO books (book_id, isbn, title, author, published_year, copies_total) VALUES
(1, '9780132350884', 'Clean Code', 'Robert C. Martin', 2008, 3),
(2, '9780201633610', 'Design Patterns', 'Erich Gamma, Richard Helm, Ralph Johnson, John Vlissides', 1994, 2),
(3, '9780134685991', 'Effective Java', 'Joshua Bloch', 2017, 2),
(4, '9781492056355', 'Fluent Python', 'Luciano Ramalho', 2022, 2),
(5, '9780262033848', 'Introduction to Algorithms', 'Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, Clifford Stein', 2022, 1),
(6, '9780131103627', 'The C Programming Language', 'Brian W. Kernighan, Dennis M. Ritchie', 1988, 2),
(7, '9781617294945', 'Grokking Algorithms', 'Aditya Bhargava', 2016, 3),
(8, '9780596007126', 'Head First Design Patterns', 'Eric Freeman, Elisabeth Robson', 2020, 1)
ON DUPLICATE KEY UPDATE title=VALUES(title), author=VALUES(author), published_year=VALUES(published_year), copies_total=VALUES(copies_total);

INSERT INTO members (member_id, member_code, full_name, email, joined_at, is_active) VALUES
(1, 'LIB-1001', 'Amina Yusuf', 'amina@example.test', CURRENT_DATE - INTERVAL 90 DAY, TRUE),
(2, 'LIB-1002', 'Bilal Ahmed', 'bilal@example.test', CURRENT_DATE - INTERVAL 60 DAY, TRUE),
(3, 'LIB-1003', 'Sara Khan', 'sara@example.test', CURRENT_DATE - INTERVAL 30 DAY, TRUE),
(4, 'LIB-1004', 'Omar Ali', 'omar@example.test', CURRENT_DATE - INTERVAL 10 DAY, TRUE)
ON DUPLICATE KEY UPDATE full_name=VALUES(full_name), email=VALUES(email), is_active=VALUES(is_active);

-- A few current loans and returns make the dashboard useful on first launch.
INSERT INTO loans (loan_id, book_id, member_id, checked_out_at, due_date, returned_at) VALUES
(1, 1, 1, CURRENT_TIMESTAMP - INTERVAL 3 DAY, CURRENT_DATE + INTERVAL 11 DAY, NULL),
(2, 1, 2, CURRENT_TIMESTAMP - INTERVAL 12 DAY, CURRENT_DATE - INTERVAL 2 DAY, NULL),
(3, 3, 3, CURRENT_TIMESTAMP - INTERVAL 5 DAY, CURRENT_DATE + INTERVAL 9 DAY, NULL),
(4, 7, 4, CURRENT_TIMESTAMP - INTERVAL 20 DAY, CURRENT_DATE - INTERVAL 6 DAY, NULL),
(5, 2, 1, CURRENT_TIMESTAMP - INTERVAL 30 DAY, CURRENT_DATE - INTERVAL 16 DAY, CURRENT_TIMESTAMP - INTERVAL 5 DAY)
ON DUPLICATE KEY UPDATE due_date=VALUES(due_date), returned_at=VALUES(returned_at);

INSERT INTO reservations (reservation_id, book_id, member_id, status, created_at) VALUES
(1, 1, 3, 'waiting', CURRENT_TIMESTAMP - INTERVAL 1 DAY),
(2, 1, 4, 'waiting', CURRENT_TIMESTAMP - INTERVAL 2 HOUR),
(3, 5, 2, 'waiting', CURRENT_TIMESTAMP - INTERVAL 4 HOUR)
ON DUPLICATE KEY UPDATE status=VALUES(status), created_at=VALUES(created_at);

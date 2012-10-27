(display "Hello! How old are you: ")
(set! AGE (read-number))
(display "You were born in: " (- (date-year) AGE))(newline)

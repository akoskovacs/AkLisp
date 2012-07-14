(display "Hello! How old are you: ")
(setq AGE (read-number))
(display "You were born in: " (- (date-year) AGE))(newline)

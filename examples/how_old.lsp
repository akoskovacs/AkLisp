(write "Hello! How old are you: ")
(set! age (read-number))
(set! year (first (get-date-time)))
(display "You were born in: " (- year age))

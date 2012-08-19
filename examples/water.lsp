(display "Water tempature (in C): ")
(set! TEMP (read-number))
(display "The water is " 
    (cond 
        ((> TEMP 100) "steam.")
        ((< TEMP 0) "ice.")
        (T "just water.")
   )
)
(newline)

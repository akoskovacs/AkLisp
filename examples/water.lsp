(display "Water tempature (in C): ")
(setq TEMP (read-number))
;; Unfourtunatly there is no cond at the time
(display "The water is " (if (< TEMP 0) "ice." "not ice.")) (newline)

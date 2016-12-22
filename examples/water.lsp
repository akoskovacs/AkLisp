(write "Water tempature (in C): ")
(set! temp (read-number))
(when (nil? temp) ($ (display "This is a wrong number!") (exit! 1))
  
(display "The water is " 
    (if (<= temp 0) "ice." 
      (if (< temp 100) "just water."
        (when (>= temp 100) "steam.")
      )
   )
)

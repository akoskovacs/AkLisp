;; Greet the user according to the time of day
(write "What is your name: ")
(set! name (read-string))      ; Read to the name global variable
(set! hour (first (get-time))) ; The first element of the list is the hour

(if (< hour 12) (write "Good morning")
  (if (< hour 18) (write "Good aftenoon")
    (when (>= hour 18) (write "Good evening"))
  )
)
(display ", " name "!")


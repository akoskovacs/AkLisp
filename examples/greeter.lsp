;; Greet the user according to the time of day
(display "What is your name: ")
(set! NAME (read-word)) ; Who is it?
(set! HOUR (first (time))) ; What's the time?
(cond
  ((> HOUR 18) (display "Good evening " ))
  ((> HOUR 12) (display "Good aftenoon " ))
  ((< HOUR 12) (display "Good morning " ))
)
(display NAME "!")(newline)


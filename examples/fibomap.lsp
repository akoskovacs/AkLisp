; A faster fibonacci implementation with a global variable

; Start with the first to numbers
(set! ls '(1 1))
(defun! fiblist ()
        ; Add a new number to the list by adding the sum of the 
        ; last (-1) and the one before (-2)
        (append! (+ (last ls) (index -2 ls)) ls)
)

; Add another 40 numbers (calling fiblist 40 times)
(times 40 fiblist)
(print ls)

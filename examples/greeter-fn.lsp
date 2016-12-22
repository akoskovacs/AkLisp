;; Greet the user according to the time of day
;; implemented with a function

;; argument is the hour (0-23)
;; the returned value is the proper greeter string
(defun! hour-to-greeting (hour)
    (if (< hour 12) "Good morning"
      (if (< hour 18) "Good aftenoon"
        (when (>= hour 18) "Good evening")
      )
    )
)
; Comment out the next line to see the bytecode disassembly:
;(disassemble :hour-to-greeting)

; The program starts here
(write "What is your name: ")
(set! name (read-string))      ; Read to the name global variable
;; Set the hour argument to the first element of the time list
;; and write out the returned greeter string, the name and punctuation.
(display (hour-to-greeting (first (get-time))) ", " name "!")


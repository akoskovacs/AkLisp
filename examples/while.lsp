; Print a string n times with the index
(defun! print-n (n str) ($
        (set! i 0)
        (while (< i n)
               ($
                 (display i ". " str)
                 (set! i (++ i))
               )
        )
    )
)

(print-n 10 "this is a string again")
(display "Now some disassembly of the print-n function:")
(disassemble :print-n)

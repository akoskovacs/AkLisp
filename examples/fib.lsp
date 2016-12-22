; Recursive, interactive fibonacci implementation

; The fib function is used by others too
(load "./fib-fn.lsp")

(set! n 0)
(while (not (nil? n))
       ($ (write "Please give me a number: ")
          (display "The " (set! n (read-number)) 
                   ". fibonacci number is " (fib n))
       )
)
(display "And the program ended.")
; Wanna see some bytecode?
;(disassemble)

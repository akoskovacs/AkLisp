; This file is loaded by other fibonacci examples,
; it's useless in itself :(.

(defun! fib (n)
        (if (<= n 1)
          1  ; the first two numbers are just 1
          (+ (fib (- n 2)) (fib (-- n))) ; fib(n-2)+fib(n-1)
        )
)

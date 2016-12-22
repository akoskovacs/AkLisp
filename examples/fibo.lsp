; We need the fib function first
(load "./fib-fn.lsp")

; times-index is similar to times
; but it will also pass the index (from 0)
; of it's current iteration.
(print (times-index 24
    (lambda (x)
      ($ (display x " = " (set! fb (fib x))) fb)
    ))
)

; A faster, sequential fibonacci implementation with lists
(set! ls '(1 1))
(set! n 0)
(set! i 2)

; C-stlye while statement
(while (>= 30 i)
       ; Cache the last value in n, as ls[-1]+ls[-2]
       ; then add it to the list
       ($ (append! 
            (set! n (+ (index (-- i) ls) 
               (index (- i 2) ls))) ls)
          (display i " = " n)
          (set! i (++ i)) ; increment cycle counter
       )
)
(print ls)

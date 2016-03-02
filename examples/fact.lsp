;; Recursively get numbers from the user and print their factorial
(defun! fact (n)
        (if (<= n 1)
          1
          (* n (fact (-- n)))
        )
)

(defun! get-fact ()
        ($ (write "Please, give me a number:")
           (read-number))
)
;(disassemble :get-fact)

(defun! read-factorial (n)
        (if (nil? n)
          (display "The program is over. :(")
          (if (= n t) ;; first call
            (read-factorial (get-fact))
            ($ (display n "! =" (fact n)) 
               (read-factorial (get-fact))) 
          )
        )
)
;(disassemble :read-factorial)
(read-factorial t)

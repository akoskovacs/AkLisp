;; Write out the factorial of a given number using range
(defun! fact (n)
        (if (= 1 n)
          1
          (* n (fact (-- n)))
        )
)

(display "4! =" (fact 4))

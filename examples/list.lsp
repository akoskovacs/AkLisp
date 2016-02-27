(defun! print-list (l)
        (if (nil? l) 
          t
          ($
              (print (head l))
              (print-list (tail l))
          )
        )
)

(print-list '(:this :is 1 "very long list" '(:with "another" :list :inside)))

; A not-so-good sum could be like this:
;(defun! sum (l n)
;        (if (nil? l)
;          ($ n)
;          (sum (tail l) (+ n (head l)))
;        )
;)
; Instead, just use fold from 0 and add the numbers
; (after an int conversion) together.
; Lastly divide the whole thing by the list count, to get
; the average.
(defun! average (xs)
        (/ (fold 0 (map xs int) +) (length xs))
)

; (disassemble :average)
(if (= 1 *argc*) 
  ; Calling with zero arguments would give a zero division error!
  (display "Need some numbers!")  

  ; The first element is the script name, but we don't need that.
  (display (average (tail *args*)))
)

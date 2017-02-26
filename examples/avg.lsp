; A not-so-good sum could be like this:
;(defun! sum (l n)
;        (if (nil? l)
;          ($ n)
;          (sum (tail l) (+ n (head l)))
;        )
;)

;  The *args* variable always contains strings, but we need numbers.
; We iterate over the strs list and convert everything to number with 
; the functions 'map' and 'number'. 
;  'map' will call it's second argument (the 'number' function) 
; on every element from it's first parameter (the 'strs' list).
(defun! to-number-list (strs) (map strs number))

; To add the numbers together, we fold the list from left,
; calling the '+' function with 0 and the first number from 'xs'.
; The result will be added to the next number in the list and so on...
; fold will stop when there are no numbers left in the 'xs' list.
(defun! sum (xs) (fold 0 xs +))

; To get the average, we add the numbers together and divide
; the result with the length of the list.
(defun! average (xs) (/ (sum xs) (length xs)))

; (disassemble :average)
(if (= 1 *argc*) 
  ; Calling with zero arguments would give a zero division error!
  (display "Need some numbers!")  

  ; The first element is the script name, but we don't need that.
  ; So, we convert the remaining *args* to numbers and call
  ; the average function on it.
  (display (average (to-number-list (tail *args*))))
)

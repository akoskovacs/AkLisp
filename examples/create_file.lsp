; This program just writes an arbitrary line to an arbitrary file
; Only works when the file module installed on your system
(LOAD "file")
(DISPLAY "Please give the filename: ")
(SET! fname (GETLINE stdin))
(IF (NIL? (SET! file (OPEN fname "w")))
    (PROGN (DISPLAY "Cannot open " fname "!")
           (NEWLINE)
           (EXIT 1)
    )
)
(DISPLAY "Line to write: ")
(SET! line (GETLINE stdin))
(FPRINT file line)
(CLOSE file)
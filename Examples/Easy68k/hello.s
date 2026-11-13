; Hello, world for the Easy68K runner
;
; Assemble with:  vasmm68k_mot -m68000 -Fsrec -s19 hello.s -o hello.s68

        org     $2000

        lea     msg(pc),a1
        moveq   #13,d0              ; Display string with CR/LF
        trap    #15
        moveq   #9,d0               ; Halt
        trap    #15

msg:    dc.b    'Hello, world!',0

; Exercises some TRAP #15 tasks of the Easy68K runner. The program prints one
; line of text, which is checked by the test.
;
; Assemble with:  vasmm68k_mot -m68000 -Fsrec -s19 tasks.s -o tasks.s68

        org     $2000

start:  lea     s_res(pc),a1
        moveq   #17,d0              ; String + signed decimal
        move.l  #-12345,d1
        trap    #15

        lea     s_hex(pc),a1
        moveq   #14,d0              ; String
        trap    #15
        moveq   #15,d0              ; Unsigned number in base D2.B
        move.l  #$DEADBEEF,d1
        moveq   #16,d2
        trap    #15

        lea     s_bin(pc),a1
        moveq   #14,d0
        trap    #15
        moveq   #15,d0
        moveq   #10,d1
        moveq   #2,d2
        trap    #15

        lea     s_fld(pc),a1
        moveq   #14,d0
        trap    #15
        moveq   #20,d0              ; Signed decimal in a field
        moveq   #42,d1
        moveq   #6,d2
        trap    #15
        lea     s_end(pc),a1
        moveq   #14,d0
        trap    #15

        moveq   #6,d0               ; Single characters
        move.b  #'O',d1
        trap    #15
        move.b  #'K',d1
        trap    #15
        move.b  #' ',d1
        trap    #15

        lea     s_abc(pc),a1
        moveq   #1,d0               ; String of given length, no CR/LF
        moveq   #3,d1
        trap    #15
        moveq   #0,d0               ; String of given length, with CR/LF
        moveq   #2,d1
        trap    #15

        moveq   #9,d0               ; Halt
        trap    #15

s_res:  dc.b    'RESULT: ',0
s_hex:  dc.b    ' ',0
s_bin:  dc.b    ' ',0
s_fld:  dc.b    ' [',0
s_end:  dc.b    '] ',0
s_abc:  dc.b    'abcdef'

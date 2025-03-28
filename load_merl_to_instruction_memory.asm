.import readWord
.import printHex
lis $4
.word 4
sw $31, -4($30)
sub $30, $30, $4
lis $5
.word 1
lis $6
.word 12
lis $7
.word readWord
jalr $7
add $9, $3, $0
add $10, $3, $0
lw $31, 0($30)
add $30, $30, $4
sw $31, -4($30)
sub $30, $30, $4
lis $7
.word readWord
jalr $7
lw $31, 0($30)
add $30, $30, $4
sw $31, -4($30)
sub $30, $30, $4
lis $7
.word readWord
jalr $7
lw $31, 0($30)
add $30, $30, $4
sw $31, -4($30)
sub $30, $30, $4
lis $7
.word readWord
jalr $7
lw $31, 0($30)
add $30, $30, $4
sub $3, $3, $6
div $3, $4
mflo $8
add $11, $8, $0
add $12, $8, $0
printline:
    sw $31, -4($30)
    sub $30, $30, $4
    lis $7
    .word readWord
    jalr $7
    add $1, $3, $0
    sw $1, 0($9)
    add $9, $9, $4
    lw $31, 0($30)
    add $30, $30, $4
    sw $31, -4($30)
    sub $30, $30, $4
    lis $7
    .word printHex
    jalr $7
    lw $31, 0($30)
    add $30, $30, $4
    sub $8, $8, $5
    bne $8, $0, printline
printagain:
    lw $1, 0($10)
    add $10, $10, $4
    sw $31, -4($30)
    sub $30, $30, $4
    lis $7
    .word printHex
    jalr $7
    lw $31, 0($30)
    add $30, $30, $4
    sub $12, $12, $5
    bne $12, $0, printagain
jr $31

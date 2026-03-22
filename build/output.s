.text
.globl func
.type func, @function
func:
  pushl %ebp
  movl %esp, %ebp
  subl $52, %esp
  pushl %ebx
.LBB0:
  movl $10, -8(%ebp)
  movl $10, -12(%ebp)
  movl $10, -16(%ebp)
  movl $30, -24(%ebp)
  movl $30, -28(%ebp)
  movl $30, -32(%ebp)
  movl $20, -36(%ebp)
  movl $20, -40(%ebp)
  movl $20, %edx
  cmpl $30, %edx
  jl .LBB2
  jmp .LBB3
.LBB1:
  movl $1, %eax
  popl %ebx
  leave
  ret
.LBB2:
  movl $20, -44(%ebp)
  jmp .LBB3
.LBB3:
  movl $1, -52(%ebp)
  jmp .LBB1
.LBB4:
  jmp .LBB1


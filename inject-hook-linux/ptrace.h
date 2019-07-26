
void ptrace_attach(pid_t target);
void remote_stop(pid_t target);
void ptrace_detach(pid_t target);
void ptrace_getregs(pid_t target, struct user_regs_struct *regs);
void ptrace_cont(pid_t target);
void ptrace_setregs(pid_t target, const struct user_regs_struct *regs);
siginfo_t ptrace_getsiginfo(pid_t target);
void ptrace_read(int pid, unsigned long addr, void *vptr, int len);
void ptrace_write(int pid, unsigned long addr, void *vptr, int len);
void checktargetsig(int pid);
void restoreStateAndDetach(pid_t target, unsigned long addr, void* backup, int datasize, struct user_regs_struct oldregs);

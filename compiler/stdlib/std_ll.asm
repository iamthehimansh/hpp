; H++ Standard Low-Level Library
; Linux x86-64 syscall wrappers
; Calling convention: System V AMD64 ABI
;   args in rdi, rsi, rdx, rcx (-> r10 for syscall), r8, r9
;   return value in rax

section .text

; ============================================================
; FILE I/O
; ============================================================

; sys_read(fd: int, buf: ptr, count: u64) -> i64
; Syscall: 0
global sys_read
sys_read:
    mov rax, 0
    syscall
    ret

; sys_write(fd: int, buf: ptr, count: u64) -> i64
; Syscall: 1
global sys_write
sys_write:
    mov rax, 1
    syscall
    ret

; sys_open(pathname: ptr, flags: int, mode: int) -> int
; Syscall: 2
global sys_open
sys_open:
    mov rax, 2
    syscall
    ret

; sys_close(fd: int) -> int
; Syscall: 3
global sys_close
sys_close:
    mov rax, 3
    syscall
    ret

; sys_stat(pathname: ptr, statbuf: ptr) -> int
; Syscall: 4
global sys_stat
sys_stat:
    mov rax, 4
    syscall
    ret

; sys_fstat(fd: int, statbuf: ptr) -> int
; Syscall: 5
global sys_fstat
sys_fstat:
    mov rax, 5
    syscall
    ret

; sys_lstat(pathname: ptr, statbuf: ptr) -> int
; Syscall: 6
global sys_lstat
sys_lstat:
    mov rax, 6
    syscall
    ret

; sys_lseek(fd: int, offset: i64, whence: int) -> i64
; Syscall: 8
global sys_lseek
sys_lseek:
    mov rax, 8
    syscall
    ret

; sys_pread64(fd: int, buf: ptr, count: u64, offset: i64) -> i64
; Syscall: 17
global sys_pread64
sys_pread64:
    mov r10, rcx
    mov rax, 17
    syscall
    ret

; sys_pwrite64(fd: int, buf: ptr, count: u64, offset: i64) -> i64
; Syscall: 18
global sys_pwrite64
sys_pwrite64:
    mov r10, rcx
    mov rax, 18
    syscall
    ret

; sys_access(pathname: ptr, mode: int) -> int
; Syscall: 21
global sys_access
sys_access:
    mov rax, 21
    syscall
    ret

; sys_pipe(pipefd: ptr) -> int
; Syscall: 22
global sys_pipe
sys_pipe:
    mov rax, 22
    syscall
    ret

; sys_dup(oldfd: int) -> int
; Syscall: 32
global sys_dup
sys_dup:
    mov rax, 32
    syscall
    ret

; sys_dup2(oldfd: int, newfd: int) -> int
; Syscall: 33
global sys_dup2
sys_dup2:
    mov rax, 33
    syscall
    ret

; sys_fcntl(fd: int, cmd: int, arg: u64) -> int
; Syscall: 72
global sys_fcntl
sys_fcntl:
    mov rax, 72
    syscall
    ret

; sys_fsync(fd: int) -> int
; Syscall: 74
global sys_fsync
sys_fsync:
    mov rax, 74
    syscall
    ret

; sys_truncate(pathname: ptr, length: i64) -> int
; Syscall: 76
global sys_truncate
sys_truncate:
    mov rax, 76
    syscall
    ret

; sys_ftruncate(fd: int, length: i64) -> int
; Syscall: 77
global sys_ftruncate
sys_ftruncate:
    mov rax, 77
    syscall
    ret

; sys_rename(oldpath: ptr, newpath: ptr) -> int
; Syscall: 82
global sys_rename
sys_rename:
    mov rax, 82
    syscall
    ret

; sys_unlink(pathname: ptr) -> int
; Syscall: 87
global sys_unlink
sys_unlink:
    mov rax, 87
    syscall
    ret

; sys_readlink(pathname: ptr, buf: ptr, bufsiz: u64) -> i64
; Syscall: 89
global sys_readlink
sys_readlink:
    mov rax, 89
    syscall
    ret

; sys_chmod(pathname: ptr, mode: int) -> int
; Syscall: 90
global sys_chmod
sys_chmod:
    mov rax, 90
    syscall
    ret

; sys_fchmod(fd: int, mode: int) -> int
; Syscall: 91
global sys_fchmod
sys_fchmod:
    mov rax, 91
    syscall
    ret

; sys_chown(pathname: ptr, owner: int, group: int) -> int
; Syscall: 92
global sys_chown
sys_chown:
    mov rax, 92
    syscall
    ret

; ============================================================
; MEMORY
; ============================================================

; sys_mmap(addr: ptr, len: u64, prot: int, flags: int, fd: int, offset: i64) -> ptr
; Syscall: 9
; 6 args: rcx -> r10 for 4th arg (flags)
global sys_mmap
sys_mmap:
    mov r10, rcx
    mov rax, 9
    syscall
    ret

; sys_mprotect(addr: ptr, len: u64, prot: int) -> int
; Syscall: 10
global sys_mprotect
sys_mprotect:
    mov rax, 10
    syscall
    ret

; sys_munmap(addr: ptr, len: u64) -> int
; Syscall: 11
global sys_munmap
sys_munmap:
    mov rax, 11
    syscall
    ret

; sys_brk(addr: ptr) -> ptr
; Syscall: 12
global sys_brk
sys_brk:
    mov rax, 12
    syscall
    ret

; sys_mremap(old_addr: ptr, old_size: u64, new_size: u64, flags: int, new_addr: ptr) -> ptr
; Syscall: 25
; 5 args: rcx -> r10 for 4th arg (flags)
global sys_mremap
sys_mremap:
    mov r10, rcx
    mov rax, 25
    syscall
    ret

; ============================================================
; PROCESS
; ============================================================

; sys_getpid() -> int
; Syscall: 39
global sys_getpid
sys_getpid:
    mov rax, 39
    syscall
    ret

; sys_fork() -> int
; Syscall: 57
global sys_fork
sys_fork:
    mov rax, 57
    syscall
    ret

; sys_execve(filename: ptr, argv: ptr, envp: ptr) -> int
; Syscall: 59
global sys_execve
sys_execve:
    mov rax, 59
    syscall
    ret

; sys_exit(status: int) -> void
; Syscall: 60
global sys_exit
sys_exit:
    mov rax, 60
    syscall
    ret

; sys_wait4(pid: int, wstatus: ptr, options: int, rusage: ptr) -> int
; Syscall: 61
; 4 args: rcx -> r10 for 4th arg (rusage)
global sys_wait4
sys_wait4:
    mov r10, rcx
    mov rax, 61
    syscall
    ret

; sys_kill(pid: int, sig: int) -> int
; Syscall: 62
global sys_kill
sys_kill:
    mov rax, 62
    syscall
    ret

; sys_getuid() -> int
; Syscall: 102
global sys_getuid
sys_getuid:
    mov rax, 102
    syscall
    ret

; sys_getgid() -> int
; Syscall: 104
global sys_getgid
sys_getgid:
    mov rax, 104
    syscall
    ret

; sys_setuid(uid: int) -> int
; Syscall: 105
global sys_setuid
sys_setuid:
    mov rax, 105
    syscall
    ret

; sys_setgid(gid: int) -> int
; Syscall: 106
global sys_setgid
sys_setgid:
    mov rax, 106
    syscall
    ret

; sys_geteuid() -> int
; Syscall: 107
global sys_geteuid
sys_geteuid:
    mov rax, 107
    syscall
    ret

; sys_getegid() -> int
; Syscall: 108
global sys_getegid
sys_getegid:
    mov rax, 108
    syscall
    ret

; sys_getppid() -> int
; Syscall: 110
global sys_getppid
sys_getppid:
    mov rax, 110
    syscall
    ret

; sys_setsid() -> int
; Syscall: 112
global sys_setsid
sys_setsid:
    mov rax, 112
    syscall
    ret

; sys_exit_group(status: int) -> void
; Syscall: 231
global sys_exit_group
sys_exit_group:
    mov rax, 231
    syscall
    ret

; ============================================================
; DIRECTORY
; ============================================================

; sys_getcwd(buf: ptr, size: u64) -> ptr
; Syscall: 79
global sys_getcwd
sys_getcwd:
    mov rax, 79
    syscall
    ret

; sys_chdir(path: ptr) -> int
; Syscall: 80
global sys_chdir
sys_chdir:
    mov rax, 80
    syscall
    ret

; sys_mkdir(pathname: ptr, mode: int) -> int
; Syscall: 83
global sys_mkdir
sys_mkdir:
    mov rax, 83
    syscall
    ret

; sys_rmdir(pathname: ptr) -> int
; Syscall: 84
global sys_rmdir
sys_rmdir:
    mov rax, 84
    syscall
    ret

; sys_getdents64(fd: int, dirp: ptr, count: u64) -> int
; Syscall: 217
global sys_getdents64
sys_getdents64:
    mov rax, 217
    syscall
    ret

; ============================================================
; NETWORK (SOCKET)
; ============================================================

; sys_socket(domain: int, type: int, protocol: int) -> int
; Syscall: 41
global sys_socket
sys_socket:
    mov rax, 41
    syscall
    ret

; sys_connect(sockfd: int, addr: ptr, addrlen: int) -> int
; Syscall: 42
global sys_connect
sys_connect:
    mov rax, 42
    syscall
    ret

; sys_accept(sockfd: int, addr: ptr, addrlen: ptr) -> int
; Syscall: 43
global sys_accept
sys_accept:
    mov rax, 43
    syscall
    ret

; sys_sendto(sockfd: int, buf: ptr, len: u64, flags: int, dest_addr: ptr, addrlen: int) -> i64
; Syscall: 44
; 6 args: rcx -> r10 for 4th arg (flags)
global sys_sendto
sys_sendto:
    mov r10, rcx
    mov rax, 44
    syscall
    ret

; sys_recvfrom(sockfd: int, buf: ptr, len: u64, flags: int, src_addr: ptr, addrlen: ptr) -> i64
; Syscall: 45
; 6 args: rcx -> r10 for 4th arg (flags)
global sys_recvfrom
sys_recvfrom:
    mov r10, rcx
    mov rax, 45
    syscall
    ret

; sys_shutdown(sockfd: int, how: int) -> int
; Syscall: 48
global sys_shutdown
sys_shutdown:
    mov rax, 48
    syscall
    ret

; sys_bind(sockfd: int, addr: ptr, addrlen: int) -> int
; Syscall: 49
global sys_bind
sys_bind:
    mov rax, 49
    syscall
    ret

; sys_listen(sockfd: int, backlog: int) -> int
; Syscall: 50
global sys_listen
sys_listen:
    mov rax, 50
    syscall
    ret

; sys_setsockopt(sockfd: int, level: int, optname: int, optval: ptr, optlen: int) -> int
; Syscall: 54
; 5 args: rcx -> r10 for 4th arg (optval)
global sys_setsockopt
sys_setsockopt:
    mov r10, rcx
    mov rax, 54
    syscall
    ret

; sys_getsockopt(sockfd: int, level: int, optname: int, optval: ptr, optlen: ptr) -> int
; Syscall: 55
; 5 args: rcx -> r10 for 4th arg (optval)
global sys_getsockopt
sys_getsockopt:
    mov r10, rcx
    mov rax, 55
    syscall
    ret

; ============================================================
; TIME
; ============================================================

; sys_nanosleep(req: ptr, rem: ptr) -> int
; Syscall: 35
global sys_nanosleep
sys_nanosleep:
    mov rax, 35
    syscall
    ret

; sys_gettimeofday(tv: ptr, tz: ptr) -> int
; Syscall: 96
global sys_gettimeofday
sys_gettimeofday:
    mov rax, 96
    syscall
    ret

; sys_time(tloc: ptr) -> i64
; Syscall: 201
global sys_time
sys_time:
    mov rax, 201
    syscall
    ret

; sys_clock_gettime(clk_id: int, tp: ptr) -> int
; Syscall: 228
global sys_clock_gettime
sys_clock_gettime:
    mov rax, 228
    syscall
    ret

; ============================================================
; SIGNAL
; ============================================================

; sys_rt_sigaction(signum: int, act: ptr, oldact: ptr, sigsetsize: u64) -> int
; Syscall: 13
; 4 args: rcx -> r10 for 4th arg (sigsetsize)
global sys_rt_sigaction
sys_rt_sigaction:
    mov r10, rcx
    mov rax, 13
    syscall
    ret

; sys_rt_sigprocmask(how: int, set: ptr, oldset: ptr, sigsetsize: u64) -> int
; Syscall: 14
; 4 args: rcx -> r10 for 4th arg (sigsetsize)
global sys_rt_sigprocmask
sys_rt_sigprocmask:
    mov r10, rcx
    mov rax, 14
    syscall
    ret

; sys_pause() -> int
; Syscall: 34
global sys_pause
sys_pause:
    mov rax, 34
    syscall
    ret

; sys_alarm(seconds: int) -> int
; Syscall: 37
global sys_alarm
sys_alarm:
    mov rax, 37
    syscall
    ret

; ============================================================
; MISC
; ============================================================

; sys_poll(fds: ptr, nfds: u64, timeout: int) -> int
; Syscall: 7
global sys_poll
sys_poll:
    mov rax, 7
    syscall
    ret

; sys_ioctl(fd: int, request: u64, arg: u64) -> int
; Syscall: 16
global sys_ioctl
sys_ioctl:
    mov rax, 16
    syscall
    ret

; sys_select(nfds: int, readfds: ptr, writefds: ptr, exceptfds: ptr, timeout: ptr) -> int
; Syscall: 23
; 5 args: rcx -> r10 for 4th arg (exceptfds)
global sys_select
sys_select:
    mov r10, rcx
    mov rax, 23
    syscall
    ret

; sys_uname(buf: ptr) -> int
; Syscall: 63
global sys_uname
sys_uname:
    mov rax, 63
    syscall
    ret

; sys_getrusage(who: int, usage: ptr) -> int
; Syscall: 98
global sys_getrusage
sys_getrusage:
    mov rax, 98
    syscall
    ret

; sys_sysinfo(info: ptr) -> int
; Syscall: 99
global sys_sysinfo
sys_sysinfo:
    mov rax, 99
    syscall
    ret

; sys_getrandom(buf: ptr, buflen: u64, flags: int) -> i64
; Syscall: 318
global sys_getrandom
sys_getrandom:
    mov rax, 318
    syscall
    ret

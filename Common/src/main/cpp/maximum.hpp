/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Fri Jan 27 12:35:09 CET 2023                                                 */


constexpr const int callmax=std::max({
#ifdef __NR__llseek
__NR__llseek,
#endif
#ifdef __NR__newselect
__NR__newselect,
#endif
#ifdef __NR__sysctl
__NR__sysctl,
#endif
#ifdef __NR_accept
__NR_accept,
#endif
#ifdef __NR_accept4
__NR_accept4,
#endif
#ifdef __NR_access
__NR_access,
#endif
#ifdef __NR_acct
__NR_acct,
#endif
#ifdef __NR_add_key
__NR_add_key,
#endif
#ifdef __NR_adjtimex
__NR_adjtimex,
#endif
#ifdef __NR_afs_syscall
__NR_afs_syscall,
#endif
#ifdef __NR_alarm
__NR_alarm,
#endif
#ifdef __NR_arch_prctl
__NR_arch_prctl,
#endif
#ifdef __NR_arch_specific_syscall
__NR_arch_specific_syscall,
#endif
#ifdef __NR_arm_fadvise64_64
__NR_arm_fadvise64_64,
#endif
#ifdef __NR_arm_sync_file_range
__NR_arm_sync_file_range,
#endif
#ifdef __NR_bdflush
__NR_bdflush,
#endif
#ifdef __NR_bind
__NR_bind,
#endif
#ifdef __NR_bpf
__NR_bpf,
#endif
#ifdef __NR_break
__NR_break,
#endif
#ifdef __NR_brk
__NR_brk,
#endif
#ifdef __NR_capget
__NR_capget,
#endif
#ifdef __NR_capset
__NR_capset,
#endif
#ifdef __NR_chdir
__NR_chdir,
#endif
#ifdef __NR_chmod
__NR_chmod,
#endif
#ifdef __NR_chown
__NR_chown,
#endif
#ifdef __NR_chown32
__NR_chown32,
#endif
#ifdef __NR_chroot
__NR_chroot,
#endif
#ifdef __NR_clock_adjtime
__NR_clock_adjtime,
#endif
#ifdef __NR_clock_adjtime64
__NR_clock_adjtime64,
#endif
#ifdef __NR_clock_getres
__NR_clock_getres,
#endif
#ifdef __NR_clock_getres_time64
__NR_clock_getres_time64,
#endif
#ifdef __NR_clock_gettime
__NR_clock_gettime,
#endif
#ifdef __NR_clock_gettime64
__NR_clock_gettime64,
#endif
#ifdef __NR_clock_nanosleep
__NR_clock_nanosleep,
#endif
#ifdef __NR_clock_nanosleep_time64
__NR_clock_nanosleep_time64,
#endif
#ifdef __NR_clock_settime
__NR_clock_settime,
#endif
#ifdef __NR_clock_settime64
__NR_clock_settime64,
#endif
#ifdef __NR_clone
__NR_clone,
#endif
#ifdef __NR_clone3
__NR_clone3,
#endif
#ifdef __NR_close
__NR_close,
#endif
#ifdef __NR_close_range
__NR_close_range,
#endif
#ifdef __NR_connect
__NR_connect,
#endif
#ifdef __NR_copy_file_range
__NR_copy_file_range,
#endif
#ifdef __NR_creat
__NR_creat,
#endif
#ifdef __NR_create_module
__NR_create_module,
#endif
#ifdef __NR_delete_module
__NR_delete_module,
#endif
#ifdef __NR_dup
__NR_dup,
#endif
#ifdef __NR_dup2
__NR_dup2,
#endif
#ifdef __NR_dup3
__NR_dup3,
#endif
#ifdef __NR_epoll_create
__NR_epoll_create,
#endif
#ifdef __NR_epoll_create1
__NR_epoll_create1,
#endif
#ifdef __NR_epoll_ctl
__NR_epoll_ctl,
#endif
#ifdef __NR_epoll_ctl_old
__NR_epoll_ctl_old,
#endif
#ifdef __NR_epoll_pwait
__NR_epoll_pwait,
#endif
#ifdef __NR_epoll_wait
__NR_epoll_wait,
#endif
#ifdef __NR_epoll_wait_old
__NR_epoll_wait_old,
#endif
#ifdef __NR_eventfd
__NR_eventfd,
#endif
#ifdef __NR_eventfd2
__NR_eventfd2,
#endif
#ifdef __NR_execve
__NR_execve,
#endif
#ifdef __NR_execveat
__NR_execveat,
#endif
#ifdef __NR_exit
__NR_exit,
#endif
#ifdef __NR_exit_group
__NR_exit_group,
#endif
#ifdef __NR_faccessat
__NR_faccessat,
#endif
#ifdef __NR_faccessat2
__NR_faccessat2,
#endif
#ifdef __NR_fadvise64
__NR_fadvise64,
#endif
#ifdef __NR_fadvise64_64
__NR_fadvise64_64,
#endif
#ifdef __NR_fallocate
__NR_fallocate,
#endif
#ifdef __NR_fanotify_init
__NR_fanotify_init,
#endif
#ifdef __NR_fanotify_mark
__NR_fanotify_mark,
#endif
#ifdef __NR_fchdir
__NR_fchdir,
#endif
#ifdef __NR_fchmod
__NR_fchmod,
#endif
#ifdef __NR_fchmodat
__NR_fchmodat,
#endif
#ifdef __NR_fchown
__NR_fchown,
#endif
#ifdef __NR_fchown32
__NR_fchown32,
#endif
#ifdef __NR_fchownat
__NR_fchownat,
#endif
#ifdef __NR_fcntl
__NR_fcntl,
#endif
#ifdef __NR_fcntl64
__NR_fcntl64,
#endif
#ifdef __NR_fdatasync
__NR_fdatasync,
#endif
#ifdef __NR_fgetxattr
__NR_fgetxattr,
#endif
#ifdef __NR_finit_module
__NR_finit_module,
#endif
#ifdef __NR_flistxattr
__NR_flistxattr,
#endif
#ifdef __NR_flock
__NR_flock,
#endif
#ifdef __NR_fork
__NR_fork,
#endif
#ifdef __NR_fremovexattr
__NR_fremovexattr,
#endif
#ifdef __NR_fsconfig
__NR_fsconfig,
#endif
#ifdef __NR_fsetxattr
__NR_fsetxattr,
#endif
#ifdef __NR_fsmount
__NR_fsmount,
#endif
#ifdef __NR_fsopen
__NR_fsopen,
#endif
#ifdef __NR_fspick
__NR_fspick,
#endif
#ifdef __NR_fstat
__NR_fstat,
#endif
#ifdef __NR_fstat64
__NR_fstat64,
#endif
#ifdef __NR_fstatat64
__NR_fstatat64,
#endif
#ifdef __NR_fstatfs
__NR_fstatfs,
#endif
#ifdef __NR_fstatfs64
__NR_fstatfs64,
#endif
#ifdef __NR_fsync
__NR_fsync,
#endif
#ifdef __NR_ftime
__NR_ftime,
#endif
#ifdef __NR_ftruncate
__NR_ftruncate,
#endif
#ifdef __NR_ftruncate64
__NR_ftruncate64,
#endif
#ifdef __NR_futex
__NR_futex,
#endif
#ifdef __NR_futex_time64
__NR_futex_time64,
#endif
#ifdef __NR_futimesat
__NR_futimesat,
#endif
#ifdef __NR_get_kernel_syms
__NR_get_kernel_syms,
#endif
#ifdef __NR_get_mempolicy
__NR_get_mempolicy,
#endif
#ifdef __NR_get_robust_list
__NR_get_robust_list,
#endif
#ifdef __NR_get_thread_area
__NR_get_thread_area,
#endif
#ifdef __NR_getcpu
__NR_getcpu,
#endif
#ifdef __NR_getcwd
__NR_getcwd,
#endif
#ifdef __NR_getdents
__NR_getdents,
#endif
#ifdef __NR_getdents64
__NR_getdents64,
#endif
#ifdef __NR_getegid
__NR_getegid,
#endif
#ifdef __NR_getegid32
__NR_getegid32,
#endif
#ifdef __NR_geteuid
__NR_geteuid,
#endif
#ifdef __NR_geteuid32
__NR_geteuid32,
#endif
#ifdef __NR_getgid
__NR_getgid,
#endif
#ifdef __NR_getgid32
__NR_getgid32,
#endif
#ifdef __NR_getgroups
__NR_getgroups,
#endif
#ifdef __NR_getgroups32
__NR_getgroups32,
#endif
#ifdef __NR_getitimer
__NR_getitimer,
#endif
#ifdef __NR_getpeername
__NR_getpeername,
#endif
#ifdef __NR_getpgid
__NR_getpgid,
#endif
#ifdef __NR_getpgrp
__NR_getpgrp,
#endif
#ifdef __NR_getpid
__NR_getpid,
#endif
#ifdef __NR_getpmsg
__NR_getpmsg,
#endif
#ifdef __NR_getppid
__NR_getppid,
#endif
#ifdef __NR_getpriority
__NR_getpriority,
#endif
#ifdef __NR_getrandom
__NR_getrandom,
#endif
#ifdef __NR_getresgid
__NR_getresgid,
#endif
#ifdef __NR_getresgid32
__NR_getresgid32,
#endif
#ifdef __NR_getresuid
__NR_getresuid,
#endif
#ifdef __NR_getresuid32
__NR_getresuid32,
#endif
#ifdef __NR_getrlimit
__NR_getrlimit,
#endif
#ifdef __NR_getrusage
__NR_getrusage,
#endif
#ifdef __NR_getsid
__NR_getsid,
#endif
#ifdef __NR_getsockname
__NR_getsockname,
#endif
#ifdef __NR_getsockopt
__NR_getsockopt,
#endif
#ifdef __NR_gettid
__NR_gettid,
#endif
#ifdef __NR_gettimeofday
__NR_gettimeofday,
#endif
#ifdef __NR_getuid
__NR_getuid,
#endif
#ifdef __NR_getuid32
__NR_getuid32,
#endif
#ifdef __NR_getxattr
__NR_getxattr,
#endif
#ifdef __NR_gtty
__NR_gtty,
#endif
#ifdef __NR_idle
__NR_idle,
#endif
#ifdef __NR_init_module
__NR_init_module,
#endif
#ifdef __NR_inotify_add_watch
__NR_inotify_add_watch,
#endif
#ifdef __NR_inotify_init
__NR_inotify_init,
#endif
#ifdef __NR_inotify_init1
__NR_inotify_init1,
#endif
#ifdef __NR_inotify_rm_watch
__NR_inotify_rm_watch,
#endif
#ifdef __NR_io_cancel
__NR_io_cancel,
#endif
#ifdef __NR_io_destroy
__NR_io_destroy,
#endif
#ifdef __NR_io_getevents
__NR_io_getevents,
#endif
#ifdef __NR_io_pgetevents
__NR_io_pgetevents,
#endif
#ifdef __NR_io_pgetevents_time64
__NR_io_pgetevents_time64,
#endif
#ifdef __NR_io_setup
__NR_io_setup,
#endif
#ifdef __NR_io_submit
__NR_io_submit,
#endif
#ifdef __NR_io_uring_enter
__NR_io_uring_enter,
#endif
#ifdef __NR_io_uring_register
__NR_io_uring_register,
#endif
#ifdef __NR_io_uring_setup
__NR_io_uring_setup,
#endif
#ifdef __NR_ioctl
__NR_ioctl,
#endif
#ifdef __NR_ioperm
__NR_ioperm,
#endif
#ifdef __NR_iopl
__NR_iopl,
#endif
#ifdef __NR_ioprio_get
__NR_ioprio_get,
#endif
#ifdef __NR_ioprio_set
__NR_ioprio_set,
#endif
#ifdef __NR_ipc
__NR_ipc,
#endif
#ifdef __NR_kcmp
__NR_kcmp,
#endif
#ifdef __NR_kexec_file_load
__NR_kexec_file_load,
#endif
#ifdef __NR_kexec_load
__NR_kexec_load,
#endif
#ifdef __NR_keyctl
__NR_keyctl,
#endif
#ifdef __NR_kill
__NR_kill,
#endif
#ifdef __NR_lchown
__NR_lchown,
#endif
#ifdef __NR_lchown32
__NR_lchown32,
#endif
#ifdef __NR_lgetxattr
__NR_lgetxattr,
#endif
#ifdef __NR_link
__NR_link,
#endif
#ifdef __NR_linkat
__NR_linkat,
#endif
#ifdef __NR_listen
__NR_listen,
#endif
#ifdef __NR_listxattr
__NR_listxattr,
#endif
#ifdef __NR_llistxattr
__NR_llistxattr,
#endif
#ifdef __NR_llseek
__NR_llseek,
#endif
#ifdef __NR_lock
__NR_lock,
#endif
#ifdef __NR_lookup_dcookie
__NR_lookup_dcookie,
#endif
#ifdef __NR_lremovexattr
__NR_lremovexattr,
#endif
#ifdef __NR_lseek
__NR_lseek,
#endif
#ifdef __NR_lsetxattr
__NR_lsetxattr,
#endif
#ifdef __NR_lstat
__NR_lstat,
#endif
#ifdef __NR_lstat64
__NR_lstat64,
#endif
#ifdef __NR_madvise
__NR_madvise,
#endif
#ifdef __NR_mbind
__NR_mbind,
#endif
#ifdef __NR_membarrier
__NR_membarrier,
#endif
#ifdef __NR_memfd_create
__NR_memfd_create,
#endif
#ifdef __NR_migrate_pages
__NR_migrate_pages,
#endif
#ifdef __NR_mincore
__NR_mincore,
#endif
#ifdef __NR_mkdir
__NR_mkdir,
#endif
#ifdef __NR_mkdirat
__NR_mkdirat,
#endif
#ifdef __NR_mknod
__NR_mknod,
#endif
#ifdef __NR_mknodat
__NR_mknodat,
#endif
#ifdef __NR_mlock
__NR_mlock,
#endif
#ifdef __NR_mlock2
__NR_mlock2,
#endif
#ifdef __NR_mlockall
__NR_mlockall,
#endif
#ifdef __NR_mmap
__NR_mmap,
#endif
#ifdef __NR_mmap2
__NR_mmap2,
#endif
#ifdef __NR_modify_ldt
__NR_modify_ldt,
#endif
#ifdef __NR_mount
__NR_mount,
#endif
#ifdef __NR_move_mount
__NR_move_mount,
#endif
#ifdef __NR_move_pages
__NR_move_pages,
#endif
#ifdef __NR_mprotect
__NR_mprotect,
#endif
#ifdef __NR_mpx
__NR_mpx,
#endif
#ifdef __NR_mq_getsetattr
__NR_mq_getsetattr,
#endif
#ifdef __NR_mq_notify
__NR_mq_notify,
#endif
#ifdef __NR_mq_open
__NR_mq_open,
#endif
#ifdef __NR_mq_timedreceive
__NR_mq_timedreceive,
#endif
#ifdef __NR_mq_timedreceive_time64
__NR_mq_timedreceive_time64,
#endif
#ifdef __NR_mq_timedsend
__NR_mq_timedsend,
#endif
#ifdef __NR_mq_timedsend_time64
__NR_mq_timedsend_time64,
#endif
#ifdef __NR_mq_unlink
__NR_mq_unlink,
#endif
#ifdef __NR_mremap
__NR_mremap,
#endif
#ifdef __NR_msgctl
__NR_msgctl,
#endif
#ifdef __NR_msgget
__NR_msgget,
#endif
#ifdef __NR_msgrcv
__NR_msgrcv,
#endif
#ifdef __NR_msgsnd
__NR_msgsnd,
#endif
#ifdef __NR_msync
__NR_msync,
#endif
#ifdef __NR_munlock
__NR_munlock,
#endif
#ifdef __NR_munlockall
__NR_munlockall,
#endif
#ifdef __NR_munmap
__NR_munmap,
#endif
#ifdef __NR_name_to_handle_at
__NR_name_to_handle_at,
#endif
#ifdef __NR_nanosleep
__NR_nanosleep,
#endif
#ifdef __NR_newfstatat
__NR_newfstatat,
#endif
#ifdef __NR_nfsservctl
__NR_nfsservctl,
#endif
#ifdef __NR_nice
__NR_nice,
#endif
#ifdef __NR_oldfstat
__NR_oldfstat,
#endif
#ifdef __NR_oldlstat
__NR_oldlstat,
#endif
#ifdef __NR_oldolduname
__NR_oldolduname,
#endif
#ifdef __NR_oldstat
__NR_oldstat,
#endif
#ifdef __NR_olduname
__NR_olduname,
#endif
#ifdef __NR_open
__NR_open,
#endif
#ifdef __NR_open_by_handle_at
__NR_open_by_handle_at,
#endif
#ifdef __NR_open_tree
__NR_open_tree,
#endif
#ifdef __NR_openat
__NR_openat,
#endif
#ifdef __NR_openat2
__NR_openat2,
#endif
#ifdef __NR_pause
__NR_pause,
#endif
#ifdef __NR_pciconfig_iobase
__NR_pciconfig_iobase,
#endif
#ifdef __NR_pciconfig_read
__NR_pciconfig_read,
#endif
#ifdef __NR_pciconfig_write
__NR_pciconfig_write,
#endif
#ifdef __NR_perf_event_open
__NR_perf_event_open,
#endif
#ifdef __NR_personality
__NR_personality,
#endif
#ifdef __NR_pidfd_getfd
__NR_pidfd_getfd,
#endif
#ifdef __NR_pidfd_open
__NR_pidfd_open,
#endif
#ifdef __NR_pidfd_send_signal
__NR_pidfd_send_signal,
#endif
#ifdef __NR_pipe
__NR_pipe,
#endif
#ifdef __NR_pipe2
__NR_pipe2,
#endif
#ifdef __NR_pivot_root
__NR_pivot_root,
#endif
#ifdef __NR_pkey_alloc
__NR_pkey_alloc,
#endif
#ifdef __NR_pkey_free
__NR_pkey_free,
#endif
#ifdef __NR_pkey_mprotect
__NR_pkey_mprotect,
#endif
#ifdef __NR_poll
__NR_poll,
#endif
#ifdef __NR_ppoll
__NR_ppoll,
#endif
#ifdef __NR_ppoll_time64
__NR_ppoll_time64,
#endif
#ifdef __NR_prctl
__NR_prctl,
#endif
#ifdef __NR_pread64
__NR_pread64,
#endif
#ifdef __NR_preadv
__NR_preadv,
#endif
#ifdef __NR_preadv2
__NR_preadv2,
#endif
#ifdef __NR_prlimit64
__NR_prlimit64,
#endif
#ifdef __NR_process_vm_readv
__NR_process_vm_readv,
#endif
#ifdef __NR_process_vm_writev
__NR_process_vm_writev,
#endif
#ifdef __NR_prof
__NR_prof,
#endif
#ifdef __NR_profil
__NR_profil,
#endif
#ifdef __NR_pselect6
__NR_pselect6,
#endif
#ifdef __NR_pselect6_time64
__NR_pselect6_time64,
#endif
#ifdef __NR_ptrace
__NR_ptrace,
#endif
#ifdef __NR_putpmsg
__NR_putpmsg,
#endif
#ifdef __NR_pwrite64
__NR_pwrite64,
#endif
#ifdef __NR_pwritev
__NR_pwritev,
#endif
#ifdef __NR_pwritev2
__NR_pwritev2,
#endif
#ifdef __NR_query_module
__NR_query_module,
#endif
#ifdef __NR_quotactl
__NR_quotactl,
#endif
#ifdef __NR_read
__NR_read,
#endif
#ifdef __NR_readahead
__NR_readahead,
#endif
#ifdef __NR_readdir
__NR_readdir,
#endif
#ifdef __NR_readlink
__NR_readlink,
#endif
#ifdef __NR_readlinkat
__NR_readlinkat,
#endif
#ifdef __NR_readv
__NR_readv,
#endif
#ifdef __NR_reboot
__NR_reboot,
#endif
#ifdef __NR_recv
__NR_recv,
#endif
#ifdef __NR_recvfrom
__NR_recvfrom,
#endif
#ifdef __NR_recvmmsg
__NR_recvmmsg,
#endif
#ifdef __NR_recvmmsg_time64
__NR_recvmmsg_time64,
#endif
#ifdef __NR_recvmsg
__NR_recvmsg,
#endif
#ifdef __NR_remap_file_pages
__NR_remap_file_pages,
#endif
#ifdef __NR_removexattr
__NR_removexattr,
#endif
#ifdef __NR_rename
__NR_rename,
#endif
#ifdef __NR_renameat
__NR_renameat,
#endif
#ifdef __NR_renameat2
__NR_renameat2,
#endif
#ifdef __NR_request_key
__NR_request_key,
#endif
#ifdef __NR_restart_syscall
__NR_restart_syscall,
#endif
#ifdef __NR_rmdir
__NR_rmdir,
#endif
#ifdef __NR_rseq
__NR_rseq,
#endif
#ifdef __NR_rt_sigaction
__NR_rt_sigaction,
#endif
#ifdef __NR_rt_sigpending
__NR_rt_sigpending,
#endif
#ifdef __NR_rt_sigprocmask
__NR_rt_sigprocmask,
#endif
#ifdef __NR_rt_sigqueueinfo
__NR_rt_sigqueueinfo,
#endif
#ifdef __NR_rt_sigreturn
__NR_rt_sigreturn,
#endif
#ifdef __NR_rt_sigsuspend
__NR_rt_sigsuspend,
#endif
#ifdef __NR_rt_sigtimedwait
__NR_rt_sigtimedwait,
#endif
#ifdef __NR_rt_sigtimedwait_time64
__NR_rt_sigtimedwait_time64,
#endif
#ifdef __NR_rt_tgsigqueueinfo
__NR_rt_tgsigqueueinfo,
#endif
#ifdef __NR_sched_get_priority_max
__NR_sched_get_priority_max,
#endif
#ifdef __NR_sched_get_priority_min
__NR_sched_get_priority_min,
#endif
#ifdef __NR_sched_getaffinity
__NR_sched_getaffinity,
#endif
#ifdef __NR_sched_getattr
__NR_sched_getattr,
#endif
#ifdef __NR_sched_getparam
__NR_sched_getparam,
#endif
#ifdef __NR_sched_getscheduler
__NR_sched_getscheduler,
#endif
#ifdef __NR_sched_rr_get_interval
__NR_sched_rr_get_interval,
#endif
#ifdef __NR_sched_rr_get_interval_time64
__NR_sched_rr_get_interval_time64,
#endif
#ifdef __NR_sched_setaffinity
__NR_sched_setaffinity,
#endif
#ifdef __NR_sched_setattr
__NR_sched_setattr,
#endif
#ifdef __NR_sched_setparam
__NR_sched_setparam,
#endif
#ifdef __NR_sched_setscheduler
__NR_sched_setscheduler,
#endif
#ifdef __NR_sched_yield
__NR_sched_yield,
#endif
#ifdef __NR_seccomp
__NR_seccomp,
#endif
#ifdef __NR_security
__NR_security,
#endif
#ifdef __NR_select
__NR_select,
#endif
#ifdef __NR_semctl
__NR_semctl,
#endif
#ifdef __NR_semget
__NR_semget,
#endif
#ifdef __NR_semop
__NR_semop,
#endif
#ifdef __NR_semtimedop
__NR_semtimedop,
#endif
#ifdef __NR_semtimedop_time64
__NR_semtimedop_time64,
#endif
#ifdef __NR_send
__NR_send,
#endif
#ifdef __NR_sendfile
__NR_sendfile,
#endif
#ifdef __NR_sendfile64
__NR_sendfile64,
#endif
#ifdef __NR_sendmmsg
__NR_sendmmsg,
#endif
#ifdef __NR_sendmsg
__NR_sendmsg,
#endif
#ifdef __NR_sendto
__NR_sendto,
#endif
#ifdef __NR_set_mempolicy
__NR_set_mempolicy,
#endif
#ifdef __NR_set_robust_list
__NR_set_robust_list,
#endif
#ifdef __NR_set_thread_area
__NR_set_thread_area,
#endif
#ifdef __NR_set_tid_address
__NR_set_tid_address,
#endif
#ifdef __NR_setdomainname
__NR_setdomainname,
#endif
#ifdef __NR_setfsgid
__NR_setfsgid,
#endif
#ifdef __NR_setfsgid32
__NR_setfsgid32,
#endif
#ifdef __NR_setfsuid
__NR_setfsuid,
#endif
#ifdef __NR_setfsuid32
__NR_setfsuid32,
#endif
#ifdef __NR_setgid
__NR_setgid,
#endif
#ifdef __NR_setgid32
__NR_setgid32,
#endif
#ifdef __NR_setgroups
__NR_setgroups,
#endif
#ifdef __NR_setgroups32
__NR_setgroups32,
#endif
#ifdef __NR_sethostname
__NR_sethostname,
#endif
#ifdef __NR_setitimer
__NR_setitimer,
#endif
#ifdef __NR_setns
__NR_setns,
#endif
#ifdef __NR_setpgid
__NR_setpgid,
#endif
#ifdef __NR_setpriority
__NR_setpriority,
#endif
#ifdef __NR_setregid
__NR_setregid,
#endif
#ifdef __NR_setregid32
__NR_setregid32,
#endif
#ifdef __NR_setresgid
__NR_setresgid,
#endif
#ifdef __NR_setresgid32
__NR_setresgid32,
#endif
#ifdef __NR_setresuid
__NR_setresuid,
#endif
#ifdef __NR_setresuid32
__NR_setresuid32,
#endif
#ifdef __NR_setreuid
__NR_setreuid,
#endif
#ifdef __NR_setreuid32
__NR_setreuid32,
#endif
#ifdef __NR_setrlimit
__NR_setrlimit,
#endif
#ifdef __NR_setsid
__NR_setsid,
#endif
#ifdef __NR_setsockopt
__NR_setsockopt,
#endif
#ifdef __NR_settimeofday
__NR_settimeofday,
#endif
#ifdef __NR_setuid
__NR_setuid,
#endif
#ifdef __NR_setuid32
__NR_setuid32,
#endif
#ifdef __NR_setxattr
__NR_setxattr,
#endif
#ifdef __NR_sgetmask
__NR_sgetmask,
#endif
#ifdef __NR_shmat
__NR_shmat,
#endif
#ifdef __NR_shmctl
__NR_shmctl,
#endif
#ifdef __NR_shmdt
__NR_shmdt,
#endif
#ifdef __NR_shmget
__NR_shmget,
#endif
#ifdef __NR_shutdown
__NR_shutdown,
#endif
#ifdef __NR_sigaction
__NR_sigaction,
#endif
#ifdef __NR_sigaltstack
__NR_sigaltstack,
#endif
#ifdef __NR_signal
__NR_signal,
#endif
#ifdef __NR_signalfd
__NR_signalfd,
#endif
#ifdef __NR_signalfd4
__NR_signalfd4,
#endif
#ifdef __NR_sigpending
__NR_sigpending,
#endif
#ifdef __NR_sigprocmask
__NR_sigprocmask,
#endif
#ifdef __NR_sigreturn
__NR_sigreturn,
#endif
#ifdef __NR_sigsuspend
__NR_sigsuspend,
#endif
#ifdef __NR_socket
__NR_socket,
#endif
#ifdef __NR_socketcall
__NR_socketcall,
#endif
#ifdef __NR_socketpair
__NR_socketpair,
#endif
#ifdef __NR_splice
__NR_splice,
#endif
#ifdef __NR_ssetmask
__NR_ssetmask,
#endif
#ifdef __NR_stat
__NR_stat,
#endif
#ifdef __NR_stat64
__NR_stat64,
#endif
#ifdef __NR_statfs
__NR_statfs,
#endif
#ifdef __NR_statfs64
__NR_statfs64,
#endif
#ifdef __NR_statx
__NR_statx,
#endif
#ifdef __NR_stime
__NR_stime,
#endif
#ifdef __NR_stty
__NR_stty,
#endif
#ifdef __NR_swapoff
__NR_swapoff,
#endif
#ifdef __NR_swapon
__NR_swapon,
#endif
#ifdef __NR_symlink
__NR_symlink,
#endif
#ifdef __NR_symlinkat
__NR_symlinkat,
#endif
#ifdef __NR_sync
__NR_sync,
#endif
#ifdef __NR_sync_file_range
__NR_sync_file_range,
#endif
#ifdef __NR_sync_file_range2
__NR_sync_file_range2,
#endif
#ifdef __NR_syncfs
__NR_syncfs,
#endif
#ifdef __NR_syscall
__NR_syscall,
#endif
#ifdef __NR_syscalls
__NR_syscalls,
#endif
#ifdef __NR_sysfs
__NR_sysfs,
#endif
#ifdef __NR_sysinfo
__NR_sysinfo,
#endif
#ifdef __NR_syslog
__NR_syslog,
#endif
#ifdef __NR_tee
__NR_tee,
#endif
#ifdef __NR_tgkill
__NR_tgkill,
#endif
#ifdef __NR_time
__NR_time,
#endif
#ifdef __NR_timer_create
__NR_timer_create,
#endif
#ifdef __NR_timer_delete
__NR_timer_delete,
#endif
#ifdef __NR_timer_getoverrun
__NR_timer_getoverrun,
#endif
#ifdef __NR_timer_gettime
__NR_timer_gettime,
#endif
#ifdef __NR_timer_gettime64
__NR_timer_gettime64,
#endif
#ifdef __NR_timer_settime
__NR_timer_settime,
#endif
#ifdef __NR_timer_settime64
__NR_timer_settime64,
#endif
#ifdef __NR_timerfd_create
__NR_timerfd_create,
#endif
#ifdef __NR_timerfd_gettime
__NR_timerfd_gettime,
#endif
#ifdef __NR_timerfd_gettime64
__NR_timerfd_gettime64,
#endif
#ifdef __NR_timerfd_settime
__NR_timerfd_settime,
#endif
#ifdef __NR_timerfd_settime64
__NR_timerfd_settime64,
#endif
#ifdef __NR_times
__NR_times,
#endif
#ifdef __NR_tkill
__NR_tkill,
#endif
#ifdef __NR_truncate
__NR_truncate,
#endif
#ifdef __NR_truncate64
__NR_truncate64,
#endif
#ifdef __NR_tuxcall
__NR_tuxcall,
#endif
#ifdef __NR_ugetrlimit
__NR_ugetrlimit,
#endif
#ifdef __NR_ulimit
__NR_ulimit,
#endif
#ifdef __NR_umask
__NR_umask,
#endif
#ifdef __NR_umount
__NR_umount,
#endif
#ifdef __NR_umount2
__NR_umount2,
#endif
#ifdef __NR_uname
__NR_uname,
#endif
#ifdef __NR_unlink
__NR_unlink,
#endif
#ifdef __NR_unlinkat
__NR_unlinkat,
#endif
#ifdef __NR_unshare
__NR_unshare,
#endif
#ifdef __NR_uselib
__NR_uselib,
#endif
#ifdef __NR_userfaultfd
__NR_userfaultfd,
#endif
#ifdef __NR_ustat
__NR_ustat,
#endif
#ifdef __NR_utime
__NR_utime,
#endif
#ifdef __NR_utimensat
__NR_utimensat,
#endif
#ifdef __NR_utimensat_time64
__NR_utimensat_time64,
#endif
#ifdef __NR_utimes
__NR_utimes,
#endif
#ifdef __NR_vfork
__NR_vfork,
#endif
#ifdef __NR_vhangup
__NR_vhangup,
#endif
#ifdef __NR_vm86
__NR_vm86,
#endif
#ifdef __NR_vm86old
__NR_vm86old,
#endif
#ifdef __NR_vmsplice
__NR_vmsplice,
#endif
#ifdef __NR_vserver
__NR_vserver,
#endif
#ifdef __NR_wait4
__NR_wait4,
#endif
#ifdef __NR_waitid
__NR_waitid,
#endif
#ifdef __NR_waitpid
__NR_waitpid,
#endif
#ifdef __NR_write
__NR_write,
#endif
#ifdef __NR_writev
__NR_writev,
#endif
0})+1;

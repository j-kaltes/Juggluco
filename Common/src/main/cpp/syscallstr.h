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


#ifdef __NR_newselect
syscallstr[__NR_newselect]="newselect";
#endif
#ifdef __NR_sysctl
syscallstr[__NR_sysctl]="sysctl";
#endif

#ifdef __NR__llseek
syscallstr[__NR__llseek]="_llseek";
#endif
#ifdef __NR__newselect
syscallstr[__NR__newselect]="_newselect";
#endif
#ifdef __NR__sysctl
syscallstr[__NR__sysctl]="_sysctl";
#endif
#ifdef __NR_accept
syscallstr[__NR_accept]="accept";
#endif
#ifdef __NR_accept4
syscallstr[__NR_accept4]="accept4";
#endif
#ifdef __NR_access
syscallstr[__NR_access]="access";
#endif
#ifdef __NR_acct
syscallstr[__NR_acct]="acct";
#endif
#ifdef __NR_add_key
syscallstr[__NR_add_key]="add_key";
#endif
#ifdef __NR_adjtimex
syscallstr[__NR_adjtimex]="adjtimex";
#endif
#ifdef __NR_afs_syscall
syscallstr[__NR_afs_syscall]="afs_syscall";
#endif
#ifdef __NR_alarm
syscallstr[__NR_alarm]="alarm";
#endif
#ifdef __NR_arch_prctl
syscallstr[__NR_arch_prctl]="arch_prctl";
#endif
#ifdef __NR_arch_specific_syscall
syscallstr[__NR_arch_specific_syscall]="arch_specific_syscall";
#endif
#ifdef __NR_arm_fadvise64_64
syscallstr[__NR_arm_fadvise64_64]="arm_fadvise64_64";
#endif
#ifdef __NR_arm_sync_file_range
syscallstr[__NR_arm_sync_file_range]="arm_sync_file_range";
#endif
#ifdef __NR_bdflush
syscallstr[__NR_bdflush]="bdflush";
#endif
#ifdef __NR_bind
syscallstr[__NR_bind]="bind";
#endif
#ifdef __NR_bpf
syscallstr[__NR_bpf]="bpf";
#endif
#ifdef __NR_break
syscallstr[__NR_break]="break";
#endif
#ifdef __NR_brk
syscallstr[__NR_brk]="brk";
#endif
#ifdef __NR_capget
syscallstr[__NR_capget]="capget";
#endif
#ifdef __NR_capset
syscallstr[__NR_capset]="capset";
#endif
#ifdef __NR_chdir
syscallstr[__NR_chdir]="chdir";
#endif
#ifdef __NR_chmod
syscallstr[__NR_chmod]="chmod";
#endif
#ifdef __NR_chown
syscallstr[__NR_chown]="chown";
#endif
#ifdef __NR_chown32
syscallstr[__NR_chown32]="chown32";
#endif
#ifdef __NR_chroot
syscallstr[__NR_chroot]="chroot";
#endif
#ifdef __NR_clock_adjtime
syscallstr[__NR_clock_adjtime]="clock_adjtime";
#endif
#ifdef __NR_clock_adjtime64
syscallstr[__NR_clock_adjtime64]="clock_adjtime64";
#endif
#ifdef __NR_clock_getres
syscallstr[__NR_clock_getres]="clock_getres";
#endif
#ifdef __NR_clock_getres_time64
syscallstr[__NR_clock_getres_time64]="clock_getres_time64";
#endif
#ifdef __NR_clock_gettime
syscallstr[__NR_clock_gettime]="clock_gettime";
#endif
#ifdef __NR_clock_gettime64
syscallstr[__NR_clock_gettime64]="clock_gettime64";
#endif
#ifdef __NR_clock_nanosleep
syscallstr[__NR_clock_nanosleep]="clock_nanosleep";
#endif
#ifdef __NR_clock_nanosleep_time64
syscallstr[__NR_clock_nanosleep_time64]="clock_nanosleep_time64";
#endif
#ifdef __NR_clock_settime
syscallstr[__NR_clock_settime]="clock_settime";
#endif
#ifdef __NR_clock_settime64
syscallstr[__NR_clock_settime64]="clock_settime64";
#endif
#ifdef __NR_clone
syscallstr[__NR_clone]="clone";
#endif
#ifdef __NR_clone3
syscallstr[__NR_clone3]="clone3";
#endif
#ifdef __NR_close
syscallstr[__NR_close]="close";
#endif
#ifdef __NR_close_range
syscallstr[__NR_close_range]="close_range";
#endif
#ifdef __NR_connect
syscallstr[__NR_connect]="connect";
#endif
#ifdef __NR_copy_file_range
syscallstr[__NR_copy_file_range]="copy_file_range";
#endif
#ifdef __NR_creat
syscallstr[__NR_creat]="creat";
#endif
#ifdef __NR_create_module
syscallstr[__NR_create_module]="create_module";
#endif
#ifdef __NR_delete_module
syscallstr[__NR_delete_module]="delete_module";
#endif
#ifdef __NR_dup
syscallstr[__NR_dup]="dup";
#endif
#ifdef __NR_dup2
syscallstr[__NR_dup2]="dup2";
#endif
#ifdef __NR_dup3
syscallstr[__NR_dup3]="dup3";
#endif
#ifdef __NR_epoll_create
syscallstr[__NR_epoll_create]="epoll_create";
#endif
#ifdef __NR_epoll_create1
syscallstr[__NR_epoll_create1]="epoll_create1";
#endif
#ifdef __NR_epoll_ctl
syscallstr[__NR_epoll_ctl]="epoll_ctl";
#endif
#ifdef __NR_epoll_ctl_old
syscallstr[__NR_epoll_ctl_old]="epoll_ctl_old";
#endif
#ifdef __NR_epoll_pwait
syscallstr[__NR_epoll_pwait]="epoll_pwait";
#endif
#ifdef __NR_epoll_wait
syscallstr[__NR_epoll_wait]="epoll_wait";
#endif
#ifdef __NR_epoll_wait_old
syscallstr[__NR_epoll_wait_old]="epoll_wait_old";
#endif
#ifdef __NR_eventfd
syscallstr[__NR_eventfd]="eventfd";
#endif
#ifdef __NR_eventfd2
syscallstr[__NR_eventfd2]="eventfd2";
#endif
#ifdef __NR_execve
syscallstr[__NR_execve]="execve";
#endif
#ifdef __NR_execveat
syscallstr[__NR_execveat]="execveat";
#endif
#ifdef __NR_exit
syscallstr[__NR_exit]="exit";
#endif
#ifdef __NR_exit_group
syscallstr[__NR_exit_group]="exit_group";
#endif
#ifdef __NR_faccessat
syscallstr[__NR_faccessat]="faccessat";
#endif
#ifdef __NR_faccessat2
syscallstr[__NR_faccessat2]="faccessat2";
#endif
#ifdef __NR_fadvise64
syscallstr[__NR_fadvise64]="fadvise64";
#endif
#ifdef __NR_fadvise64_64
syscallstr[__NR_fadvise64_64]="fadvise64_64";
#endif
#ifdef __NR_fallocate
syscallstr[__NR_fallocate]="fallocate";
#endif
#ifdef __NR_fanotify_init
syscallstr[__NR_fanotify_init]="fanotify_init";
#endif
#ifdef __NR_fanotify_mark
syscallstr[__NR_fanotify_mark]="fanotify_mark";
#endif
#ifdef __NR_fchdir
syscallstr[__NR_fchdir]="fchdir";
#endif
#ifdef __NR_fchmod
syscallstr[__NR_fchmod]="fchmod";
#endif
#ifdef __NR_fchmodat
syscallstr[__NR_fchmodat]="fchmodat";
#endif
#ifdef __NR_fchown
syscallstr[__NR_fchown]="fchown";
#endif
#ifdef __NR_fchown32
syscallstr[__NR_fchown32]="fchown32";
#endif
#ifdef __NR_fchownat
syscallstr[__NR_fchownat]="fchownat";
#endif
#ifdef __NR_fcntl
syscallstr[__NR_fcntl]="fcntl";
#endif
#ifdef __NR_fcntl64
syscallstr[__NR_fcntl64]="fcntl64";
#endif
#ifdef __NR_fdatasync
syscallstr[__NR_fdatasync]="fdatasync";
#endif
#ifdef __NR_fgetxattr
syscallstr[__NR_fgetxattr]="fgetxattr";
#endif
#ifdef __NR_finit_module
syscallstr[__NR_finit_module]="finit_module";
#endif
#ifdef __NR_flistxattr
syscallstr[__NR_flistxattr]="flistxattr";
#endif
#ifdef __NR_flock
syscallstr[__NR_flock]="flock";
#endif
#ifdef __NR_fork
syscallstr[__NR_fork]="fork";
#endif
#ifdef __NR_fremovexattr
syscallstr[__NR_fremovexattr]="fremovexattr";
#endif
#ifdef __NR_fsconfig
syscallstr[__NR_fsconfig]="fsconfig";
#endif
#ifdef __NR_fsetxattr
syscallstr[__NR_fsetxattr]="fsetxattr";
#endif
#ifdef __NR_fsmount
syscallstr[__NR_fsmount]="fsmount";
#endif
#ifdef __NR_fsopen
syscallstr[__NR_fsopen]="fsopen";
#endif
#ifdef __NR_fspick
syscallstr[__NR_fspick]="fspick";
#endif
#ifdef __NR_fstat
syscallstr[__NR_fstat]="fstat";
#endif
#ifdef __NR_fstat64
syscallstr[__NR_fstat64]="fstat64";
#endif
#ifdef __NR_fstatat64
syscallstr[__NR_fstatat64]="fstatat64";
#endif
#ifdef __NR_fstatfs
syscallstr[__NR_fstatfs]="fstatfs";
#endif
#ifdef __NR_fstatfs64
syscallstr[__NR_fstatfs64]="fstatfs64";
#endif
#ifdef __NR_fsync
syscallstr[__NR_fsync]="fsync";
#endif
#ifdef __NR_ftime
syscallstr[__NR_ftime]="ftime";
#endif
#ifdef __NR_ftruncate
syscallstr[__NR_ftruncate]="ftruncate";
#endif
#ifdef __NR_ftruncate64
syscallstr[__NR_ftruncate64]="ftruncate64";
#endif
#ifdef __NR_futex
syscallstr[__NR_futex]="futex";
#endif
#ifdef __NR_futex_time64
syscallstr[__NR_futex_time64]="futex_time64";
#endif
#ifdef __NR_futimesat
syscallstr[__NR_futimesat]="futimesat";
#endif
#ifdef __NR_get_kernel_syms
syscallstr[__NR_get_kernel_syms]="get_kernel_syms";
#endif
#ifdef __NR_get_mempolicy
syscallstr[__NR_get_mempolicy]="get_mempolicy";
#endif
#ifdef __NR_get_robust_list
syscallstr[__NR_get_robust_list]="get_robust_list";
#endif
#ifdef __NR_get_thread_area
syscallstr[__NR_get_thread_area]="get_thread_area";
#endif
#ifdef __NR_getcpu
syscallstr[__NR_getcpu]="getcpu";
#endif
#ifdef __NR_getcwd
syscallstr[__NR_getcwd]="getcwd";
#endif
#ifdef __NR_getdents
syscallstr[__NR_getdents]="getdents";
#endif
#ifdef __NR_getdents64
syscallstr[__NR_getdents64]="getdents64";
#endif
#ifdef __NR_getegid
syscallstr[__NR_getegid]="getegid";
#endif
#ifdef __NR_getegid32
syscallstr[__NR_getegid32]="getegid32";
#endif
#ifdef __NR_geteuid
syscallstr[__NR_geteuid]="geteuid";
#endif
#ifdef __NR_geteuid32
syscallstr[__NR_geteuid32]="geteuid32";
#endif
#ifdef __NR_getgid
syscallstr[__NR_getgid]="getgid";
#endif
#ifdef __NR_getgid32
syscallstr[__NR_getgid32]="getgid32";
#endif
#ifdef __NR_getgroups
syscallstr[__NR_getgroups]="getgroups";
#endif
#ifdef __NR_getgroups32
syscallstr[__NR_getgroups32]="getgroups32";
#endif
#ifdef __NR_getitimer
syscallstr[__NR_getitimer]="getitimer";
#endif
#ifdef __NR_getpeername
syscallstr[__NR_getpeername]="getpeername";
#endif
#ifdef __NR_getpgid
syscallstr[__NR_getpgid]="getpgid";
#endif
#ifdef __NR_getpgrp
syscallstr[__NR_getpgrp]="getpgrp";
#endif
#ifdef __NR_getpid
syscallstr[__NR_getpid]="getpid";
#endif
#ifdef __NR_getpmsg
syscallstr[__NR_getpmsg]="getpmsg";
#endif
#ifdef __NR_getppid
syscallstr[__NR_getppid]="getppid";
#endif
#ifdef __NR_getpriority
syscallstr[__NR_getpriority]="getpriority";
#endif
#ifdef __NR_getrandom
syscallstr[__NR_getrandom]="getrandom";
#endif
#ifdef __NR_getresgid
syscallstr[__NR_getresgid]="getresgid";
#endif
#ifdef __NR_getresgid32
syscallstr[__NR_getresgid32]="getresgid32";
#endif
#ifdef __NR_getresuid
syscallstr[__NR_getresuid]="getresuid";
#endif
#ifdef __NR_getresuid32
syscallstr[__NR_getresuid32]="getresuid32";
#endif
#ifdef __NR_getrlimit
syscallstr[__NR_getrlimit]="getrlimit";
#endif
#ifdef __NR_getrusage
syscallstr[__NR_getrusage]="getrusage";
#endif
#ifdef __NR_getsid
syscallstr[__NR_getsid]="getsid";
#endif
#ifdef __NR_getsockname
syscallstr[__NR_getsockname]="getsockname";
#endif
#ifdef __NR_getsockopt
syscallstr[__NR_getsockopt]="getsockopt";
#endif
#ifdef __NR_gettid
syscallstr[__NR_gettid]="gettid";
#endif
#ifdef __NR_gettimeofday
syscallstr[__NR_gettimeofday]="gettimeofday";
#endif
#ifdef __NR_getuid
syscallstr[__NR_getuid]="getuid";
#endif
#ifdef __NR_getuid32
syscallstr[__NR_getuid32]="getuid32";
#endif
#ifdef __NR_getxattr
syscallstr[__NR_getxattr]="getxattr";
#endif
#ifdef __NR_gtty
syscallstr[__NR_gtty]="gtty";
#endif
#ifdef __NR_idle
syscallstr[__NR_idle]="idle";
#endif
#ifdef __NR_init_module
syscallstr[__NR_init_module]="init_module";
#endif
#ifdef __NR_inotify_add_watch
syscallstr[__NR_inotify_add_watch]="inotify_add_watch";
#endif
#ifdef __NR_inotify_init
syscallstr[__NR_inotify_init]="inotify_init";
#endif
#ifdef __NR_inotify_init1
syscallstr[__NR_inotify_init1]="inotify_init1";
#endif
#ifdef __NR_inotify_rm_watch
syscallstr[__NR_inotify_rm_watch]="inotify_rm_watch";
#endif
#ifdef __NR_io_cancel
syscallstr[__NR_io_cancel]="io_cancel";
#endif
#ifdef __NR_io_destroy
syscallstr[__NR_io_destroy]="io_destroy";
#endif
#ifdef __NR_io_getevents
syscallstr[__NR_io_getevents]="io_getevents";
#endif
#ifdef __NR_io_pgetevents
syscallstr[__NR_io_pgetevents]="io_pgetevents";
#endif
#ifdef __NR_io_pgetevents_time64
syscallstr[__NR_io_pgetevents_time64]="io_pgetevents_time64";
#endif
#ifdef __NR_io_setup
syscallstr[__NR_io_setup]="io_setup";
#endif
#ifdef __NR_io_submit
syscallstr[__NR_io_submit]="io_submit";
#endif
#ifdef __NR_io_uring_enter
syscallstr[__NR_io_uring_enter]="io_uring_enter";
#endif
#ifdef __NR_io_uring_register
syscallstr[__NR_io_uring_register]="io_uring_register";
#endif
#ifdef __NR_io_uring_setup
syscallstr[__NR_io_uring_setup]="io_uring_setup";
#endif
#ifdef __NR_ioctl
syscallstr[__NR_ioctl]="ioctl";
#endif
#ifdef __NR_ioperm
syscallstr[__NR_ioperm]="ioperm";
#endif
#ifdef __NR_iopl
syscallstr[__NR_iopl]="iopl";
#endif
#ifdef __NR_ioprio_get
syscallstr[__NR_ioprio_get]="ioprio_get";
#endif
#ifdef __NR_ioprio_set
syscallstr[__NR_ioprio_set]="ioprio_set";
#endif
#ifdef __NR_ipc
syscallstr[__NR_ipc]="ipc";
#endif
#ifdef __NR_kcmp
syscallstr[__NR_kcmp]="kcmp";
#endif
#ifdef __NR_kexec_file_load
syscallstr[__NR_kexec_file_load]="kexec_file_load";
#endif
#ifdef __NR_kexec_load
syscallstr[__NR_kexec_load]="kexec_load";
#endif
#ifdef __NR_keyctl
syscallstr[__NR_keyctl]="keyctl";
#endif
#ifdef __NR_kill
syscallstr[__NR_kill]="kill";
#endif
#ifdef __NR_lchown
syscallstr[__NR_lchown]="lchown";
#endif
#ifdef __NR_lchown32
syscallstr[__NR_lchown32]="lchown32";
#endif
#ifdef __NR_lgetxattr
syscallstr[__NR_lgetxattr]="lgetxattr";
#endif
#ifdef __NR_link
syscallstr[__NR_link]="link";
#endif
#ifdef __NR_linkat
syscallstr[__NR_linkat]="linkat";
#endif
#ifdef __NR_listen
syscallstr[__NR_listen]="listen";
#endif
#ifdef __NR_listxattr
syscallstr[__NR_listxattr]="listxattr";
#endif
#ifdef __NR_llistxattr
syscallstr[__NR_llistxattr]="llistxattr";
#endif
#ifdef __NR_llseek
syscallstr[__NR_llseek]="llseek";
#endif
#ifdef __NR_lock
syscallstr[__NR_lock]="lock";
#endif
#ifdef __NR_lookup_dcookie
syscallstr[__NR_lookup_dcookie]="lookup_dcookie";
#endif
#ifdef __NR_lremovexattr
syscallstr[__NR_lremovexattr]="lremovexattr";
#endif
#ifdef __NR_lseek
syscallstr[__NR_lseek]="lseek";
#endif
#ifdef __NR_lsetxattr
syscallstr[__NR_lsetxattr]="lsetxattr";
#endif
#ifdef __NR_lstat
syscallstr[__NR_lstat]="lstat";
#endif
#ifdef __NR_lstat64
syscallstr[__NR_lstat64]="lstat64";
#endif
#ifdef __NR_madvise
syscallstr[__NR_madvise]="madvise";
#endif
#ifdef __NR_mbind
syscallstr[__NR_mbind]="mbind";
#endif
#ifdef __NR_membarrier
syscallstr[__NR_membarrier]="membarrier";
#endif
#ifdef __NR_memfd_create
syscallstr[__NR_memfd_create]="memfd_create";
#endif
#ifdef __NR_migrate_pages
syscallstr[__NR_migrate_pages]="migrate_pages";
#endif
#ifdef __NR_mincore
syscallstr[__NR_mincore]="mincore";
#endif
#ifdef __NR_mkdir
syscallstr[__NR_mkdir]="mkdir";
#endif
#ifdef __NR_mkdirat
syscallstr[__NR_mkdirat]="mkdirat";
#endif
#ifdef __NR_mknod
syscallstr[__NR_mknod]="mknod";
#endif
#ifdef __NR_mknodat
syscallstr[__NR_mknodat]="mknodat";
#endif
#ifdef __NR_mlock
syscallstr[__NR_mlock]="mlock";
#endif
#ifdef __NR_mlock2
syscallstr[__NR_mlock2]="mlock2";
#endif
#ifdef __NR_mlockall
syscallstr[__NR_mlockall]="mlockall";
#endif
#ifdef __NR_mmap
syscallstr[__NR_mmap]="mmap";
#endif
#ifdef __NR_mmap2
syscallstr[__NR_mmap2]="mmap2";
#endif
#ifdef __NR_modify_ldt
syscallstr[__NR_modify_ldt]="modify_ldt";
#endif
#ifdef __NR_mount
syscallstr[__NR_mount]="mount";
#endif
#ifdef __NR_move_mount
syscallstr[__NR_move_mount]="move_mount";
#endif
#ifdef __NR_move_pages
syscallstr[__NR_move_pages]="move_pages";
#endif
#ifdef __NR_mprotect
syscallstr[__NR_mprotect]="mprotect";
#endif
#ifdef __NR_mpx
syscallstr[__NR_mpx]="mpx";
#endif
#ifdef __NR_mq_getsetattr
syscallstr[__NR_mq_getsetattr]="mq_getsetattr";
#endif
#ifdef __NR_mq_notify
syscallstr[__NR_mq_notify]="mq_notify";
#endif
#ifdef __NR_mq_open
syscallstr[__NR_mq_open]="mq_open";
#endif
#ifdef __NR_mq_timedreceive
syscallstr[__NR_mq_timedreceive]="mq_timedreceive";
#endif
#ifdef __NR_mq_timedreceive_time64
syscallstr[__NR_mq_timedreceive_time64]="mq_timedreceive_time64";
#endif
#ifdef __NR_mq_timedsend
syscallstr[__NR_mq_timedsend]="mq_timedsend";
#endif
#ifdef __NR_mq_timedsend_time64
syscallstr[__NR_mq_timedsend_time64]="mq_timedsend_time64";
#endif
#ifdef __NR_mq_unlink
syscallstr[__NR_mq_unlink]="mq_unlink";
#endif
#ifdef __NR_mremap
syscallstr[__NR_mremap]="mremap";
#endif
#ifdef __NR_msgctl
syscallstr[__NR_msgctl]="msgctl";
#endif
#ifdef __NR_msgget
syscallstr[__NR_msgget]="msgget";
#endif
#ifdef __NR_msgrcv
syscallstr[__NR_msgrcv]="msgrcv";
#endif
#ifdef __NR_msgsnd
syscallstr[__NR_msgsnd]="msgsnd";
#endif
#ifdef __NR_msync
syscallstr[__NR_msync]="msync";
#endif
#ifdef __NR_munlock
syscallstr[__NR_munlock]="munlock";
#endif
#ifdef __NR_munlockall
syscallstr[__NR_munlockall]="munlockall";
#endif
#ifdef __NR_munmap
syscallstr[__NR_munmap]="munmap";
#endif
#ifdef __NR_name_to_handle_at
syscallstr[__NR_name_to_handle_at]="name_to_handle_at";
#endif
#ifdef __NR_nanosleep
syscallstr[__NR_nanosleep]="nanosleep";
#endif
#ifdef __NR_newfstatat
syscallstr[__NR_newfstatat]="newfstatat";
#endif
#ifdef __NR_nfsservctl
syscallstr[__NR_nfsservctl]="nfsservctl";
#endif
#ifdef __NR_nice
syscallstr[__NR_nice]="nice";
#endif
#ifdef __NR_oldfstat
syscallstr[__NR_oldfstat]="oldfstat";
#endif
#ifdef __NR_oldlstat
syscallstr[__NR_oldlstat]="oldlstat";
#endif
#ifdef __NR_oldolduname
syscallstr[__NR_oldolduname]="oldolduname";
#endif
#ifdef __NR_oldstat
syscallstr[__NR_oldstat]="oldstat";
#endif
#ifdef __NR_olduname
syscallstr[__NR_olduname]="olduname";
#endif
#ifdef __NR_open
syscallstr[__NR_open]="open";
#endif
#ifdef __NR_open_by_handle_at
syscallstr[__NR_open_by_handle_at]="open_by_handle_at";
#endif
#ifdef __NR_open_tree
syscallstr[__NR_open_tree]="open_tree";
#endif
#ifdef __NR_openat
syscallstr[__NR_openat]="openat";
#endif
#ifdef __NR_openat2
syscallstr[__NR_openat2]="openat2";
#endif
#ifdef __NR_pause
syscallstr[__NR_pause]="pause";
#endif
#ifdef __NR_pciconfig_iobase
syscallstr[__NR_pciconfig_iobase]="pciconfig_iobase";
#endif
#ifdef __NR_pciconfig_read
syscallstr[__NR_pciconfig_read]="pciconfig_read";
#endif
#ifdef __NR_pciconfig_write
syscallstr[__NR_pciconfig_write]="pciconfig_write";
#endif
#ifdef __NR_perf_event_open
syscallstr[__NR_perf_event_open]="perf_event_open";
#endif
#ifdef __NR_personality
syscallstr[__NR_personality]="personality";
#endif
#ifdef __NR_pidfd_getfd
syscallstr[__NR_pidfd_getfd]="pidfd_getfd";
#endif
#ifdef __NR_pidfd_open
syscallstr[__NR_pidfd_open]="pidfd_open";
#endif
#ifdef __NR_pidfd_send_signal
syscallstr[__NR_pidfd_send_signal]="pidfd_send_signal";
#endif
#ifdef __NR_pipe
syscallstr[__NR_pipe]="pipe";
#endif
#ifdef __NR_pipe2
syscallstr[__NR_pipe2]="pipe2";
#endif
#ifdef __NR_pivot_root
syscallstr[__NR_pivot_root]="pivot_root";
#endif
#ifdef __NR_pkey_alloc
syscallstr[__NR_pkey_alloc]="pkey_alloc";
#endif
#ifdef __NR_pkey_free
syscallstr[__NR_pkey_free]="pkey_free";
#endif
#ifdef __NR_pkey_mprotect
syscallstr[__NR_pkey_mprotect]="pkey_mprotect";
#endif
#ifdef __NR_poll
syscallstr[__NR_poll]="poll";
#endif
#ifdef __NR_ppoll
syscallstr[__NR_ppoll]="ppoll";
#endif
#ifdef __NR_ppoll_time64
syscallstr[__NR_ppoll_time64]="ppoll_time64";
#endif
#ifdef __NR_prctl
syscallstr[__NR_prctl]="prctl";
#endif
#ifdef __NR_pread64
syscallstr[__NR_pread64]="pread64";
#endif
#ifdef __NR_preadv
syscallstr[__NR_preadv]="preadv";
#endif
#ifdef __NR_preadv2
syscallstr[__NR_preadv2]="preadv2";
#endif
#ifdef __NR_prlimit64
syscallstr[__NR_prlimit64]="prlimit64";
#endif
#ifdef __NR_process_vm_readv
syscallstr[__NR_process_vm_readv]="process_vm_readv";
#endif
#ifdef __NR_process_vm_writev
syscallstr[__NR_process_vm_writev]="process_vm_writev";
#endif
#ifdef __NR_prof
syscallstr[__NR_prof]="prof";
#endif
#ifdef __NR_profil
syscallstr[__NR_profil]="profil";
#endif
#ifdef __NR_pselect6
syscallstr[__NR_pselect6]="pselect6";
#endif
#ifdef __NR_pselect6_time64
syscallstr[__NR_pselect6_time64]="pselect6_time64";
#endif
#ifdef __NR_ptrace
syscallstr[__NR_ptrace]="ptrace";
#endif
#ifdef __NR_putpmsg
syscallstr[__NR_putpmsg]="putpmsg";
#endif
#ifdef __NR_pwrite64
syscallstr[__NR_pwrite64]="pwrite64";
#endif
#ifdef __NR_pwritev
syscallstr[__NR_pwritev]="pwritev";
#endif
#ifdef __NR_pwritev2
syscallstr[__NR_pwritev2]="pwritev2";
#endif
#ifdef __NR_query_module
syscallstr[__NR_query_module]="query_module";
#endif
#ifdef __NR_quotactl
syscallstr[__NR_quotactl]="quotactl";
#endif
#ifdef __NR_read
syscallstr[__NR_read]="read";
#endif
#ifdef __NR_readahead
syscallstr[__NR_readahead]="readahead";
#endif
#ifdef __NR_readdir
syscallstr[__NR_readdir]="readdir";
#endif
#ifdef __NR_readlink
syscallstr[__NR_readlink]="readlink";
#endif
#ifdef __NR_readlinkat
syscallstr[__NR_readlinkat]="readlinkat";
#endif
#ifdef __NR_readv
syscallstr[__NR_readv]="readv";
#endif
#ifdef __NR_reboot
syscallstr[__NR_reboot]="reboot";
#endif
#ifdef __NR_recv
syscallstr[__NR_recv]="recv";
#endif
#ifdef __NR_recvfrom
syscallstr[__NR_recvfrom]="recvfrom";
#endif
#ifdef __NR_recvmmsg
syscallstr[__NR_recvmmsg]="recvmmsg";
#endif
#ifdef __NR_recvmmsg_time64
syscallstr[__NR_recvmmsg_time64]="recvmmsg_time64";
#endif
#ifdef __NR_recvmsg
syscallstr[__NR_recvmsg]="recvmsg";
#endif
#ifdef __NR_remap_file_pages
syscallstr[__NR_remap_file_pages]="remap_file_pages";
#endif
#ifdef __NR_removexattr
syscallstr[__NR_removexattr]="removexattr";
#endif
#ifdef __NR_rename
syscallstr[__NR_rename]="rename";
#endif
#ifdef __NR_renameat
syscallstr[__NR_renameat]="renameat";
#endif
#ifdef __NR_renameat2
syscallstr[__NR_renameat2]="renameat2";
#endif
#ifdef __NR_request_key
syscallstr[__NR_request_key]="request_key";
#endif
#ifdef __NR_restart_syscall
syscallstr[__NR_restart_syscall]="restart_syscall";
#endif
#ifdef __NR_rmdir
syscallstr[__NR_rmdir]="rmdir";
#endif
#ifdef __NR_rseq
syscallstr[__NR_rseq]="rseq";
#endif
#ifdef __NR_rt_sigaction
syscallstr[__NR_rt_sigaction]="rt_sigaction";
#endif
#ifdef __NR_rt_sigpending
syscallstr[__NR_rt_sigpending]="rt_sigpending";
#endif
#ifdef __NR_rt_sigprocmask
syscallstr[__NR_rt_sigprocmask]="rt_sigprocmask";
#endif
#ifdef __NR_rt_sigqueueinfo
syscallstr[__NR_rt_sigqueueinfo]="rt_sigqueueinfo";
#endif
#ifdef __NR_rt_sigreturn
syscallstr[__NR_rt_sigreturn]="rt_sigreturn";
#endif
#ifdef __NR_rt_sigsuspend
syscallstr[__NR_rt_sigsuspend]="rt_sigsuspend";
#endif
#ifdef __NR_rt_sigtimedwait
syscallstr[__NR_rt_sigtimedwait]="rt_sigtimedwait";
#endif
#ifdef __NR_rt_sigtimedwait_time64
syscallstr[__NR_rt_sigtimedwait_time64]="rt_sigtimedwait_time64";
#endif
#ifdef __NR_rt_tgsigqueueinfo
syscallstr[__NR_rt_tgsigqueueinfo]="rt_tgsigqueueinfo";
#endif
#ifdef __NR_sched_get_priority_max
syscallstr[__NR_sched_get_priority_max]="sched_get_priority_max";
#endif
#ifdef __NR_sched_get_priority_min
syscallstr[__NR_sched_get_priority_min]="sched_get_priority_min";
#endif
#ifdef __NR_sched_getaffinity
syscallstr[__NR_sched_getaffinity]="sched_getaffinity";
#endif
#ifdef __NR_sched_getattr
syscallstr[__NR_sched_getattr]="sched_getattr";
#endif
#ifdef __NR_sched_getparam
syscallstr[__NR_sched_getparam]="sched_getparam";
#endif
#ifdef __NR_sched_getscheduler
syscallstr[__NR_sched_getscheduler]="sched_getscheduler";
#endif
#ifdef __NR_sched_rr_get_interval
syscallstr[__NR_sched_rr_get_interval]="sched_rr_get_interval";
#endif
#ifdef __NR_sched_rr_get_interval_time64
syscallstr[__NR_sched_rr_get_interval_time64]="sched_rr_get_interval_time64";
#endif
#ifdef __NR_sched_setaffinity
syscallstr[__NR_sched_setaffinity]="sched_setaffinity";
#endif
#ifdef __NR_sched_setattr
syscallstr[__NR_sched_setattr]="sched_setattr";
#endif
#ifdef __NR_sched_setparam
syscallstr[__NR_sched_setparam]="sched_setparam";
#endif
#ifdef __NR_sched_setscheduler
syscallstr[__NR_sched_setscheduler]="sched_setscheduler";
#endif
#ifdef __NR_sched_yield
syscallstr[__NR_sched_yield]="sched_yield";
#endif
#ifdef __NR_seccomp
syscallstr[__NR_seccomp]="seccomp";
#endif
#ifdef __NR_security
syscallstr[__NR_security]="security";
#endif
#ifdef __NR_select
syscallstr[__NR_select]="select";
#endif
#ifdef __NR_semctl
syscallstr[__NR_semctl]="semctl";
#endif
#ifdef __NR_semget
syscallstr[__NR_semget]="semget";
#endif
#ifdef __NR_semop
syscallstr[__NR_semop]="semop";
#endif
#ifdef __NR_semtimedop
syscallstr[__NR_semtimedop]="semtimedop";
#endif
#ifdef __NR_semtimedop_time64
syscallstr[__NR_semtimedop_time64]="semtimedop_time64";
#endif
#ifdef __NR_send
syscallstr[__NR_send]="send";
#endif
#ifdef __NR_sendfile
syscallstr[__NR_sendfile]="sendfile";
#endif
#ifdef __NR_sendfile64
syscallstr[__NR_sendfile64]="sendfile64";
#endif
#ifdef __NR_sendmmsg
syscallstr[__NR_sendmmsg]="sendmmsg";
#endif
#ifdef __NR_sendmsg
syscallstr[__NR_sendmsg]="sendmsg";
#endif
#ifdef __NR_sendto
syscallstr[__NR_sendto]="sendto";
#endif
#ifdef __NR_set_mempolicy
syscallstr[__NR_set_mempolicy]="set_mempolicy";
#endif
#ifdef __NR_set_robust_list
syscallstr[__NR_set_robust_list]="set_robust_list";
#endif
#ifdef __NR_set_thread_area
syscallstr[__NR_set_thread_area]="set_thread_area";
#endif
#ifdef __NR_set_tid_address
syscallstr[__NR_set_tid_address]="set_tid_address";
#endif
#ifdef __NR_setdomainname
syscallstr[__NR_setdomainname]="setdomainname";
#endif
#ifdef __NR_setfsgid
syscallstr[__NR_setfsgid]="setfsgid";
#endif
#ifdef __NR_setfsgid32
syscallstr[__NR_setfsgid32]="setfsgid32";
#endif
#ifdef __NR_setfsuid
syscallstr[__NR_setfsuid]="setfsuid";
#endif
#ifdef __NR_setfsuid32
syscallstr[__NR_setfsuid32]="setfsuid32";
#endif
#ifdef __NR_setgid
syscallstr[__NR_setgid]="setgid";
#endif
#ifdef __NR_setgid32
syscallstr[__NR_setgid32]="setgid32";
#endif
#ifdef __NR_setgroups
syscallstr[__NR_setgroups]="setgroups";
#endif
#ifdef __NR_setgroups32
syscallstr[__NR_setgroups32]="setgroups32";
#endif
#ifdef __NR_sethostname
syscallstr[__NR_sethostname]="sethostname";
#endif
#ifdef __NR_setitimer
syscallstr[__NR_setitimer]="setitimer";
#endif
#ifdef __NR_setns
syscallstr[__NR_setns]="setns";
#endif
#ifdef __NR_setpgid
syscallstr[__NR_setpgid]="setpgid";
#endif
#ifdef __NR_setpriority
syscallstr[__NR_setpriority]="setpriority";
#endif
#ifdef __NR_setregid
syscallstr[__NR_setregid]="setregid";
#endif
#ifdef __NR_setregid32
syscallstr[__NR_setregid32]="setregid32";
#endif
#ifdef __NR_setresgid
syscallstr[__NR_setresgid]="setresgid";
#endif
#ifdef __NR_setresgid32
syscallstr[__NR_setresgid32]="setresgid32";
#endif
#ifdef __NR_setresuid
syscallstr[__NR_setresuid]="setresuid";
#endif
#ifdef __NR_setresuid32
syscallstr[__NR_setresuid32]="setresuid32";
#endif
#ifdef __NR_setreuid
syscallstr[__NR_setreuid]="setreuid";
#endif
#ifdef __NR_setreuid32
syscallstr[__NR_setreuid32]="setreuid32";
#endif
#ifdef __NR_setrlimit
syscallstr[__NR_setrlimit]="setrlimit";
#endif
#ifdef __NR_setsid
syscallstr[__NR_setsid]="setsid";
#endif
#ifdef __NR_setsockopt
syscallstr[__NR_setsockopt]="setsockopt";
#endif
#ifdef __NR_settimeofday
syscallstr[__NR_settimeofday]="settimeofday";
#endif
#ifdef __NR_setuid
syscallstr[__NR_setuid]="setuid";
#endif
#ifdef __NR_setuid32
syscallstr[__NR_setuid32]="setuid32";
#endif
#ifdef __NR_setxattr
syscallstr[__NR_setxattr]="setxattr";
#endif
#ifdef __NR_sgetmask
syscallstr[__NR_sgetmask]="sgetmask";
#endif
#ifdef __NR_shmat
syscallstr[__NR_shmat]="shmat";
#endif
#ifdef __NR_shmctl
syscallstr[__NR_shmctl]="shmctl";
#endif
#ifdef __NR_shmdt
syscallstr[__NR_shmdt]="shmdt";
#endif
#ifdef __NR_shmget
syscallstr[__NR_shmget]="shmget";
#endif
#ifdef __NR_shutdown
syscallstr[__NR_shutdown]="shutdown";
#endif
#ifdef __NR_sigaction
syscallstr[__NR_sigaction]="sigaction";
#endif
#ifdef __NR_sigaltstack
syscallstr[__NR_sigaltstack]="sigaltstack";
#endif
#ifdef __NR_signal
syscallstr[__NR_signal]="signal";
#endif
#ifdef __NR_signalfd
syscallstr[__NR_signalfd]="signalfd";
#endif
#ifdef __NR_signalfd4
syscallstr[__NR_signalfd4]="signalfd4";
#endif
#ifdef __NR_sigpending
syscallstr[__NR_sigpending]="sigpending";
#endif
#ifdef __NR_sigprocmask
syscallstr[__NR_sigprocmask]="sigprocmask";
#endif
#ifdef __NR_sigreturn
syscallstr[__NR_sigreturn]="sigreturn";
#endif
#ifdef __NR_sigsuspend
syscallstr[__NR_sigsuspend]="sigsuspend";
#endif
#ifdef __NR_socket
syscallstr[__NR_socket]="socket";
#endif
#ifdef __NR_socketcall
syscallstr[__NR_socketcall]="socketcall";
#endif
#ifdef __NR_socketpair
syscallstr[__NR_socketpair]="socketpair";
#endif
#ifdef __NR_splice
syscallstr[__NR_splice]="splice";
#endif
#ifdef __NR_ssetmask
syscallstr[__NR_ssetmask]="ssetmask";
#endif
#ifdef __NR_stat
syscallstr[__NR_stat]="stat";
#endif
#ifdef __NR_stat64
syscallstr[__NR_stat64]="stat64";
#endif
#ifdef __NR_statfs
syscallstr[__NR_statfs]="statfs";
#endif
#ifdef __NR_statfs64
syscallstr[__NR_statfs64]="statfs64";
#endif
#ifdef __NR_statx
syscallstr[__NR_statx]="statx";
#endif
#ifdef __NR_stime
syscallstr[__NR_stime]="stime";
#endif
#ifdef __NR_stty
syscallstr[__NR_stty]="stty";
#endif
#ifdef __NR_swapoff
syscallstr[__NR_swapoff]="swapoff";
#endif
#ifdef __NR_swapon
syscallstr[__NR_swapon]="swapon";
#endif
#ifdef __NR_symlink
syscallstr[__NR_symlink]="symlink";
#endif
#ifdef __NR_symlinkat
syscallstr[__NR_symlinkat]="symlinkat";
#endif
#ifdef __NR_sync
syscallstr[__NR_sync]="sync";
#endif
#ifdef __NR_sync_file_range
syscallstr[__NR_sync_file_range]="sync_file_range";
#endif
#ifdef __NR_sync_file_range2
syscallstr[__NR_sync_file_range2]="sync_file_range2";
#endif
#ifdef __NR_syncfs
syscallstr[__NR_syncfs]="syncfs";
#endif
#ifdef __NR_syscall
syscallstr[__NR_syscall]="syscall";
#endif
#ifdef __NR_syscalls
syscallstr[__NR_syscalls]="syscalls";
#endif
#ifdef __NR_sysfs
syscallstr[__NR_sysfs]="sysfs";
#endif
#ifdef __NR_sysinfo
syscallstr[__NR_sysinfo]="sysinfo";
#endif
#ifdef __NR_syslog
syscallstr[__NR_syslog]="syslog";
#endif
#ifdef __NR_tee
syscallstr[__NR_tee]="tee";
#endif
#ifdef __NR_tgkill
syscallstr[__NR_tgkill]="tgkill";
#endif
#ifdef __NR_time
syscallstr[__NR_time]="time";
#endif
#ifdef __NR_timer_create
syscallstr[__NR_timer_create]="timer_create";
#endif
#ifdef __NR_timer_delete
syscallstr[__NR_timer_delete]="timer_delete";
#endif
#ifdef __NR_timer_getoverrun
syscallstr[__NR_timer_getoverrun]="timer_getoverrun";
#endif
#ifdef __NR_timer_gettime
syscallstr[__NR_timer_gettime]="timer_gettime";
#endif
#ifdef __NR_timer_gettime64
syscallstr[__NR_timer_gettime64]="timer_gettime64";
#endif
#ifdef __NR_timer_settime
syscallstr[__NR_timer_settime]="timer_settime";
#endif
#ifdef __NR_timer_settime64
syscallstr[__NR_timer_settime64]="timer_settime64";
#endif
#ifdef __NR_timerfd_create
syscallstr[__NR_timerfd_create]="timerfd_create";
#endif
#ifdef __NR_timerfd_gettime
syscallstr[__NR_timerfd_gettime]="timerfd_gettime";
#endif
#ifdef __NR_timerfd_gettime64
syscallstr[__NR_timerfd_gettime64]="timerfd_gettime64";
#endif
#ifdef __NR_timerfd_settime
syscallstr[__NR_timerfd_settime]="timerfd_settime";
#endif
#ifdef __NR_timerfd_settime64
syscallstr[__NR_timerfd_settime64]="timerfd_settime64";
#endif
#ifdef __NR_times
syscallstr[__NR_times]="times";
#endif
#ifdef __NR_tkill
syscallstr[__NR_tkill]="tkill";
#endif
#ifdef __NR_truncate
syscallstr[__NR_truncate]="truncate";
#endif
#ifdef __NR_truncate64
syscallstr[__NR_truncate64]="truncate64";
#endif
#ifdef __NR_tuxcall
syscallstr[__NR_tuxcall]="tuxcall";
#endif
#ifdef __NR_ugetrlimit
syscallstr[__NR_ugetrlimit]="ugetrlimit";
#endif
#ifdef __NR_ulimit
syscallstr[__NR_ulimit]="ulimit";
#endif
#ifdef __NR_umask
syscallstr[__NR_umask]="umask";
#endif
#ifdef __NR_umount
syscallstr[__NR_umount]="umount";
#endif
#ifdef __NR_umount2
syscallstr[__NR_umount2]="umount2";
#endif
#ifdef __NR_uname
syscallstr[__NR_uname]="uname";
#endif
#ifdef __NR_unlink
syscallstr[__NR_unlink]="unlink";
#endif
#ifdef __NR_unlinkat
syscallstr[__NR_unlinkat]="unlinkat";
#endif
#ifdef __NR_unshare
syscallstr[__NR_unshare]="unshare";
#endif
#ifdef __NR_uselib
syscallstr[__NR_uselib]="uselib";
#endif
#ifdef __NR_userfaultfd
syscallstr[__NR_userfaultfd]="userfaultfd";
#endif
#ifdef __NR_ustat
syscallstr[__NR_ustat]="ustat";
#endif
#ifdef __NR_utime
syscallstr[__NR_utime]="utime";
#endif
#ifdef __NR_utimensat
syscallstr[__NR_utimensat]="utimensat";
#endif
#ifdef __NR_utimensat_time64
syscallstr[__NR_utimensat_time64]="utimensat_time64";
#endif
#ifdef __NR_utimes
syscallstr[__NR_utimes]="utimes";
#endif
#ifdef __NR_vfork
syscallstr[__NR_vfork]="vfork";
#endif
#ifdef __NR_vhangup
syscallstr[__NR_vhangup]="vhangup";
#endif
#ifdef __NR_vm86
syscallstr[__NR_vm86]="vm86";
#endif
#ifdef __NR_vm86old
syscallstr[__NR_vm86old]="vm86old";
#endif
#ifdef __NR_vmsplice
syscallstr[__NR_vmsplice]="vmsplice";
#endif
#ifdef __NR_vserver
syscallstr[__NR_vserver]="vserver";
#endif
#ifdef __NR_wait4
syscallstr[__NR_wait4]="wait4";
#endif
#ifdef __NR_waitid
syscallstr[__NR_waitid]="waitid";
#endif
#ifdef __NR_waitpid
syscallstr[__NR_waitpid]="waitpid";
#endif
#ifdef __NR_write
syscallstr[__NR_write]="write";
#endif
#ifdef __NR_writev
syscallstr[__NR_writev]="writev";
#endif

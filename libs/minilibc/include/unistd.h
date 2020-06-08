/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _UNISTD_H
#define _UNISTD_H       1
#include <features.h>
#ifndef __off_t_defined 
//typedef  int __off_t;
#define __off_t_defined 
#endif
typedef __off_t off_t;
typedef int __pid_t;
typedef int ssize_t;

/* there is no function achieve */
extern int write (int __fd, __const void *__buf, int __n);
extern int read (int __fd, void *__buf,int __nbytes);
extern int lseek (int __fd, int __offset, int __whence);


extern int close (int __fd);
extern void _exit (int __status);
extern __pid_t fork (void);
extern unsigned int sleep (unsigned int __seconds);
extern int dup2 (int __fd, int __fd2);
extern int execvp (__const char *__file, char *__const __argv[]);
extern int execve (__const char *__path, char *__const __argv[],
                   char *__const __envp[]);
extern int pipe (int __pipedes[2]);


extern int kill (__pid_t __pid, int __sig);
extern int unlink (__const char *__name);
extern unsigned int usleep (unsigned int __usec); /* new */

#endif

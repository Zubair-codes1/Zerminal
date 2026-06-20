# **Zerminal Shell**

A custom UNIX shell made from scratch in C covering fork/exec with pipes, redirection and signal handling. I built this project
to understand the UNIX system better and get a feel for how syscalls are used. I intend to build a PTY terminal emulator around it to
complete the project.

## **Features**

1. Builtin commands
2. Pipes
3. I/O redirection
4. Signal Handling

## **Architecture**

Certain commands that do not change the shell's process state like mkdir, ls and others are handled using a child process that is forked
from the main shell process. This is because execvp (variant of exec()) runs a new process on the current one. If this was done on the
parent process then it would cause the shell to stop executing.

Other commands are built in to the shell. This includes cd and exit. The reason for this is that these commands affect the global state of
the shell, so they have to be carried out in the parent process because if they werent then their effects would not be seen in the shell
(as child processes eventually end).

Pipes are implemented using pipe() to create a kernel buffer with two file descriptors. Two child processes are forked — the first has its stdout redirected to the write end via dup2(), the second has its stdin redirected to the read end. Both ends are closed in every process that doesn't need them to ensure EOF is correctly signalled when the writer exits.

For signal handling I made it so that the parent process ignores any signals and is unaffected by them, however child processes turn on default signal disposition via signal(SIGINT, SIG_DFL).

For redirection, I did something similar to the pipe system but instead using only one child process and I set either its input or output
to the file that is opened.

## **Running The Program**

The program only works on UNIX based systems as the standard POSIX syscalls are used throughout. The binary file is also only tested on macOS and thus doing a complete build is preferable for other OS's.

Complete build:
```
git clone https://github.com/Zubair-codes1/Zerminal
make
./bin/zerminal
```

Or from Binary (from latest GitHub Releases):
```
./zerminal
```

## **Syntax**

```
ls
pwd
cd ..
cd ../projects
exit
ls -la
cd ~/projects
ls | grep foo
ls > output.txt
sort < input.txt
```

## **Known Limitations**

1. No multiple pipes
2. No >>
3. No tab completion

## **Roadmap**

1. Shell completion
2. PTY implementation
3. Zerminal GUI layer

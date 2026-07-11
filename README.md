# **Zerminal Shell & Terminal Emulator**

## **Badges**
![Tests](https://github.com/Zubair-codes1/Zerminal/actions/workflows/tests.yml/badge.svg)


## **Summary**

A lightweight, custom UNIX shell and graphical terminal emulator built completely from scratch in C. This project combines a low-level command-line interpreter (covering fork/exec, pipelines, redirection, and signals) with a fully-fledged desktop frontend utilizing **raylib** and a real-time **POSIX Pseudoterminal (PTY)** backend.


## **Features**

### **1. Graphical Terminal Layer (Zerminal GUI)**
* **Raylib Canvas Renderer:** Completely bypasses standard host terminal text output to paint graphics directly to an independent OS window.
* **Bit-Mask Raster Font Engine:** Uses a custom $8 \times 16$ monochrome bitmap array font to draw text character-by-bit onto the layout canvas.
* **Asynchronous Parallel Pipelines:** Runs an independent keyboard event polling engine on the main window thread while handling incoming stream data simultaneously.

### **2. Custom UNIX Shell Backend**
* **Builtin Commands:** Native support for critical shell environment modifiers (`cd`, `exit`).
* **Process Pipelines:** Inter-process communication via `pipe()` buffers with deterministic descriptor lifecycle handling.
* **I/O Redirection:** Stream capturing and routing (`>`, `<`, and append `>>`).
* **Default Signal Delegation:** Parent process ignores terminal interruptions while dynamically restoring default signal actions (`SIG_DFL`) within spawned sub-tasks.


## **Architecture Overview**

Zerminal is split cleanly into a **Frontend Desktop UI** and a **Backend Execution Shell** connected over a virtual subsystem layer.

### **The Shell Component**
Commands modifying global state (like `cd` or `exit`) are executed natively within the parent process. External binaries (like `ls` or `mkdir`) are executed via `execvp()` inside a dedicated `fork()`. Pipelines chain processes together by re-routing standard descriptors via `dup2()`, ensuring file descriptors are tightly closed to guarantee proper EOF synchronization.

### **The PTY & Threading Subsystem**
To isolate rendering lag from shell processing, execution is driven by an asynchronous decoupled architecture:
1.  **Main Thread (The Painter):** Spawns a dedicated Raylib window framework. It captures native OS window keyboard inputs, shifts raw characters into the PTY Master stream, and continuously redraws the visual character grid loop at a locked 60 FPS under a shared safety mutex.
2.  **PTY Reader Thread (The Parser):** Spawns a background worker thread tasked entirely with reading output streaming from the PTY Master. It contains a mini state machine parser that converts control behaviors (like carriage returns `\r` and line feeds `\n`), manages horizontal line-wrapping limits, and triggers screen memory shunts when content vertical boundaries scroll past maximum limits.

## **Running The Program**

The program only works on UNIX based systems as the standard POSIX syscalls are used throughout. The binary file is also only tested on macOS and thus doing a complete build is preferable for other OS's.

Complete build:
```
git clone https://github.com/Zubair-codes1/Zerminal
cd Zerminal
make
./bin/zerminal
```

Or from Binary (from latest GitHub Releases):
```
./zerminal
```


## **Syntax & Usage Guide**

```bash
ls -la
pwd
cd ../projects
ls | grep foo
ls > output.txt
sort < input.txt
exit
```

## **Known Limitations**

1. Fixed Grid Matrix: The terminal canvas size is currently hard-locked to an $80 \times 25$ grid footprint.
2. Single Pipelines Only: Command chaining is currently limited to a single pipe operator split.
3. No Tab Autocompletion: File and command path discovery are not yet integrated into the command string parser.

## **Roadmap**

1. Dynamic Terminal Window Reflow: Implement IsWindowResized() handlers paired with PTY ioctl(..., TIOCSWINSZ, ...) signals to dynamically resize rows and columns on the fly when the desktop window edge is dragged.
2. ANSI Escape Code Decoding Engine: Expand the background PTY thread parsing loop to interpret ANSI color and formatting sequences (\033[31m, etc.) to support rich terminal themes.
3. Interactive Tab Completion: Add directory inspection routines to the shell parser to facilitate native path autocompletion.
4. Multiple Command Chaining: Re-architect the process pipeline handler to support an arbitrary number of multiple chained sequential pipes (cmd1 | cmd2 | cmd3).

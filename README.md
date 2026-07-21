# **Zerminal Shell & Terminal Emulator**

## **Badges**
![Tests](https://github.com/Zubair-codes1/Zerminal/actions/workflows/tests.yml/badge.svg)

## **Summary**

A lightweight, custom UNIX shell and graphical terminal emulator built completely from scratch in C. This project combines a low-level command-line interpreter (handling process execution, pipelines, redirection, and signals) with a fully-fledged desktop frontend utilizing **Raylib** and a real-time **POSIX Pseudoterminal (PTY)** backend.

## **Features**

### **1. Graphical Terminal Layer (Zerminal GUI)**
* **Raylib Canvas Renderer:** Paints graphics directly to an independent OS window, completely bypassing host terminal output.
* **Bit-Mask Raster Font Engine:** Uses a custom 8x16 monochrome bitmap array font to draw text character-by-bit onto the layout canvas.
* **Dynamic Window Resizing & Reflow:** Handles window resize events via `IsWindowResized()` paired with PTY `ioctl(..., TIOCSWINSZ, ...)` signals to dynamically recalculate grid rows and columns on the fly.
* **ANSI Escape Sequence Parser:** Interprets incoming ANSI color and formatting escape sequences to render styled text and colored output dynamically.
* **Asynchronous Parallel Pipelines:** Runs an independent keyboard event polling engine on the main window thread while processing incoming stream data simultaneously.

### **2. Custom UNIX Shell Backend**
* **Builtin Commands:** Native support for critical shell environment modifiers (`cd`, `exit`).
* **Process Pipelines:** Inter-process communication via `pipe()` buffers with deterministic descriptor lifecycle handling.
* **I/O Redirection:** Stream capturing and routing (`>`, `<`, and `>>`).
* **Default Signal Delegation:** Parent process ignores terminal interruptions while dynamically restoring default signal actions (`SIG_DFL`) within spawned sub-tasks.

---

## **Architecture Overview**

Zerminal is split cleanly into a **Frontend Desktop UI** and a **Backend Execution Shell** connected over a virtual subsystem layer.

### **The Shell Component**
Commands modifying global state (like `cd` or `exit`) are executed natively within the parent process. External binaries (like `ls` or `mkdir`) are executed via `execvp()` inside a dedicated `fork()`. Pipelines chain processes together by re-routing standard descriptors via `dup2()`, ensuring file descriptors are tightly closed to guarantee proper EOF synchronization.

### **The PTY & Threading Subsystem**
To isolate rendering lag from shell processing, execution is driven by an asynchronous decoupled architecture:
1. **Main Thread (The Painter):** Spawns a dedicated Raylib window framework. It captures native OS window keyboard inputs, shifts raw characters into the PTY Master stream, and continuously redraws the visual character grid loop at a locked 60 FPS under a shared safety mutex.
2. **PTY Reader Thread (The Parser):** Spawns a background worker thread tasked entirely with reading output streaming from the PTY Master. It contains a mini state machine parser that interprets control behaviors (like carriage returns `\r`, line feeds `\n`, and ANSI sequences), manages horizontal line-wrapping limits, and triggers screen memory shunts when content vertical boundaries scroll past maximum limits.

---

## **Prerequisites & Dependencies**

* **OS:** macOS or Linux (POSIX compliant)
* **Compiler:** `clang` or `gcc` (with C11 support)
* **Libraries:** [Raylib](https://www.raylib.com/) (`libraylib`), `pthread`

---

## **Building & Running**

### **1. Clone the Repository**
`git clone https://github.com/Zubair-codes1/Zerminal.git`
`cd Zerminal`

### **2. Compile the Project**
Build both the main terminal application and the standalone shell binary using the provided `Makefile`:
`make`

### **3. Run the Terminal**
`make run`
(or directly via: `./bin/terminal`)

### **4. Run the Test Suite**
Zerminal features automated test suites verifying screen buffers, input handlers, and shell execution across macOS and Linux CI:
`make tests`

---

## **Syntax & Usage Guide**

* `ls -la`
* `pwd`
* `cd ../projects`
* `ls | grep foo`
* `ls > output.txt`
* `sort < input.txt`
* `exit`

---

## **Known Limitations**

1. **Single Pipelines Only:** Command chaining is currently limited to a single pipe operator split.
2. **No Tab Autocompletion:** File and command path discovery are not yet integrated into the command string parser.
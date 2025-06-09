# Bash‑Like Shell <!-- omit in toc -->

> **CSE3033 · Operating Systems**
> Marmara University — Fall 2024

A minimalist **Unix‑style command‑line shell** written in C.  The project demonstrates process creation, job control, I/O redirection, and implementation of custom built‑in commands without relying on `system()` or `execvp()`.  The code base is adapted from the skeleton provided in lab sessions and fully satisfies the specification in the programming assignment PDF.

---

## Table of Contents

1. [Usage](#usage)

   * [Job Control](#job-control)
   * [Built‑in Commands](#built‑in-commands)
   * [I/O Redirection](#io-redirection)
2. [Implementation Notes](#implementation-notes)
3. [Limitations](#limitations)
---

## Features

* **External command execution** using `fork()` + `execv()` with manual `PATH` lookup.
* **Foreground / background jobs** (`&`) with `waitpid()`‑based tracking.
* **Built‑in job control commands:**

  * `history` — stores the **last 10** commands; supports `history -i <idx>` to re‑execute.
  * `fg %<pid>` — bring a background job to the foreground.
  * `^Z` (Ctrl‑Z) — terminate current foreground job and its descendants.
  * `exit` — safe shutdown; refuses to quit while background jobs are active.
* **I/O redirection** for **stdin**, **stdout**, and **stderr**, including append (`>>`).
* Tokenizer accepts up to **128 chars / 32 arguments** per line (assignment limit).
* Extensive **error checking** and POSIX‑compliant signal handling.

---

## Usage

At the prompt `myshell: ` enter any executable in your `PATH`, optional arguments, and redirection operators.

```text
myshell: ls -l /usr/bin > listing.txt &
myshell: history
myshell: fg %4921
myshell: gcc main.c 2> build.log
myshell: <Ctrl‑D>   # exit
```

### Job Control

* Append `&` to run a command **in the background**.
* Use `fg %<pid>` to bring it to the foreground.
* Press `Ctrl‑Z` to terminate the current foreground job.

### Built‑in Commands

| Command            | Description                                                |
| ------------------ | ---------------------------------------------------------- |
| `history`          | Print last 10 commands with indices.                       |
| `history -i <idx>` | Re‑execute command at index *idx* and update history.      |
| `fg %<pid>`        | Move background process *pid* to foreground and wait.      |
| `exit`             | Quit the shell (only when no background processes remain). |

### I/O Redirection

| Syntax           | Effect                                        |
| ---------------- | --------------------------------------------- |
| `cmd > file`     | Redirect **stdout** to *file* (overwrite).    |
| `cmd >> file`    | Redirect **stdout** and **append** to *file*. |
| `cmd < file`     | Use *file* as **stdin**.                      |
| `cmd 2> file`    | Redirect **stderr** to *file*.                |
| `cmd < in > out` | Combine redirections.                         |

---

## Implementation Notes

* **Tokenizer** built on `strtok_r()`; supports quoted strings and escaped spaces.
* **PATH search** implemented manually by splitting `$PATH` and testing executable bits with `access()`.
* **Job list** stored in a simple linked list; pruned on `SIGCHLD` via non‑blocking `waitpid()`.
* **History** circular buffer (size 10) persisted in memory only.
* **Signals:** `SIGINT`, `SIGTSTP`, and `SIGCHLD` are handled explicitly; default handlers restored in child processes.

---

## Limitations

* Pipes (`|`) are **not** supported.
* No support for environment variable expansion (e.g., `$HOME`).
* History buffer is **not** written to disk; resets each session.

---

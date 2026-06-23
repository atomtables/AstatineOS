# Errno for AstatineOS base
Errnos are represented as an unsigned 8-bit integer with each actually having a semantic meaning (unlike Unix) and some only being able to be triggered by the operating system.
### 0: no error
- normal operation
## Process error codes
### 1: aborted
- the program, library, or the operating system had to syscall into abort
- only triggerable by the operating system
### 2: 
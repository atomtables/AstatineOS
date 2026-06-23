# Syscalls for AstatineOS base

we'll have a decent amount of syscalls (listed as #:name(argc))

## 0:abort(0)
- Ends current program immediately with code 1
- unlike unix, this is not a SIGABRT and is handled completely by the kernel
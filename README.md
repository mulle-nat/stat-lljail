# stat-lljail — stat(2) penetrates the lljail (Landlock) jail

Minimal, portable reproducer for a hole in the release sandbox: an lljail
allow-list that does **not** include your home directory still lets `stat(2)`
return full metadata (existence, size, mode, owner, timestamps) for paths
under it, even though `open(2)`/`opendir(3)` are denied with `EACCES`.


## Prerequisites

* `lljail` (a Landlock jail) installed and on `PATH`, or pointed at via
  `LLJAIL=/path/to/lljail`.
* A C compiler (`cc`, or set `CC=clang`).

## Files

| file | purpose |
| ---- | ------- |
| `sandbox`    | bare-bones, portable lljail wrapper |
| `statjail.c` | the reproducer: `stat` vs `open`/`opendir` on a path inside and outside the allow-list |
| `run.sh`     | compiles `statjail.c` and runs it bare and under `./sandbox` |
| `README.md`  | this file |

## Run

``` sh
sh run.sh
```

`run.sh` probes `$HOME` by default. It never assumes a fixed location: the
sandbox allow-list includes *the directory the script was unpacked into*
(read+execute, so the binary can run) plus `/usr /bin /etc /lib /lib64`, and
`/tmp` as the writable control.

### Options

| variable | default | meaning |
| -------- | ------- | ------- |
| `FORBIDDEN` | `$HOME` | path to probe (should be outside the allow-list) |
| `LLJAIL`    | `lljail` | the sandbox executable |
| `CC`        | `cc` | the compiler |

Example probing some other protected tree:

``` sh
FORBIDDEN=/etc/shadow sh run.sh   # adjust to a path your allow-list omits
```

## Example output

```
### 1. bare run — control (no jail)
forbidden path: /home/alice

-- outside the allow-list (contents must be denied) --
stat(/home/alice                 ) = OK    dir  size=4096 mode=750
opendir(/home/alice              ) = OK
stat(/home/alice/.profile        ) = OK    file size=1234 mode=644
open(/home/alice/.profile        ) = OK    fd=3

-- control: inside the allow-list (/tmp) --
stat(/tmp                        ) = OK    dir  size=4096 mode=1777
opendir(/tmp                     ) = OK

### 2. under ./sandbox — bare-bones lljail ('/home/alice' not allowed)
forbidden path: /home/alice

-- outside the allow-list (contents must be denied) --
stat(/home/alice                 ) = OK    dir  size=4096 mode=750
opendir(/home/alice              ) = FAIL  errno=13 (Permission denied)
stat(/home/alice/.profile        ) = OK    file size=1234 mode=644
open(/home/alice/.profile        ) = FAIL  errno=13 (Permission denied)

-- control: inside the allow-list (/tmp) --
stat(/tmp                        ) = OK    dir  size=4096 mode=1777
opendir(/tmp                     ) = OK
```

Step 1 is the control: without a jail everything succeeds. In step 2 `stat`
still succeeds on `/home/alice` (the hole) while `opendir`/`open` are denied.

## Why

`lljail` is a Landlock jail. Landlock mediates access through
`LANDLOCK_ACCESS_FS_*` rights on `open`, `readdir`, `truncate`, etc. There is
**no Landlock right that governs `stat(2)`** — metadata lookups are not
restricted. So a path outside the allow-list is unreachable for its
*contents*, but its *existence, size, mode, owner and timestamps* leak.

This is why `ls -l "$HOME/.profile"` still prints a line inside the jail,
while `cat "$HOME/.profile"` and `ls "$HOME"` are denied.

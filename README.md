# Transaction State Manager (C)

A small command-line program written in **C11** that stores transactions and
controls how their state changes, based on commands typed by the user.

```
CREATED  ->  PROCESSING  ->  SUCCESS
                         \->  FAILED
```

`SUCCESS` and `FAILED` are **final** states. Once a transaction reaches one of
them it can never change again.

---

## 1. Approach / Thinking

**Step 1 - Understand the problem.** We need to remember many transactions
(id, amount, state) and only allow a fixed set of state changes.

**Step 2 - Choose the data model.**
- `enum TxnState` holds the four states: `CREATED`, `PROCESSING`, `SUCCESS`, `FAILED`.
- `struct Transaction` holds one transaction: `id`, `amount`, `state`.
- All transactions are stored in one fixed-size array
  (`Transaction transactions[1000]`) with a counter `txn_count`.
  A plain array is simple, has no dynamic-memory bugs, and 1000 is far more
  than the required minimum of 100.

**Step 3 - One function per operation** (as required):

| Function | What it does |
|---|---|
| `create_transaction()` | Validates id + amount, adds a new `CREATED` transaction |
| `start_transaction()` | `CREATED -> PROCESSING` |
| `finish_transaction()` | `PROCESSING -> SUCCESS` or `FAILED` (used by both `SUCCESS` and `FAIL`) |
| `print_status()` | Prints `id \| amount \| STATE` for one transaction |
| `list_transactions()` | Prints every transaction |
| `find_transaction()` | Looks up an id in the array (returns index or -1) |
| `parse_amount()` | Safely converts text to a number and checks `> 0` |
| `handle_command()` | Splits a line into words and calls the right function |

**Step 4 - Main loop.** `main()` reads one line at a time with `fgets`, sends it
to `handle_command()`, and stops on `EXIT` (or end of input).

**Step 5 - Never crash.** Every function checks its input first and prints a
clear `Error: ...` message instead of failing:
- unknown id, duplicate id, bad amount, wrong number of arguments,
- unknown command, invalid state change,
- id too long, line too long, storage full.

Because `fgets` is used with a fixed buffer and `strtod` is used for numbers
(instead of `scanf`), bad input cannot overflow memory or crash the program.

## 2. Assumptions

- A transaction id is one word with no spaces, at most 31 characters.
  Ids are **case-sensitive** (`TXN1` and `txn1` are different).
- Commands are **case-insensitive** (`start txn1` works).
- Amount can be a whole or decimal number (`500`, `99.5`). It must be greater
  than 0. `abc`, `0`, `-5`, `nan`, `inf` are rejected.
- Whole amounts print as `500`; decimal amounts print with two digits (`99.50`).
- Maximum 1000 transactions are stored (the requirement was at least 100).
- Data lives only in memory; it is gone when the program exits.
- Blank lines are ignored.
- Extra words after a command (e.g. `STATUS A B`) show a usage error.

## 3. Build and Run (step by step)

### What you need
A C compiler, `gcc` (or `clang`). Check with:

```
gcc --version
```

If this prints a version number, you are ready.
- **Windows:** install [MSYS2](https://www.msys2.org/) or [MinGW-w64](https://www.mingw-w64.org/) (or use WSL).
- **macOS:** run `xcode-select --install`.
- **Linux (Ubuntu/Debian):** run `sudo apt install build-essential`.

### Step 1 - Get the code
```
git clone https://github.com/UksinghM/assignment-1-transaction-state-manager.git
cd assignment-1-transaction-state-manager
```

### Step 2 - Compile
Linux / macOS / Git Bash:
```
gcc -std=c11 -Wall -Wextra -pedantic -o txn_manager main.c -lm
```
Windows PowerShell:
```
gcc -std=c11 -Wall -Wextra -pedantic -o txn_manager.exe main.c -lm
```

### Step 3 - Run it
```
./txn_manager          # Linux / macOS / Git Bash
.\txn_manager.exe      # Windows PowerShell
```
The program now waits for commands. No prompt is shown, this is normal.

### Step 4 - Type commands, one per line
```
CREATE TXN101 500
START TXN101
STATUS TXN101
SUCCESS TXN101
EXIT
```
Type `EXIT` to quit.

### Run the ready-made sample input instead of typing
```
./txn_manager < sample_input.txt                      # Linux / macOS / Git Bash
Get-Content sample_input.txt | .\txn_manager.exe      # Windows PowerShell
```

### Command reference

| Command | Meaning |
|---|---|
| `CREATE <id> <amount>` | Add a new transaction in `CREATED` state |
| `START <id>` | Move `CREATED` to `PROCESSING` |
| `SUCCESS <id>` | Move `PROCESSING` to `SUCCESS` (final) |
| `FAIL <id>` | Move `PROCESSING` to `FAILED` (final) |
| `STATUS <id>` | Show `id \| amount \| STATE` |
| `LIST` | Show all transactions |
| `EXIT` | Quit the program |

## 4. Sample Run

Input (`sample_input.txt`):
```
CREATE TXN101 500
START TXN101
STATUS TXN101
SUCCESS TXN101
START TXN101
EXIT
```

Output:
```
Transaction TXN101 created
Transaction TXN101 is PROCESSING
TXN101 | 500 | PROCESSING
Transaction TXN101 is SUCCESS
Error: invalid state change
```

### Error handling examples
```
CREATE A 100        ->  Transaction A created
CREATE A 50         ->  Error: transaction A already exists
CREATE B 0          ->  Error: amount must be a number greater than 0
SUCCESS A           ->  Error: invalid state change       (A is still CREATED)
START Z             ->  Error: transaction Z not found
HELLO               ->  Error: invalid command
STATUS              ->  Error: usage: STATUS <id>
```

## 5. Project Files

```
main.c             all source code
sample_input.txt   the example from the assignment
sample_output.txt  expected output for that input
README.md          this file
.gitignore         keeps compiled programs out of the repository
```
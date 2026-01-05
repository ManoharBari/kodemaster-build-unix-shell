# Build Your Own Unix Shell

Welcome to the **Build Your Own Unix Shell** challenge!

## Quick Start

```bash
# Build your shell
make

# Run your shell
./your_program

# Test it
$ echo hello
hello
$ exit
```

## Project Structure

```
.
├── main.c          # Your shell implementation
├── Makefile        # Build configuration
├── .vizh/          # Build scripts (don't modify)
│   ├── compile.sh
│   └── run.sh
└── README.md
```

## How to Test

After making changes:

```bash
# Rebuild
make

# Run
./your_program

# Push to run automated tests
git add .
git commit -m "your message"
git push origin master
```

## Tips

- Start simple: get the REPL loop working first
- Use `printf` and `fflush(stdout)` for debugging
- The prompt should be exactly `$ ` (dollar sign + space)
- Check the step instructions for what each test expects

Good luck! 🚀

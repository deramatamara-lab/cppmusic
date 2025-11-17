# Pre-commit Hooks Setup

This project uses pre-commit hooks to enforce code quality standards before commits are made.

## Installation

1. Install pre-commit:
   ```bash
   pip install pre-commit
   ```

   Or using conda:
   ```bash
   conda install -c conda-forge pre-commit
   ```

2. Install the hooks:
   ```bash
   pre-commit install
   ```

3. (Optional) Install clang-tidy and clang-format if not already installed:
   - **Windows**: Download from LLVM releases or use Chocolatey: `choco install llvm`
   - **macOS**: `brew install llvm`
   - **Linux**: `sudo apt-get install clang-tidy clang-format` (or equivalent)

## What Gets Checked

The pre-commit hooks automatically check:

- **Code Formatting**: Ensures code matches `.clang-format` style
- **Static Analysis**: Runs `clang-tidy` with DAW-specific rules
- **File Quality**: Removes trailing whitespace, fixes line endings
- **Large Files**: Prevents accidentally committing large files
- **Merge Conflicts**: Detects unresolved merge conflict markers

## Running Manually

You can run all hooks manually:

```bash
pre-commit run --all-files
```

Or run a specific hook:

```bash
pre-commit run clang-format --all-files
pre-commit run clang-tidy --all-files
```

## Bypassing Hooks (Not Recommended)

If you absolutely must bypass hooks (e.g., for emergency fixes), use:

```bash
git commit --no-verify
```

**Warning**: This should only be used in exceptional circumstances. All code must still pass CI checks before merging.

## Troubleshooting

### clang-tidy fails with "compile_commands.json not found"

Ensure you've built the project at least once:

```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

### Hooks are slow

The hooks only run on changed files by default. If you need to check all files, use `--all-files` flag.

### Hook installation fails

Make sure you have Python 3.6+ installed and pip is up to date:

```bash
python --version
pip install --upgrade pip
```


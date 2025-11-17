# Rules Enforcement Guide

This document explains how the [DAW Development Rules](DAW_DEV_RULES.md) are enforced throughout the development workflow.

## Overview

The DAW project uses multiple layers of enforcement to ensure all code adheres to the ultra-hardened development standards:

1. **Pre-commit Hooks**: Catch issues before they enter the repository
2. **CI/CD Pipeline**: Automated checks on every push and pull request
3. **Code Review**: Mandatory checklist verification
4. **Static Analysis**: Automated detection of rule violations

## Pre-commit Hooks

Pre-commit hooks run automatically before each commit, catching common violations early.

### What Gets Checked

- **Code Formatting**: Ensures code matches `.clang-format` style
- **Static Analysis**: Runs `clang-tidy` with DAW-specific rules
- **File Quality**: Removes trailing whitespace, fixes line endings
- **Large Files**: Prevents accidentally committing large files

### Setup

See [SETUP_PRE_COMMIT.md](SETUP_PRE_COMMIT.md) for installation instructions.

### Bypassing (Not Recommended)

Pre-commit hooks can be bypassed with `git commit --no-verify`, but this should only be used in exceptional circumstances. All code must still pass CI checks.

## CI/CD Pipeline

The GitHub Actions workflow (`.github/workflows/ci.yml`) runs on every push and pull request.

### Automated Checks

1. **Code Formatting** (`format-check`)
   - Verifies all code matches `.clang-format` style
   - Runs on Windows, macOS, and Linux

2. **Build Verification** (`build-windows`, `build-macos`, `build-linux`)
   - Ensures code compiles on all platforms
   - Compiler warnings treated as errors (`-Werror`)
   - C++20 standard enforced

3. **Static Analysis** (`static-analysis`)
   - Runs `clang-tidy` with `.clang-tidy` configuration
   - Detects:
     - Raw pointers and manual memory management
     - Potential thread safety issues
     - C++20 compliance violations
     - Code quality issues

4. **Test Coverage** (`test-coverage`)
   - Generates coverage reports
   - Enforces 80%+ coverage for audio engine code
   - Fails if threshold not met

5. **Compiler Warnings** (`compiler-warnings`)
   - Tests with multiple compilers (GCC, Clang)
   - Ensures no warnings with `-Werror` enabled

### Critical Rule Enforcement

The CI pipeline specifically checks for violations of:

- **Section 1.1** (Memory Management): No raw `new`/`delete`/`malloc`/`free`
- **Section 1.2** (Thread Safety): No locks/allocations in audio thread
- **Section 1.4** (C++ Standards): C++20 compliance
- **Section 8.1** (Toolchain): Warnings as errors

## Code Review Process

Every pull request must include a completed checklist (`.github/pull_request_template.md`).

### Mandatory Checklist Items

The PR template includes all items from Section 9.2 of DAW_DEV_RULES.md:

- [ ] No raw owning pointers
- [ ] No locks/allocations in audio thread
- [ ] C++20 features used sensibly
- [ ] Public APIs documented
- [ ] Tests written and passing
- [ ] Coverage ≥80% for engine code
- [ ] UI uses design system
- [ ] Layout responsive
- [ ] Thread safety verified
- [ ] Error paths implemented
- [ ] No secrets hardcoded
- [ ] No platform hacks without guards

### Review Process

1. **Author**: Completes checklist before requesting review
2. **Reviewer**: Verifies all items are checked and accurate
3. **Approval**: Only granted when all items pass

**Violations = Automatic Rejection**: Any unchecked item or false claim results in PR rejection.

## Static Analysis Configuration

The `.clang-tidy` configuration enforces:

### Memory Safety (Section 1.1)
- `modernize-use-unique_ptr`: Enforces smart pointers
- `modernize-avoid-c-arrays`: Prefers std::array
- `cppcoreguidelines-owning-memory`: Catches raw owning pointers

### Real-Time Safety (Section 1.2, 2.1)
- While clang-tidy cannot directly detect allocations in `processBlock()`, it catches:
  - Use of `std::vector`/`std::string` in suspicious contexts
  - Lock usage patterns
  - Missing `noexcept` on RT functions

### C++20 Compliance (Section 1.4)
- Enforces modern C++ features
- Detects deprecated patterns

## Manual Verification

Some rules require manual verification during code review:

### Real-Time Safety (Section 2.1)
- **Manual Check Required**: Review all `processBlock()` implementations
- **Look For**:
  - Heap allocations (`new`, `std::vector`, `std::string`)
  - Locks (`std::mutex`, `CriticalSection`, `ScopedLock`)
  - File/network I/O
  - Logging to disk

### Architecture (Section 5.2)
- **Manual Check Required**: Verify layered architecture
- **Look For**:
  - Upward dependencies (lower layers depending on higher)
  - "God classes" mixing concerns
  - UI directly accessing audio engine internals

### Performance (Section 6)
- **Manual Check Required**: Profile critical paths
- **Tools**: Use profiling tools (Instruments, VTune, etc.)
- **Thresholds**: Verify audio thread budget met

## Enforcement Levels

Rules are categorized by enforcement level:

### CRITICAL
- **Enforcement**: Automatic rejection if violated
- **Examples**: Raw pointers, RT allocations, locks in audio thread
- **Detection**: Static analysis + code review

### ERROR
- **Enforcement**: Must be fixed before merge
- **Examples**: Missing documentation, test coverage below threshold
- **Detection**: CI checks + code review

### WARNING
- **Enforcement**: Should be addressed, but not blocking
- **Examples**: Performance optimizations, documentation improvements
- **Detection**: Code review + profiling

## Violation Handling

### Pre-commit Hooks
- **Action**: Commit blocked
- **Resolution**: Fix issues, re-commit

### CI Pipeline
- **Action**: PR marked as failed
- **Resolution**: Fix issues, push new commit

### Code Review
- **Action**: PR rejected
- **Resolution**: Address reviewer feedback, update PR

### Post-merge Discovery
- **Action**: Create issue, prioritize fix
- **Resolution**: Hotfix PR if critical, otherwise next release

## Tools Reference

### clang-format
- **Config**: `.clang-format`
- **Usage**: `clang-format -i -style=file <file>`
- **CI**: Automatic check on all C++ files

### clang-tidy
- **Config**: `.clang-tidy`
- **Usage**: `clang-tidy <file> -p build --config-file=.clang-tidy`
- **CI**: Runs on all C++ files
- **Requires**: `compile_commands.json` (generated by CMake)

### CMake
- **Config**: `CMakeLists.txt`
- **Enforces**: C++20, warnings as errors, compile commands export

### Pre-commit
- **Config**: `.pre-commit-config.yaml`
- **Usage**: `pre-commit run --all-files`
- **Installation**: See [SETUP_PRE_COMMIT.md](SETUP_PRE_COMMIT.md)

## Continuous Improvement

The enforcement system is continuously improved:

1. **New Rules**: Added to DAW_DEV_RULES.md and enforcement tools
2. **Tool Updates**: Static analysis rules refined based on findings
3. **CI Enhancements**: New checks added as needed
4. **Documentation**: Updated to reflect current practices

## Getting Help

If you're unsure about a rule or enforcement:

1. **Read**: [DAW_DEV_RULES.md](DAW_DEV_RULES.md) - the source of truth
2. **Check**: This document for enforcement details
3. **Ask**: Open a discussion or ask in code review
4. **Review**: Existing code for examples

## Summary

The enforcement system ensures that:

- ✅ No violations enter the codebase
- ✅ All code follows DAW_DEV_RULES.md
- ✅ Critical rules (RT safety, memory management) are automatically checked
- ✅ Code quality is maintained consistently
- ✅ Reviewers have clear checklists to follow

**Remember**: The rules exist to ensure zero-glitch, zero-crash audio. Violations compromise this goal and are not acceptable.


# Security Policy

This document describes the security practices, hardening measures, and vulnerability reporting process for the cppmusic DAW project.

## Build Hardening Flags

The project employs multiple layers of compile-time and link-time hardening:

### Compiler Warning Flags

All introduced code must compile cleanly with:

```
-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnull-dereference
```

### Stack Protection

```
-fstack-protector-strong
```

Enables stack canaries for functions with local buffers, protecting against stack buffer overflows.

### Fortify Source

```
-D_FORTIFY_SOURCE=2
```

Enables runtime buffer overflow detection for standard library functions (memcpy, sprintf, etc.).

### Position Independent Code

```
-fPIE -pie
```

Generates position-independent executables, enabling full ASLR (Address Space Layout Randomization).

### Relocation Read-Only

```
-Wl,-z,relro,-z,now
```

- `relro`: Makes the GOT (Global Offset Table) read-only after relocation
- `now`: Resolves all symbols at load time (full RELRO), preventing GOT overwrite attacks

### Sanitizers (Development/CI)

Optional sanitizers for catching memory and undefined behavior issues:

- **AddressSanitizer (ASAN)**: `ENABLE_ASAN=ON`
  - Detects buffer overflows, use-after-free, double-free

- **UndefinedBehaviorSanitizer (UBSAN)**: `ENABLE_UBSAN=ON`
  - Detects integer overflow, null pointer dereference, misaligned access

### Low-Latency Flags (Opt-in)

For ultra-low-latency scenarios, optional aggressive optimization flags can be enabled:

```
-ffast-math -fno-exceptions -fno-rtti
```

Enable with `ENABLE_LOW_LATENCY=ON`. **Tradeoffs**:
- `-ffast-math`: May affect IEEE floating-point compliance
- `-fno-exceptions`: Disables C++ exceptions (use noexcept everywhere)
- `-fno-rtti`: Disables RTTI (no dynamic_cast, no typeid)

Use only after evaluating impact on your specific use case.

## Runtime Security Practices

### Memory Safety

- **RAII**: All resources managed via RAII patterns
- **Smart Pointers**: `std::unique_ptr` and `std::shared_ptr` for ownership
- **No Raw `new`/`delete`**: Production code must not use raw allocation
- **Pre-allocated Buffers**: Audio thread uses pre-allocated buffers only

### Real-Time Thread Safety

- **Lock-Free Audio Thread**: No locks, allocations, or exceptions in `processBlock()`
- **Atomic Operations**: Use `std::atomic` for thread communication
- **Lock-Free Queues**: For inter-thread messaging

### Optional: Memory Locking (mlockall)

For ultra-low-latency scenarios, audio buffers can be locked into RAM:

```cpp
#include <sys/mman.h>

// Lock all current and future memory (requires CAP_IPC_LOCK or root)
if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
    // Handle error - may require elevated privileges
}
```

**Note**: This is optional and requires appropriate system permissions.

## Sandboxing Direction

### Plugin Isolation (Future)

Third-party plugins will be hosted in isolated processes:

- **Process Isolation**: Plugins run in separate processes
- **IPC Communication**: Audio/MIDI data passed via shared memory or pipes
- **Crash Isolation**: Plugin crashes do not bring down the host
- **Resource Limits**: CPU and memory limits per plugin process

### File System Access

- **Principle of Least Privilege**: Request only necessary file access
- **User-Controlled Paths**: Only access user-specified directories
- **No Network Access**: Audio processing has no network requirements

## Dependency Verification

### Third-Party Libraries

- **JUCE Framework**: Pinned to specific version (7.0.11)
- **Checksum Verification**: Dependencies verified via FetchContent or package manager
- **Minimal Dependencies**: Reduce attack surface by minimizing external code

### Supply Chain Security

- **Lock Files**: CMake FetchContent with pinned Git tags/commits
- **Reproducible Builds**: Same inputs produce identical outputs
- **CI Verification**: All dependencies verified in CI pipeline

## Vulnerability Reporting Process

### Reporting a Vulnerability

If you discover a security vulnerability, please report it responsibly:

1. **Do NOT** open a public GitHub issue for security vulnerabilities
2. **Email**: Send details to the project maintainers (contact via GitHub profile)
3. **Include**:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact assessment
   - Any suggested fixes (optional)

### Response Timeline

- **Acknowledgment**: Within 48 hours
- **Initial Assessment**: Within 7 days
- **Fix Development**: Depends on severity
- **Disclosure**: Coordinated disclosure after fix is available

### Severity Levels

| Level | Description | Response Time |
|-------|-------------|---------------|
| Critical | Remote code execution, data loss | Immediate |
| High | Privilege escalation, denial of service | 7 days |
| Medium | Information disclosure, limited impact | 30 days |
| Low | Minor issues, defense-in-depth | Next release |

## Security Testing

### Automated Testing

- **AddressSanitizer**: Run in CI for every PR
- **UndefinedBehaviorSanitizer**: Run in CI for every PR
- **Static Analysis**: clang-tidy checks for common issues
- **Fuzz Testing**: (Planned) For file format parsing

### Manual Review

- **Code Review**: All changes reviewed before merge
- **Security-Focused Review**: For sensitive areas (file I/O, plugin hosting)

## Secure Development Guidelines

### For Contributors

1. **Never commit secrets**: No API keys, passwords, or credentials in code
2. **Validate all input**: Especially file formats and user input
3. **Use safe APIs**: Prefer bounds-checked containers and functions
4. **Handle errors gracefully**: No undefined behavior on error paths
5. **Document security assumptions**: Note thread-safety and trust boundaries

### Code Patterns to Avoid

```cpp
// ❌ DON'T: Raw arrays without bounds checking
char buffer[256];
strcpy(buffer, user_input);  // Buffer overflow risk

// ✅ DO: Use safe alternatives
std::string buffer;
buffer = user_input;  // Automatically sized

// ❌ DON'T: Unchecked integer operations
int result = a * b;  // Potential overflow

// ✅ DO: Check for overflow or use safe math
if (a > 0 && b > std::numeric_limits<int>::max() / a) {
    // Handle overflow
}
```

## Compliance

This project aims to follow security best practices as outlined by:

- **CWE/SANS Top 25**: Avoiding common software weaknesses
- **OWASP**: Where applicable to desktop applications
- **C++ Core Guidelines**: Especially sections on safety

---

*Last updated: Phase 1 implementation*

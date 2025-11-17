# Contributing Guidelines

## Development Workflow

1. **Read the Rules**: Understand [DAW_DEV_RULES.md](DAW_DEV_RULES.md) - **MANDATORY**
2. **Create Branch**: Create feature branch from main
3. **Write Code**: Follow all development rules
4. **Write Tests**: Add unit tests for new functionality
5. **Code Review**: Ensure all checklist items pass
6. **Merge**: After approval, merge to main

## Code Review Checklist

Before submitting code for review, verify:

- [ ] No raw pointers or manual memory management
- [ ] No locks or allocations in audio thread (`processBlock()`)
- [ ] All public APIs documented with Doxygen comments
- [ ] Unit tests written and passing
- [ ] Follows SOLID principles
- [ ] Proper error handling
- [ ] Thread safety verified
- [ ] Performance profiled (if applicable)
- [ ] UI is accessible and responsive
- [ ] No hardcoded values (use constants)
- [ ] Code formatted with `.clang-format`
- [ ] No compiler warnings
- [ ] Cross-platform compatibility verified

## Coding Standards

### Memory Management

```cpp
// ✅ DO: Use smart pointers
auto processor = std::make_unique<AudioProcessor>();

// ❌ DON'T: Raw pointers
AudioProcessor* processor = new AudioProcessor();
```

### Audio Thread Safety

```cpp
// ✅ DO: Lock-free access
auto gain = gainAtomic.load(std::memory_order_acquire);

// ❌ DON'T: Locks in audio thread
const ScopedLock sl(lock);  // FORBIDDEN
```

### AI Processing

```cpp
// ✅ DO: Async AI processing
aiProcessor.requestAIProcessing(input);

// ❌ DON'T: AI in audio thread
auto result = aiModel.infer(buffer);  // FORBIDDEN
```

## Testing Requirements

- **Audio Processing**: >80% code coverage
- **Real-time Safety**: Verify no allocations in `processBlock()`
- **Thread Safety**: Test concurrent access patterns
- **UI Components**: Test responsiveness and accessibility

## Documentation

- All public classes and methods must have Doxygen comments
- Include parameter descriptions
- Document thread safety guarantees
- Provide usage examples for complex APIs

## Commit Messages

Use clear, descriptive commit messages:

```
feat: Add compressor effect processor

- Implements dynamic range compression
- Thread-safe parameter changes
- Real-time safe processing
- Unit tests included
```


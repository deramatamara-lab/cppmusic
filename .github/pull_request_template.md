# Pull Request

## Description

<!-- Provide a clear description of what this PR does and why it's needed -->

## Related Issue

<!-- Link to related issue, if any -->
Closes #

## Type of Change

- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update
- [ ] Performance improvement
- [ ] Refactoring

## Testing

- [ ] Unit tests added/updated
- [ ] Integration tests added/updated
- [ ] Manual testing performed
- [ ] All existing tests pass

## Code Review Checklist

**CRITICAL**: All items must be checked before merge. See [DAW_DEV_RULES.md](../../docs/DAW_DEV_RULES.md) for details.

### Memory & Thread Safety
- [ ] No raw owning pointers (`new`/`delete`, `malloc`/`free`)
- [ ] No locks/allocations in audio thread (`processBlock()`)
- [ ] Thread safety verified (no data races)
- [ ] All smart pointers used correctly (`unique_ptr`, `shared_ptr`, etc.)

### Code Quality
- [ ] C++20 features used appropriately
- [ ] Public APIs documented with Doxygen comments
- [ ] `noexcept` specified where appropriate (especially RT code)
- [ ] `[[nodiscard]]` used where ignoring return value is a bug
- [ ] No magic numbers (use constants/design system tokens)

### Testing & Coverage
- [ ] Tests written and passing
- [ ] Test coverage ≥80% for audio engine code
- [ ] Real-time safety verified (no allocations in `processBlock()` paths)
- [ ] Regression tests added for bug fixes

### UI/UX (if applicable)
- [ ] Uses design system (no magic colors/fonts/spacings)
- [ ] Layout responsive and non-janky
- [ ] Accessibility verified (keyboard navigation, screen readers)
- [ ] Performance tested (60fps target maintained)

### Architecture
- [ ] Follows layered architecture (no upward dependencies)
- [ ] No "god classes" mixing concerns
- [ ] Error paths and fallbacks implemented
- [ ] AI code (if any) runs on background threads only

### Build & Platform
- [ ] No secrets or hardcoded file paths
- [ ] No platform-specific hacks without guards/comments
- [ ] Cross-platform compatibility verified
- [ ] Compiles with warnings as errors enabled

### Documentation
- [ ] Public API changes documented
- [ ] Complex logic has inline comments
- [ ] Design decisions documented (if non-obvious)

## Automated Checks

<!-- CI will run these automatically. Check status below: -->

- [ ] Code formatting (clang-format) passes
- [ ] Static analysis (clang-tidy) passes
- [ ] All builds succeed (Windows, macOS, Linux)
- [ ] All tests pass
- [ ] Coverage threshold met (≥80% for audio engine)

## Additional Notes

<!-- Any additional information reviewers should know -->

## Checklist for Reviewers

Before approving, verify:
1. All checklist items above are checked
2. Code follows [DAW_DEV_RULES.md](../../docs/DAW_DEV_RULES.md)
3. No violations of critical rules (raw pointers, RT allocations, etc.)
4. Tests are meaningful and cover edge cases
5. Performance impact assessed (if applicable)

---

**Remember**: Violations of DAW_DEV_RULES.md are hard errors and will result in PR rejection.


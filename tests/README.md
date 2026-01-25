# https_dns_proxy Tests

This directory contains the test suite for https_dns_proxy, organized into multiple tiers for different testing needs.

## Test Organization

```
tests/
├── unit/                    # Fast, isolated unit tests
└── robot/                   # Functional tests (Robot Framework)
```

## Running Tests

### Quick Test (Recommended for CI/CD)

```bash
# Build and run all fast tests
cmake .
make
make test

# Or manually:
./test_bootstrap_source_binding  # Unit tests
python3 -m robot.run tests/robot/functional_tests.robot  # Functional tests
```

### Unit Tests (< 1 second)

Unit tests verify individual functions in isolation with mocked dependencies.

```bash
# Build and run
cmake .
make
./test_bootstrap_source_binding
```

**What's tested:**
- `set_bootstrap_source_addr()` function logic
- IPv4 and IPv6 address binding (17 test cases total)
- Edge cases: loopback, link-local, IPv4-mapped IPv6, full format
- Invalid address handling (spaces, hostnames, malformed addresses)
- Address family mismatch detection
- Special addresses (broadcast, etc.)

### Functional Tests (~ 5-10 seconds)

Robot Framework tests verify end-to-end behavior on the actual system.

```bash
# Install dependencies (once)
pip3 install robotframework

# Run tests
python3 -m robot.run tests/robot/functional_tests.robot

# Run specific test
python3 -m robot.run -t "Source Address Binding Bootstrap DNS" tests/robot/functional_tests.robot

# Run only bootstrap tests
python3 -m robot.run -i bootstrap tests/robot/functional_tests.robot
```

**What's tested:**
- Complete proxy functionality
- Source address binding for HTTPS connections (commit 67ecae0)
- Source address binding for bootstrap DNS (commit 6162723)
- DNS resolution through the proxy
- Various protocol features (HTTP/2, TCP, truncation, etc.)

## Test Coverage

### Source Address Binding Features

| Feature | Unit | Functional |
|---------|------|------------|
| IPv4 source binding | ✅ 48 tests | ✅ |
| IPv6 source binding | ✅ 48 tests | ⚠️ |
| HTTPS connection binding | ❌ | ✅ |
| Bootstrap DNS binding | ✅ 48 tests | ✅ |
| Invalid address handling | ✅ 48 tests | ✅ |
| Family mismatch detection | ✅ 48 tests | ✅ |

✅ = Fully covered
⚠️ = Depends on system configuration (IPv6 availability)
❌ = Not covered (HTTPS mocking too complex for unit tests)

## Continuous Integration

### Recommended CI/CD Pipeline

```yaml
# .github/workflows/cmake.yml
- name: Build
  run: make

- name: Unit Tests (Fast)
  run: ./test_bootstrap_source_binding

- name: Functional Tests
  run: make test ARGS="--verbose"
```

### Test Execution Times

| Test Type | Duration | When to Run |
|-----------|----------|-------------|
| Unit | < 1s | Every commit |
| Functional (Robot) | 5-10s | Every PR |

## Writing New Tests

### Adding a Unit Test

1. Create test in `tests/unit/test_*.c`
2. Add to `CMakeLists.txt`:
   ```cmake
   add_executable(test_my_feature tests/unit/test_my_feature.c)
   target_link_libraries(test_my_feature cares)
   add_test(NAME unit_my_feature COMMAND test_my_feature)
   ```

### Adding a Functional Test

1. Add test case to `tests/robot/functional_tests.robot`:
   ```robot
   My New Feature Test
       [Documentation]  Test description
       [Tags]  feature-tag
       Start Proxy  -some  -flags
       Run Dig
       # Add assertions
   ```

## Debugging Failed Tests

### Unit Test Failures

```bash
# Run with gdb
gdb ./test_bootstrap_source_binding
(gdb) run
```

### Functional Test Failures

```bash
# Check Robot Framework logs
cat log.html    # Open in browser
cat output.xml

# Run single test with verbose output
python3 -m robot.run -L DEBUG -t "Test Name" tests/robot/functional_tests.robot
```

## Test Dependencies

- **Unit tests**: Standard C library only (no external dependencies)
- **Functional tests**: `python3`, `python3-pip`, `robotframework`, `dnsutils` (dig)

## Test Philosophy

We follow the **test pyramid**:

```
       /\
      /  \  Functional (some, slower, end-to-end)
     /    \
    /------\
   /        \
  /----------\  Unit Tests (many, fast, focused)
 /______________\
```

**Guidelines:**
- Write unit tests for all new logic and validation code
- Add functional tests for user-facing features and end-to-end behavior
- Keep tests fast and CI-friendly

## Continuous Improvement

When adding new features:
1. ✅ Add unit tests first (TDD)
2. ✅ Add functional test for user-facing behavior

When fixing bugs:
1. ✅ Add test that reproduces the bug
2. ✅ Fix the bug
3. ✅ Verify test passes

## License

These tests are part of https_dns_proxy and follow the same MIT license.

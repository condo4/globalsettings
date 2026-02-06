#!/bin/bash
# Test script for += operator feature

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_ROOT="$SCRIPT_DIR/build/test_append_$$"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   GlobalSettings += Operator Test Runner              ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"
echo ""

# Build the test if needed
BUILD_DIR="$SCRIPT_DIR/build"
if [ ! -f "$BUILD_DIR/test_append_operator" ]; then
    echo -e "${GREEN}→${NC} Building test_append_operator..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    if [ ! -f "Makefile" ]; then
        cmake ..
    fi
    make test_append_operator
    echo -e "${GREEN}✅ Test built successfully${NC}"
    echo ""
fi

# Create test root directory
echo -e "${GREEN}→${NC} Creating test filesystem: $TEST_ROOT"
mkdir -p "$TEST_ROOT"/{etc,var,tmp}

# Create test configuration directories
mkdir -p "$TEST_ROOT/etc/system.conf.d"
mkdir -p "$TEST_ROOT/var/system.conf.d"
mkdir -p "$TEST_ROOT/tmp/system.conf.d"

# Create test configuration file with += operators
cat > "$TEST_ROOT/etc/system.conf" << 'EOF'
# Test string append without spaces
[General]
test_a = "foo"
test_a += "bar"

# Test string append with leading space
test_b = "foo"
test_b += " bar"

# Test string append with trailing space
test_c = "foo "
test_c += " bar"

# Test integer addition
test_d = 5
test_d += 4

# Test multiple string appends
test_multi = "one"
test_multi += "two"
test_multi += "three"

# Test multiple integer additions
test_sum = 5
test_sum += 5
test_sum += 5

# Test double addition
test_double = 3.5
test_double += 5.0

# Test mixed int and double
test_mixed = 5
test_mixed += 2.5

# Test in category
[TestCategory]
category_test = "hello"
category_test += "world"
EOF

echo -e "${GREEN}→${NC} Test configuration created"
echo ""

# Run test in namespace
echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Running += operator tests...${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
echo ""

unshare --user --map-root-user --mount --pid --fork bash -c "
    set -e
    mount --make-rprivate / 2>/dev/null
    mount --bind '$TEST_ROOT/etc' /etc 2>/dev/null
    mount --bind '$TEST_ROOT/var' /var 2>/dev/null
    mount --bind '$TEST_ROOT/tmp' /tmp 2>/dev/null
    cd '$BUILD_DIR'
    ./test_append_operator
"

exit_code=$?

# Cleanup
echo ""
echo -e "${GREEN}→${NC} Cleaning up test filesystem..."
rm -rf "$TEST_ROOT"
echo -e "${GREEN}✅ Cleanup complete${NC}"

if [ $exit_code -eq 0 ]; then
    echo ""
    echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║          ${GREEN}✅ ALL TESTS PASSED! ✅${BLUE}                    ║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"
else
    echo ""
    echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║          ${RED}❌ TESTS FAILED! ❌${BLUE}                         ║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"
fi

exit $exit_code

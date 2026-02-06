#!/bin/bash
# Script to run tests in isolated namespace with fake /etc, /tmp and /var
# This allows testing file operations without root permissions

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_ROOT="$SCRIPT_DIR/build/test_root_$$"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   GlobalSettings Namespace Test Runner                ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check if unshare is available
if ! command -v unshare &> /dev/null; then
    echo -e "${RED}ERROR: 'unshare' command not found${NC}"
    echo "This script requires util-linux package with user namespace support"
    echo "Install with: sudo apt-get install util-linux"
    exit 1
fi

# Check if user namespaces are enabled
if [ ! -f /proc/sys/kernel/unprivileged_userns_clone ]; then
    echo -e "${YELLOW}WARNING: Cannot check if user namespaces are enabled${NC}"
elif [ "$(cat /proc/sys/kernel/unprivileged_userns_clone)" != "1" ]; then
    echo -e "${RED}ERROR: Unprivileged user namespaces are disabled${NC}"
    echo "Enable with: sudo sysctl kernel.unprivileged_userns_clone=1"
    echo "Or permanently: echo 'kernel.unprivileged_userns_clone=1' | sudo tee -a /etc/sysctl.conf"
    exit 1
fi

# Check if build directory and test executable exist
BUILD_DIR="$SCRIPT_DIR/build"
LIB_BUILD_DIR="$SCRIPT_DIR/../build"

if [ ! -d "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/test_functional" ]; then
    echo -e "${YELLOW}→${NC} Build directory or test executable not found"

    # Build library first if needed
    if [ ! -f "$LIB_BUILD_DIR/libglobalsettings.so" ]; then
        echo -e "${GREEN}→${NC} Building GlobalSettings library..."
        mkdir -p "$LIB_BUILD_DIR"
        cd "$LIB_BUILD_DIR"
        if ! cmake .. ; then
            echo -e "${RED}ERROR: Failed to configure library${NC}"
            exit 1
        fi
        if ! make ; then
            echo -e "${RED}ERROR: Failed to build library${NC}"
            exit 1
        fi
        echo -e "${GREEN}✅ Library built successfully${NC}"
    else
        echo -e "${GREEN}→${NC} Library already built"
    fi

    # Build tests
    echo -e "${GREEN}→${NC} Building test suite..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    if ! cmake .. ; then
        echo -e "${RED}ERROR: Failed to configure tests${NC}"
        exit 1
    fi
    if ! make test_functional ; then
        echo -e "${RED}ERROR: Failed to build test_functional${NC}"
        exit 1
    fi
    echo -e "${GREEN}✅ Tests built successfully${NC}"
    echo ""
else
    echo -e "${GREEN}→${NC} Build directory found, tests ready"
fi

# Ensure build directory exists for TEST_ROOT
mkdir -p "$BUILD_DIR"

# Create test root directory
echo -e "${GREEN}→${NC} Creating test filesystem: $TEST_ROOT"
mkdir -p "$TEST_ROOT"/{etc,var,tmp}

# Create test configuration directories
echo -e "${GREEN}→${NC} Setting up test directories..."
mkdir -p "$TEST_ROOT/etc/system.conf.d"
mkdir -p "$TEST_ROOT/var/system.conf.d"
mkdir -p "$TEST_ROOT/tmp/system.conf.d"

# Create test configuration files
echo -e "${GREEN}→${NC} Creating test configuration files..."

# /etc/system.conf (read-only system config)
cat > "$TEST_ROOT/etc/system.conf" << 'EOF'
# System-wide configuration (read-only)
[General]
system_name = test_system
read_only = true

[Network]
hostname = test-host
domain = example.com
EOF

# /etc/system.conf.d/base.conf
cat > "$TEST_ROOT/etc/system.conf.d/base.conf" << 'EOF'
# Base configuration
[General]
version = 1.0
environment = test

[Paths]
data_dir = /var/data
log_dir = /var/log
EOF

# Create writable test config for the test application
# Note: Using test_functional as the app name (matches the binary name)
cat > "$TEST_ROOT/var/system.conf.d/test_functional.conf" << 'EOF'
# Test application configuration (writable)
[General]
@app_key = test_functional
app_key = test_value

[Settings]
@test_setting = test_functional
test_setting = initial_value
EOF

# Create volatile test config for the test application
cat > "$TEST_ROOT/tmp/system.conf.d/test_functional.conf" << 'EOF'
# Test application volatile configuration (writable)
[General]
~volatile_key = test_functional
volatile_key = volatile_test_value

[VolatileSettings]
~test_volatile_setting = test_functional
test_volatile_setting = initial_volatile_value
EOF

echo -e "${GREEN}→${NC} Test filesystem ready at: $TEST_ROOT"
echo ""

# Function to run command in namespace
run_in_namespace() {
    local cmd="$1"

    echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}Running: ${YELLOW}$cmd${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
    echo ""

    # Run with unshare:
    # --user: Create user namespace
    # --map-root-user: Map current user to root in namespace
    # --mount: Create mount namespace
    # --pid: Create PID namespace (optional, for isolation)
    # --fork: Fork before executing command
    unshare --user --map-root-user --mount --pid --fork bash -c "
        set -e

        # Remount root as private to avoid affecting host
        mount --make-rprivate /

        # Bind mount our test directories over /etc and /var
        mount --bind '$TEST_ROOT/etc' /etc
        mount --bind '$TEST_ROOT/var' /var

        # Make /var writable (it already is, but ensure it)
        # /etc remains as-is from the bind mount

        # Show what we mounted (for debugging)
        if [ \"\$VERBOSE\" = \"1\" ]; then
            echo -e '${GREEN}Mounted test filesystem:${NC}'
            echo '/etc -> $TEST_ROOT/etc'
            echo '/var -> $TEST_ROOT/var'
            echo ''
        fi

        # Run the actual command
        cd '$SCRIPT_DIR/build'
        $cmd
    "

    local exit_code=$?

    echo ""
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✅ Command succeeded${NC}"
    else
        echo -e "${RED}❌ Command failed with exit code: $exit_code${NC}"
    fi
    echo ""

    return $exit_code
}

# Function to cleanup
cleanup() {
    echo ""
    echo -e "${GREEN}→${NC} Cleaning up test filesystem..."

    # Show created files (for debugging)
    if [ "$VERBOSE" = "1" ]; then
        echo ""
        echo "Files in $TEST_ROOT/var/system.conf.d:"
        ls -la "$TEST_ROOT/var/system.conf.d/" || true
        echo ""
    fi

    # Remove test root
    rm -rf "$TEST_ROOT"

    echo -e "${GREEN}✅ Cleanup complete${NC}"
}

# Register cleanup on exit
trap cleanup EXIT

# Parse command line arguments
if [ $# -eq 0 ]; then
    # Default: run all tests
    echo -e "${GREEN}Running all functional tests...${NC}"
    echo ""

    # Run each test
    FAILED=0

    if run_in_namespace "./test_functional"; then
        echo -e "${GREEN}✅ Functional tests: PASSED${NC}"
    else
        echo -e "${RED}❌ Functional tests: FAILED${NC}"
        FAILED=$((FAILED + 1))
    fi

    echo ""
    echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
    if [ $FAILED -eq 0 ]; then
        echo -e "${BLUE}║          ${GREEN}✅ ALL TESTS PASSED! ✅${BLUE}                    ║${NC}"
    else
        echo -e "${BLUE}║          ${RED}❌ $FAILED TEST(S) FAILED! ❌${BLUE}                   ║${NC}"
    fi
    echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"

    exit $FAILED

elif [ "$1" = "--shell" ]; then
    # Interactive shell in namespace
    echo -e "${GREEN}Starting interactive shell in namespace...${NC}"
    echo -e "${YELLOW}TIP: /etc and /var point to test filesystem${NC}"
    echo -e "${YELLOW}TIP: Use 'exit' to leave the namespace${NC}"
    echo ""

    unshare --user --map-root-user --mount --pid --fork bash -c "
        mount --make-rprivate /
        mount --bind '$TEST_ROOT/etc' /etc
        mount --bind '$TEST_ROOT/var' /var

        echo -e '${GREEN}You are now in the test namespace!${NC}'
        echo ''
        echo 'Mounted:'
        echo '  /etc -> $TEST_ROOT/etc'
        echo '  /var -> $TEST_ROOT/var'
        echo ''

        cd '$SCRIPT_DIR/build'
        bash
    "

elif [ "$1" = "--verbose" ]; then
    # Run with verbose output
    export VERBOSE=1
    shift
    if [ $# -eq 0 ]; then
        # Run all tests
        exec "$0"
    else
        # Run specific command
        run_in_namespace "$*"
    fi

else
    # Run specific command
    run_in_namespace "$*"
fi

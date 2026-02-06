#!/bin/bash
# Test script for -s/--source parameter in settings command

set -e

echo "╔════════════════════════════════════════════════════════╗"
echo "║   Testing -s/--source Parameter in Settings Command   ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Create temporary test environment
TEST_ROOT="/tmp/settings_test_$$"
mkdir -p "$TEST_ROOT"/{etc,var,tmp}

# Setup test directories for different bases
mkdir -p "$TEST_ROOT/etc/system.conf.d"
mkdir -p "$TEST_ROOT/var/system.conf.d"
mkdir -p "$TEST_ROOT/etc/custom_app.conf.d"
mkdir -p "$TEST_ROOT/var/custom_app.conf.d"
mkdir -p "$TEST_ROOT/tmp/system.conf.d"
mkdir -p "$TEST_ROOT/tmp/custom_app.conf.d"

# Create test config for default "system" base
cat > "$TEST_ROOT/etc/system.conf" << 'EOF'
[General]
hostname = default-host
version = 1.0

[Network]
port = 8080
EOF

# Create test config for "custom_app" base
cat > "$TEST_ROOT/etc/custom_app.conf" << 'EOF'
[General]
hostname = custom-host
version = 2.0

[Network]
port = 9090
EOF

echo "✅ Test environment created at: $TEST_ROOT"
echo ""

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SETTINGS_BIN="$SCRIPT_DIR/../build/settings"

# Check if settings binary exists
if [ ! -f "$SETTINGS_BIN" ]; then
    echo "ERROR: settings binary not found at: $SETTINGS_BIN"
    echo "Please build the project first: cd ../build && make settings"
    exit 1
fi

# Function to run settings command in test namespace
run_test() {
    local description="$1"
    shift
    local cmd="$@"

    echo "→ Testing: $description"
    echo "  Command: settings $cmd"

    unshare --user --map-root-user --mount --pid --fork bash -c "
        mount --make-rprivate / 2>/dev/null
        mount --bind '$TEST_ROOT/etc' /etc 2>/dev/null
        mount --bind '$TEST_ROOT/var' /var 2>/dev/null
        mount --bind '$TEST_ROOT/tmp' /tmp 2>/dev/null
        cd '$SCRIPT_DIR/../build'
        ./settings $cmd
    " 2>/dev/null

    echo ""
}

echo "═══════════════════════════════════════════════════════"
echo "TEST 1: Default base (system)"
echo "═══════════════════════════════════════════════════════"
run_test "Get hostname with default base" "hostname"
run_test "Get port with default base" "[Network]port"
echo ""

echo "═══════════════════════════════════════════════════════"
echo "TEST 2: Custom base using -s parameter"
echo "═══════════════════════════════════════════════════════"
run_test "Get hostname with custom base (-s)" "-s custom_app hostname"
run_test "Get port with custom base (-s)" "-s custom_app [Network]port"
echo ""

echo "═══════════════════════════════════════════════════════"
echo "TEST 3: Custom base using --source parameter"
echo "═══════════════════════════════════════════════════════"
run_test "Get hostname with custom base (--source)" "--source custom_app hostname"
run_test "Get version with custom base (--source)" "--source custom_app version"
echo ""

echo "═══════════════════════════════════════════════════════"
echo "TEST 4: List all settings with different bases"
echo "═══════════════════════════════════════════════════════"
echo "→ Default base (system):"
run_test "List default base" "" | head -5
echo "→ Custom base (custom_app):"
run_test "List custom base" "-s custom_app" | head -5
echo ""

# Cleanup
rm -rf "$TEST_ROOT"

echo "╔════════════════════════════════════════════════════════╗"
echo "║              ✅ ALL TESTS COMPLETED! ✅               ║"
echo "╚════════════════════════════════════════════════════════╝"

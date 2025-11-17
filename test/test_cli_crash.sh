#!/bin/bash

echo "=== CLI Crash Protection Test ==="
echo ""

SETTINGS="./build/settings"
FAILED=0

# Test 1: Missing closing bracket (the original crash)
echo "Test 1: Missing closing bracket '[category'..."
OUTPUT=$($SETTINGS "[category" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 1 ] && echo "$OUTPUT" | grep -q "ERROR.*missing.*]"; then
    echo "✅ HANDLED: Graceful error (exit code: $EXIT_CODE)"
else
    echo "❌ FAILED: Expected error exit code 1"
    FAILED=$((FAILED + 1))
fi
echo ""

# Test 2: Missing opening bracket 'category]'
echo "Test 2: Missing opening bracket 'category]'..."
OUTPUT=$($SETTINGS "category]" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ HANDLED: Treated as normal key (exit code: $EXIT_CODE)"
else
    echo "❌ FAILED: Unexpected exit code $EXIT_CODE"
    FAILED=$((FAILED + 1))
fi
echo ""

# Test 3: Empty brackets '[]'
echo "Test 3: Empty brackets '[]'..."
OUTPUT=$($SETTINGS "[]" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ HANDLED: Treated as valid (exit code: $EXIT_CODE)"
else
    echo "⚠️  WARNING: Exit code $EXIT_CODE (acceptable)"
fi
echo ""

# Test 4: Multiple brackets '[[category]]'
echo "Test 4: Multiple brackets '[[category]]'..."
OUTPUT=$($SETTINGS "[[category]]" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ HANDLED: Processed (exit code: $EXIT_CODE)"
else
    echo "⚠️  WARNING: Exit code $EXIT_CODE (acceptable)"
fi
echo ""

# Test 5: Bracket at end '[category]'
echo "Test 5: Valid format '[category]key'..."
OUTPUT=$($SETTINGS "[category]key" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ HANDLED: Valid format processed (exit code: $EXIT_CODE)"
else
    echo "⚠️  WARNING: Exit code $EXIT_CODE (may be expected if key doesn't exist)"
fi
echo ""

# Test 6: Just opening bracket '['
echo "Test 6: Just opening bracket '['..."
OUTPUT=$($SETTINGS "[" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 1 ] && echo "$OUTPUT" | grep -q "ERROR"; then
    echo "✅ HANDLED: Graceful error (exit code: $EXIT_CODE)"
else
    echo "❌ FAILED: Expected error exit code 1"
    FAILED=$((FAILED + 1))
fi
echo ""

# Test 7: Very long input without bracket
echo "Test 7: Very long input without closing bracket..."
LONG_INPUT="[$(printf 'a%.0s' {1..1000})"
OUTPUT=$($SETTINGS "$LONG_INPUT" 2>&1)
EXIT_CODE=$?
if [ $EXIT_CODE -eq 1 ] && echo "$OUTPUT" | grep -q "ERROR"; then
    echo "✅ HANDLED: Graceful error on long input (exit code: $EXIT_CODE)"
else
    echo "❌ FAILED: Expected error exit code 1"
    FAILED=$((FAILED + 1))
fi
echo ""

echo "=================================="
if [ $FAILED -eq 0 ]; then
    echo "✅ ALL TESTS PASSED!"
    echo "CLI tool handles malformed input gracefully!"
    exit 0
else
    echo "❌ $FAILED TEST(S) FAILED!"
    exit 1
fi

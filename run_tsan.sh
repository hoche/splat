#!/bin/bash
# Script to run ThreadSanitizer tests with suppressions
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export TSAN_OPTIONS="suppressions=${SCRIPT_DIR}/tsan_suppressions.txt"
exec "$@"

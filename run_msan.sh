#!/bin/bash
# Script to run MemorySanitizer tests with suppressions
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export MSAN_OPTIONS="suppressions=${SCRIPT_DIR}/msan_suppressions.txt:halt_on_error=0"
exec "$@"

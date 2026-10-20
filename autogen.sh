#!/bin/sh
# Compatibility entry point for the maintained PCB-ai bootstrap.
exec "$(dirname "$0")/bootstrap.sh" "$@"

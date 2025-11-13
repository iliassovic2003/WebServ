#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

INPUT_JSON=$(cat)

"$SCRIPT_DIR/ssMan" "$INPUT_JSON"
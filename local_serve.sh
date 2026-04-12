#!/usr/bin/env bash

set -euo pipefail
set -x

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

inc_args="--incremental"
[ "${1:-}" = "full" ] && inc_args=""

echo "Starting jekyll..."

bundle exec jekyll serve -w $inc_args --host 0.0.0.0

#!/bin/bash
set -euo pipefail
source /cygdrive/c/Users/hankw/Documents/Codex/ti-nspire-dev/env.sh
cd "$(dirname "$(realpath "$0")")"
make -j4 "${@:-all}"

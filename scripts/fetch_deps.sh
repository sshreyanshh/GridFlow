#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../backend/third_party"

# Pinned versions live in these URLs. Bumping one is a deliberate commit.
declare -A URLS=(
  [httplib.h]="https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.15.3/httplib.h"
  [json.hpp]="https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp"
  [doctest.h]="https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h"
)

for name in "${!URLS[@]}"; do
  curl -fsSL "${URLS[$name]}" -o "$name"
done

if [[ -f DEPENDENCIES.lock ]]; then
  sha256sum -c DEPENDENCIES.lock          # fails loudly on any mismatch
else
  sha256sum httplib.h json.hpp doctest.h > DEPENDENCIES.lock
  echo "Created DEPENDENCIES.lock. Commit it."
fi
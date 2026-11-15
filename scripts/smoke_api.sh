#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"
if [[ ! -x backend/build/gridflow_server ]]; then
    echo "Build backend/build/gridflow_server first" >&2
    exit 1
fi
port="$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1]); s.close()')"
backend/build/gridflow_server --data-dir "$repo_root/data" --port "$port" &
server_pid=$!
cleanup() {
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
}
trap cleanup EXIT
base="http://127.0.0.1:$port"
ready=0
for _ in $(seq 1 50); do
    if curl -fsS "$base/api/health" >/dev/null; then ready=1; break; fi
    sleep 0.1
done
if [[ "$ready" != 1 ]]; then echo "Server did not become ready" >&2; exit 1; fi

check_ok() {
    local method="$1"
    local endpoint="$2"
    if [[ "$method" == "GET" ]]; then
        curl -fsS "$base$endpoint" | grep -q '"ok":true'
    else
        curl -fsS -X "$method" "$base$endpoint" | grep -q '"ok":true'
    fi
}
check_key() {
    local method="$1"
    local endpoint="$2"
    local key="$3"
    local body
    if [[ "$method" == "GET" ]]; then body="$(curl -fsS "$base$endpoint")"
    else body="$(curl -fsS -X "$method" "$base$endpoint")"; fi
    grep -q '"ok":true' <<<"$body"
    grep -q "\"$key\"" <<<"$body"
}
check_ok GET /api/health
check_key GET /api/network nodes
check_key GET /api/state summary
check_key GET /api/scenarios scenarios
check_key GET /api/analysis/critical-nodes ranking
check_key GET '/api/routes?from=S1&to=D1&metric=cost' bottleneckCapacity
check_key GET /api/events events
check_key POST /api/scenarios/SC02/run recovery
check_key POST /api/reset state
error_body="$(curl -sS -X POST -H 'Content-Type: application/json' -d '{' "$base/api/disruptions")"
grep -q '"ok":false' <<<"$error_body"
grep -q '"code":"INVALID_JSON"' <<<"$error_body"
echo "GridFlow API smoke test passed"

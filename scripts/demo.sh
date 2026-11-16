#!/usr/bin/env bash
# One-command demo launcher for GridFlow.
#
#   ./scripts/demo.sh           build + unit tests + API smoke test + live API check,
#                               then start backend and frontend (live mode)
#   ./scripts/demo.sh --fast    skip the unit tests and the smoke test (already verified)
#   ./scripts/demo.sh --mock    frontend only, with mock data and no backend (fallback)
#   ./scripts/demo.sh --terminal  open the interactive C++ terminal demo
#
# Run it from anywhere; it locates the repository root itself.
# Ports: backend 8080 (must match the Vite proxy in frontend/vite.config.js), frontend 5173.
# Logs:  .demo-backend.log and .demo-frontend.log in the repository root.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BACKEND_DIR="$ROOT/backend"
BUILD_DIR="$BACKEND_DIR/build"
FRONTEND_DIR="$ROOT/frontend"
BACKEND_PORT=8080
FRONTEND_PORT=5173
MODE="full"
INTERFACE=""

usage() {
    sed -n '2,11p' "$0" | sed 's/^# \{0,1\}//'
}

for arg in "$@"; do
    case "$arg" in
        --fast) MODE="fast" ;;
        --mock) MODE="mock" ;;
        --terminal) INTERFACE="terminal" ;;
        --frontend) INTERFACE="frontend" ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $arg" >&2; usage; exit 1 ;;
    esac
done

# ------------------------------------------------------------------ helpers

say()  { printf '\n\033[1;36m==> %s\033[0m\n' "$*"; }
ok()   { printf '\033[1;32m    ok\033[0m  %s\n' "$*"; }
fail() { printf '\n\033[1;31mFAIL: %s\033[0m\n' "$*" >&2; exit 1; }
need() { command -v "$1" >/dev/null 2>&1 || fail "missing tool '$1' (install it and retry)"; }

PIDS=()
cleanup() {
    if ((${#PIDS[@]})); then
        for pid in "${PIDS[@]}"; do
            # a negative pid kills the whole process group (npm and the node child it spawns)
            kill -- "-$pid" 2>/dev/null || kill "$pid" 2>/dev/null || true
        done
    fi
}
trap cleanup EXIT INT TERM

wait_for_url() {  # wait_for_url <url> <seconds> <pid-to-watch> <log-file>
    local url="$1" seconds="$2" pid="$3" log="$4"
    local tries=$((seconds * 5))
    for _ in $(seq 1 "$tries"); do
        if curl -fs "$url" >/dev/null 2>&1; then return 0; fi
        if ! kill -0 "$pid" 2>/dev/null; then
            echo "---- last lines of $log ----" >&2
            tail -n 20 "$log" >&2 || true
            fail "process exited before $url became reachable"
        fi
        sleep 0.2
    done
    fail "$url not reachable after ${seconds}s (see $log)"
}

api() {  # api METHOD PATH [JSON_BODY]  -> prints the response body, fails on HTTP errors
    local method="$1" path="$2" body="${3:-}"
    if [[ -n "$body" ]]; then
        curl -fsS -X "$method" -H 'Content-Type: application/json' -d "$body" \
            "http://127.0.0.1:$BACKEND_PORT$path"
    else
        curl -fsS -X "$method" "http://127.0.0.1:$BACKEND_PORT$path"
    fi
}

http_code() {  # http_code METHOD PATH [JSON_BODY]  -> prints only the HTTP status code
    local method="$1" path="$2" body="${3:-}"
    if [[ -n "$body" ]]; then
        curl -s -o /dev/null -w '%{http_code}' -X "$method" -H 'Content-Type: application/json' \
            -d "$body" "http://127.0.0.1:$BACKEND_PORT$path"
    else
        curl -s -o /dev/null -w '%{http_code}' -X "$method" "http://127.0.0.1:$BACKEND_PORT$path"
    fi
}

expect_ok() {  # expect_ok METHOD PATH [JSON_BODY]: the envelope must say "ok": true
    local out
    out="$(api "$@")" || fail "request failed: $*"
    if ! grep -Eq '"ok"[[:space:]]*:[[:space:]]*true' <<<"$out"; then
        fail "response is not ok for: $* -> ${out:0:300}"
    fi
    ok "$1 $2"
}

expect_status() {  # expect_status CODE METHOD PATH [JSON_BODY]
    local want="$1"; shift
    local got
    got="$(http_code "$@")"
    [[ "$got" == "$want" ]] || fail "expected HTTP $want, got $got for: $*"
    ok "$1 $2 -> $want (expected error)"
}

# ------------------------------------------------------------------ steps

check_tools() {
    if [[ "$INTERFACE" == "frontend" ]]; then
        need npm
        need node
        need curl
    fi
    if [[ "$MODE" != "mock" ]]; then
        need cmake
        need g++
    fi
}

ensure_ports_free() {
    if [[ "$MODE" != "mock" ]] && curl -fs "http://127.0.0.1:$BACKEND_PORT/api/health" >/dev/null 2>&1; then
        fail "something already answers on port $BACKEND_PORT. Stop it first (ss -ltnp | grep $BACKEND_PORT)"
    fi
    if curl -fs "http://127.0.0.1:$FRONTEND_PORT" >/dev/null 2>&1; then
        fail "something already answers on port $FRONTEND_PORT. Stop the old dev server first"
    fi
}

build_backend() {
    say "Building backend"
    cmake -S "$BACKEND_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug >/dev/null
    cmake --build "$BUILD_DIR" -j
    [[ -x "$BUILD_DIR/gridflow_server" ]] || fail "gridflow_server was not built"
    [[ -x "$BUILD_DIR/gridflow_tests" ]]  || fail "gridflow_tests was not built"
    ok "build finished"
}

run_unit_tests() {
    say "Running unit tests"
    "$BUILD_DIR/gridflow_tests" | tail -n 4
}

run_smoke_test() {
    say "Running API smoke test (scripts/smoke_api.sh)"
    if [[ -x "$ROOT/scripts/smoke_api.sh" ]]; then
        "$ROOT/scripts/smoke_api.sh"
        ok "smoke test passed"
    else
        echo "    scripts/smoke_api.sh not found or not executable, skipping"
    fi
}

start_backend() {
    say "Starting backend on port $BACKEND_PORT"
    local log="$ROOT/.demo-backend.log"
    "$BUILD_DIR/gridflow_server" --data-dir "$ROOT/data" --port "$BACKEND_PORT" >"$log" 2>&1 &
    local pid=$!
    PIDS+=("$pid")
    wait_for_url "http://127.0.0.1:$BACKEND_PORT/api/health" 10 "$pid" "$log"
    ok "backend is healthy (pid $pid)"
}

live_check() {
    say "Checking every feature through the live API"
    expect_ok GET  /api/health
    expect_ok GET  /api/network
    expect_ok GET  /api/state
    expect_ok GET  /api/scenarios
    expect_ok GET  "/api/routes?from=S1&to=D1&metric=cost"
    expect_ok GET  "/api/routes?from=S1&to=D1&metric=time"
    expect_ok GET  /api/analysis/critical-nodes
    expect_ok GET  /api/events

    # the four disruption primitives
    expect_ok POST /api/disruptions '{"type":"NODE_OUTAGE","targetId":"H2","severity":1.0}'
    expect_ok POST /api/reset
    expect_ok POST /api/disruptions '{"type":"EDGE_DISRUPTION","targetId":"E12","severity":0.5}'
    expect_ok POST /api/reset
    expect_ok POST /api/disruptions '{"type":"SUPPLY_REDUCTION","targetId":"T2","severity":0.5}'
    expect_ok POST /api/reset
    expect_ok POST /api/disruptions '{"type":"DEMAND_SURGE","targetId":"D1","severity":0.4}'
    expect_ok POST /api/reset

    # every preset scenario, each from a clean baseline
    for id in SC01 SC02 SC03 SC04 SC05; do
        expect_ok POST "/api/scenarios/$id/run"
        expect_ok POST /api/reset
    done

    # error handling
    expect_status 404 GET  "/api/routes?from=NOPE&to=D1&metric=cost"
    expect_status 404 POST /api/disruptions '{"type":"NODE_OUTAGE","targetId":"ZZ9","severity":1.0}'
    expect_status 400 POST /api/disruptions 'this is not json'
    expect_status 404 POST /api/scenarios/SC99/run
    expect_status 404 GET  /api/does-not-exist
    ok "all live checks passed, simulation is back at baseline"
}

start_frontend() {  # start_frontend <true|false>   (true = mock data, false = real backend)
    local use_mocks="$1"
    say "Starting frontend (VITE_USE_MOCKS=$use_mocks)"
    local log="$ROOT/.demo-frontend.log"
    cd "$FRONTEND_DIR"
    if [[ ! -d node_modules ]]; then
        echo "    node_modules missing, running npm install"
        npm install
    fi
    VITE_USE_MOCKS="$use_mocks" setsid npm run dev:vite -- --host 127.0.0.1 --port "$FRONTEND_PORT" \
        --strictPort >"$log" 2>&1 &
    local pid=$!
    PIDS+=("$pid")
    cd "$ROOT"
    wait_for_url "http://127.0.0.1:$FRONTEND_PORT" 30 "$pid" "$log"
    ok "frontend is up (pid $pid)"
}

banner() {
    local label="$1"
    printf '\n\033[1;32m=============================================================\n'
    printf '  GridFlow demo is running  (%s)\n' "$label"
    printf '  Open:     http://127.0.0.1:%s\n' "$FRONTEND_PORT"
    if [[ "$MODE" != "mock" ]]; then
        printf '  Backend:  http://127.0.0.1:%s/api/health\n' "$BACKEND_PORT"
    fi
    printf '  Logs:     .demo-backend.log   .demo-frontend.log\n'
    printf '  Stop:     press Ctrl+C in this terminal\n'
    printf '=============================================================\033[0m\n\n'
}

# ------------------------------------------------------------------ main

cd "$ROOT"
if [[ -z "$INTERFACE" ]]; then
    printf '\n  Choose how to explore GridFlow:\n'
    printf '  1  Frontend dashboard\n'
    printf '  2  Terminal demo\n'
    printf '  Choice [1]: '
    interface_choice="1"
    read -r interface_choice || true
    if [[ "$interface_choice" == "2" ]]; then
        INTERFACE="terminal"
    else
        INTERFACE="frontend"
    fi
fi

check_tools

if [[ "$INTERFACE" == "terminal" ]]; then
    need cmake
    need g++
    say "Building terminal demo"
    cmake -S "$BACKEND_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug >/dev/null
    cmake --build "$BUILD_DIR" --target gridflow_terminal -j
    ok "terminal demo built"
    exec "$BUILD_DIR/gridflow_terminal" --data-dir "$ROOT/data"
fi

ensure_ports_free

if [[ "$MODE" == "mock" ]]; then
    start_frontend true
    banner "MOCK mode, no backend"
else
    build_backend
    if [[ "$MODE" == "full" ]]; then
        run_unit_tests
        run_smoke_test
    fi
    start_backend
    live_check
    start_frontend false
    banner "LIVE mode"
fi

# Keep the script alive so the trap can stop everything on Ctrl+C.
wait

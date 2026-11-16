#!/usr/bin/env bash
# Start the C++ API and Vite together for the common `npm run dev` workflow.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FRONTEND_DIR="$ROOT/frontend"
BUILD_DIR="$ROOT/backend/build"
BACKEND_PORT=8080
FRONTEND_PORT=5173
BACKEND_PID=""
FRONTEND_PID=""
STARTED_BACKEND=0

gridflow_api_ready() {
    curl -fsS "http://127.0.0.1:$BACKEND_PORT/api/health" 2>/dev/null \
        | grep -q '"service":"gridflow"'
}

cleanup() {
    if [[ -n "$FRONTEND_PID" ]]; then
        kill "$FRONTEND_PID" 2>/dev/null || true
        wait "$FRONTEND_PID" 2>/dev/null || true
    fi
    if [[ "$STARTED_BACKEND" == "1" && -n "$BACKEND_PID" ]]; then
        kill "$BACKEND_PID" 2>/dev/null || true
        wait "$BACKEND_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

if ! gridflow_api_ready; then
    command -v cmake >/dev/null || { echo "cmake is required to build the GridFlow API" >&2; exit 1; }
    command -v c++ >/dev/null || { echo "A C++17 compiler is required to build the GridFlow API" >&2; exit 1; }
    echo "Building GridFlow C++ API..."
    cmake -S "$ROOT/backend" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug
    cmake --build "$BUILD_DIR" --target gridflow_server -j
    echo "Starting GridFlow API at http://127.0.0.1:$BACKEND_PORT"
    "$BUILD_DIR/gridflow_server" --data-dir "$ROOT/data" --port "$BACKEND_PORT" \
        >"$ROOT/.gridflow-api.log" 2>&1 &
    BACKEND_PID=$!
    STARTED_BACKEND=1
    for _ in $(seq 1 50); do
        if gridflow_api_ready; then
            break
        fi
        if ! kill -0 "$BACKEND_PID" 2>/dev/null; then
            cat "$ROOT/.gridflow-api.log" >&2
            echo "GridFlow API stopped during startup" >&2
            exit 1
        fi
        sleep 0.2
    done
    if ! gridflow_api_ready; then
        cat "$ROOT/.gridflow-api.log" >&2
        echo "GridFlow API did not become ready on port $BACKEND_PORT" >&2
        exit 1
    fi
else
    echo "Using existing GridFlow API at http://127.0.0.1:$BACKEND_PORT"
fi

cd "$FRONTEND_DIR"
echo "Starting frontend at http://127.0.0.1:$FRONTEND_PORT (Ctrl+C stops the dev server)"
npm run dev:vite -- --host 127.0.0.1 --port "$FRONTEND_PORT" "$@" &
FRONTEND_PID=$!
wait "$FRONTEND_PID"

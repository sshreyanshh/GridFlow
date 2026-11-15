# GridFlow backend

## Requirements

- CMake 3.16 or newer
- A C++17 compiler
- POSIX threads
- `curl` and Python 3 for the smoke script

The HTTP, JSON, and doctest headers are vendored in `third_party/`.

## Build and run

From the repository root:

```bash
cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Debug
cmake --build backend/build -j
./backend/build/gridflow_server
```

The server defaults to `127.0.0.1:8080`. `--data-dir PATH` selects a directory containing `network.json` and `scenarios.json`; `--port N` changes the port. The default data directory is `../data` when launched from `backend/build`, otherwise `data` from the current directory.

## Tests and sanitizers

```bash
./backend/build/gridflow_tests
cmake -S backend -B backend/build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
cmake --build backend/build-asan -j
./backend/build-asan/gridflow_tests
```

Run the live endpoint smoke check after building:

```bash
./scripts/smoke_api.sh
```

## Data and layout

- `data/network.json`: static 22-node, 40-edge baseline
- `data/scenarios.json`: five disruption presets
- `include/gridflow/`: Track A public headers and model/core/algorithm/simulation/API interfaces
- `src/`: implementations and retained Track B modules
- `tests/`: doctest unit and integration tests
- `third_party/`: vendored httplib, nlohmann/json, and doctest

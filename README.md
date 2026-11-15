# GridFlow

**Modelling, Analysing and Optimizing Energy Distribution Networks**

GridFlow is a graph-based simulation system that models a regional energy supply network and studies how it behaves when parts of it fail. Facilities are vertices of a directed, capacity-constrained graph, and supply routes are edges. A user can disrupt the network, see which facilities are affected, find structurally critical infrastructure, explore alternative routes, and review how supply is redistributed.

The project is a Data Structures and Algorithms (DSA) lab project. The core data structures (hash table, min-heap, queue, linked list, graph) and graph algorithms (BFS, connected components, articulation points) are implemented from scratch in C++17, with a React and Cytoscape.js dashboard for visualization.

> **Status at a glance.** The C++ backend is a tested data-structure and algorithm library with a health-check HTTP endpoint. The dashboard is complete and runs against an in-browser JavaScript simulation engine (mock API) by default. The full HTTP integration between the two is defined by an API contract but is not implemented in the backend yet. See [Project Status](#3-project-status) for the exact breakdown.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Features](#2-features)
3. [Project Status](#3-project-status)
4. [System Architecture](#4-system-architecture)
5. [Data Structures and Algorithms](#5-data-structures-and-algorithms)
6. [The Network Model](#6-the-network-model)
7. [API Contract](#7-api-contract)
8. [Repository Structure](#8-repository-structure)
9. [Getting Started](#9-getting-started)
10. [Testing](#10-testing)
11. [Limitations](#11-limitations)
12. [Future Work](#12-future-work)
13. [Academic Context and Team](#13-academic-context-and-team)
14. [References](#14-references)
15. [License](#15-license)

---

## 1. Overview

Energy distribution networks are chains of dependent facilities: production, import, processing, distribution, power generation and finally demand. The failure of one component can ripple through everything downstream. GridFlow represents this dependency structure as a graph and uses graph algorithms to answer practical questions:

- Which facilities and routes are exposed when a given facility or route fails?
- Which facilities are structural single points of failure?
- Which demand centres lose supply, and how do priorities decide who is served first?
- What is the cheapest or fastest route between two facilities?

**Sustainable Development Goals addressed:** SDG 7 (Affordable and Clean Energy) and SDG 9 (Industry, Innovation and Infrastructure).

## 2. Features

**Network visualization**
- Interactive network drawn on a geographic map of India using Cytoscape.js, with node shapes by facility type and colours by operational status.
- Zoom to reveal labels and detailed telemetry; click a node or edge to inspect it.
- Light and dark themes.

**Disruption simulation** (four disruption types and five preset scenarios)

| Disruption | Effect |
|---|---|
| Node outage | A facility fails completely |
| Edge disruption | A route loses part (reduced) or all (failed) of its capacity |
| Supply reduction | A source or import terminal supplies less |
| Demand surge | A demand centre needs more energy |

Preset scenarios: Cyclone on Western Coast, Central Hub Fire, Import Restrictions, Heatwave Demand Surge, Cascading Failure.

**Analysis dashboard** (tabs): Summary, Shortages, Recovery, Critical Nodes, Route Finder, Event Log.

- **Summary:** supply, demand, served energy and unmet demand.
- **Shortages:** affected demand centres ordered by priority, then size of shortage.
- **Recovery:** reallocated supply paths with flow, cost and time, unresolved shortages, and recommendations.
- **Critical Nodes:** articulation points and an impact-based ranking.
- **Route Finder:** cheapest or fastest route between two facilities.
- **Event Log:** an append-only history of the simulation.

## 3. Project Status

| Component | State |
|---|---|
| Custom data structures in C++ (HashTable, MinHeap, Queue, LinkedList, Graph) | Implemented and unit-tested |
| BFS downstream impact analysis, connected components, articulation points (C++) | Implemented and unit-tested |
| Disruption engine, impact analyzer, event log, JSON serializer (C++) | Implemented and unit-tested; serializer output is checked against the contract examples |
| C++ HTTP server | Health-check endpoint only (`GET /api/health`) |
| Route search (Dijkstra) and priority-based supply allocation | Implemented in the **frontend** mock simulation engine (JavaScript) |
| React dashboard | Complete; runs on the mock API by default |
| Backend to frontend integration through the HTTP API | Defined in the contract; the remaining endpoints are not yet served by the C++ backend |

The dashboard therefore demonstrates the full user experience on a mock engine, while the C++ library demonstrates the data structures and graph algorithms that the integrated system is built on.

## 4. System Architecture

```
          ┌──────────────────────────────────────────────┐
          │  Frontend  (React 18, Vite, Cytoscape.js)    │
          │  Dashboard · Network map · Analysis tabs     │
          └───────────────┬──────────────────────────────┘
                          │  api/client.js chooses one implementation:
            ┌─────────────┴─────────────┐
            │ mockApi.js  (default)     │  realApi.js  (VITE_USE_MOCKS=false)
            │ JS simulation engine      │  fetch('/api/...') via Vite proxy
            └───────────────────────────┘              │
                                                       ▼   JSON over HTTP (contract v0.1)
          ┌──────────────────────────────────────────────┐
          │  Backend  (C++17)                            │
          │  api/   Serializer · health endpoint         │
          │  sim/   DisruptionEngine · DisruptionQueue   │
          │         ImpactAnalyzer · EventLog            │
          │  algo/  Traversal (BFS, components)          │
          │         ArticulationPoints                   │
          │  core/  Graph      model/  Facility, Route   │
          │  ds/    HashTable · MinHeap · Queue · List   │
          └──────────────────────────────────────────────┘
```

Layers only call downwards: the API layer uses the simulation layer, which uses the algorithms, which use the graph and data structures. The graph addresses everything by integer index rather than by pointer, which keeps copies of the graph safe and independent.

| Concern | Technology |
|---|---|
| Backend language and build | C++17, CMake 3.16 or newer |
| HTTP server | cpp-httplib 0.15.3 (vendored) |
| JSON | nlohmann/json 3.11.3 (vendored, used for input and output only) |
| Backend tests | doctest 2.4.11 (vendored) |
| Frontend | React 18.3.1, Vite 6.0.5, Cytoscape.js 3.33.1 |
| Frontend tests | Vitest |

Third-party headers are committed in `backend/third_party/` and pinned by checksum in `DEPENDENCIES.lock`, so the backend builds offline.

## 5. Data Structures and Algorithms

All structures below are implemented in this repository. The standard library is used only for `std::string`, `std::vector` (as raw storage), memory allocation primitives and I/O. The standard associative and priority containers are not used in the backend.

### Data structures

| Structure | Location | Role in the system | Complexity |
|---|---|---|---|
| Graph (directed adjacency lists, outgoing and incoming) | `backend/include/gridflow/core/Graph.h` | Stores facilities and routes; validates unique ids, valid endpoints and absence of self-loops | O(V + E) space |
| Hash table (separate chaining, FNV-1a 64-bit, automatic resizing at load factor 0.75) | `backend/include/gridflow/ds/HashTable.h` | Maps facility and route ids to indices | average O(1) lookup and insert |
| Min-heap (binary heap, comparator-based) | `backend/include/gridflow/ds/MinHeap.h` | Priority-based retrieval; implemented and tested as a reusable component | O(log n) push and pop |
| Queue (dynamic circular buffer) | `backend/src/ds/Queue.h` | BFS frontier; FIFO processing of disruption events | O(1) amortized |
| Singly linked list (head and tail pointers) | `backend/src/ds/LinkedList.h` | Append-only event history | O(1) append |

### Algorithms

| Algorithm | Location | Application | Complexity |
|---|---|---|---|
| Breadth-first search | `backend/src/algo/Traversal.cpp` | Identifies facilities and routes downstream of a disruption, skipping failed components | O(V + E) |
| Connected components (iterative DFS over the undirected operational graph) | `backend/src/algo/Traversal.cpp` | Detects demand centres cut off from every supply facility | O(V + E) |
| Articulation points (DFS discovery times and low-link values, Tarjan) | `backend/src/algo/ArticulationPoints.cpp` | Finds structurally critical facilities; run on the undirected view of the operational network | O(V + E) |
| Dijkstra shortest path (linear-scan minimum selection) | `frontend/src/mocks/simulate.js` | Cheapest or fastest route; also used inside allocation | O(V²) selection plus edge scans |
| Greedy priority-based allocation | `frontend/src/mocks/simulate.js` | Serves demand centres in priority order along the cheapest feasible paths, respecting edge, node and supply capacities | repeated shortest-path searches |

Two points worth noting:

- **Articulation points are defined for undirected graphs**, while energy flows in one direction. GridFlow therefore reports structural weak points (articulation points) and a separate flow-based ranking that fails each facility in turn and measures the resulting unmet demand.
- **The allocation is a greedy heuristic.** It serves critical demand first and always takes the cheapest remaining path. It never cancels earlier flow, so the result is not guaranteed to be globally optimal.

## 6. The Network Model

The bundled dataset (`contracts/examples/network.json`) is an illustrative network of **22 facilities and 40 directed routes**, with a total supply of 2600 and a total demand of 2000 energy units per day. It uses India-inspired facility names but is **not real operational data**.

| Layer | Facility type | Count |
|---|---|---|
| 1 | Source | 3 |
| 1 | Import terminal | 3 |
| 2 | Processing | 4 |
| 3 | Distribution hub | 4 |
| 4 | Power plant | 3 |
| 5 | Demand centre | 5 |

Every facility has a throughput **capacity**. Sources and import terminals have a **supply**. Demand centres have a **demand** and a **priority** (1 is critical, 3 is lowest). Every route has a **capacity**, a **cost** per unit and a **transit time**. A single abstract commodity (energy units per day) is modelled.

**Operational states**

| Element | States |
|---|---|
| Facility | Active, Degraded, Failed |
| Route | Active, Reduced, Failed |

Disruptions are validated (existence of the target, suitability of the target type, severity between 0 and 1) and a later disruption on the same target replaces the earlier one instead of compounding.

## 7. API Contract

The full specification is in [`contracts/api-contract.md`](contracts/api-contract.md). Every response uses a common envelope:

```json
{ "ok": true,  "data": { } }
{ "ok": false, "error": { "code": "NODE_NOT_FOUND", "message": "...", "details": { } } }
```

| Endpoint | Purpose | C++ server | Frontend mock |
|---|---|---|---|
| `GET /api/health` | Service check | Yes | Yes |
| `GET /api/network` | Static network structure | Contract and serializer only | Yes |
| `GET /api/state` | Current statuses, flows, summary | Contract and serializer only | Yes |
| `POST /api/disruptions` | Apply a disruption, return impact, recovery and state | Contract and serializer only | Yes |
| `GET /api/scenarios`, `POST /api/scenarios/{id}/run` | List and run presets | Contract only | Yes |
| `GET /api/analysis/critical-nodes` | Articulation points and criticality ranking | Contract only | Yes |
| `GET /api/routes?from=&to=&metric=` | Cheapest or fastest route | Contract and serializer only | Yes |
| `GET /api/events` | Event log | Contract and serializer only | Yes |
| `POST /api/reset` | Restore the baseline | Contract only | Yes |

The C++ `Serializer` produces the contract's JSON shapes, and its unit tests compare the output with the reference payloads in [`contracts/examples/`](contracts/examples/).

## 8. Repository Structure

```
GridFlow/
├── backend/
│   ├── CMakeLists.txt
│   ├── BACKEND_SPEC.md                 backend technical specification
│   ├── include/gridflow/
│   │   ├── core/Graph.h                directed graph, adjacency lists
│   │   ├── ds/HashTable.h, MinHeap.h   custom data structures
│   │   └── model/                      Types, Facility, Route, Disruption
│   ├── src/
│   │   ├── core/Graph.cpp
│   │   ├── model/Types.cpp
│   │   ├── ds/Queue.h, LinkedList.h    custom data structures
│   │   ├── algo/                       Traversal (BFS, components), ArticulationPoints
│   │   ├── sim/                        DisruptionEngine, DisruptionQueue,
│   │   │                               ImpactAnalyzer, EventLog
│   │   ├── api/Serializer.*            struct to JSON conversion
│   │   └── main.cpp                    HTTP server (health endpoint)
│   ├── tests/                          doctest unit tests, one file per module
│   └── third_party/                    cpp-httplib, nlohmann/json, doctest, DEPENDENCIES.lock
├── frontend/
│   ├── package.json, vite.config.js
│   └── src/
│       ├── api/                        client.js, realApi.js, mockApi.js, errors.js
│       ├── mocks/                      simulate.js (mock simulation engine) and its tests
│       ├── hooks/                      SimulationContext.jsx (shared state and actions)
│       ├── components/NetworkGraph.jsx Cytoscape map visualization
│       ├── pages/Dashboard.jsx
│       └── styles/
├── contracts/
│   ├── api-contract.md                 API specification (v0.1)
│   └── examples/                       reference JSON payloads and the sample network
├── data/
├── docs/
└── scripts/fetch_deps.sh               re-downloads and verifies the pinned dependencies
```

## 9. Getting Started

### Prerequisites

| Tool | Version |
|---|---|
| C++ compiler | GCC or Clang with C++17 support |
| CMake | 3.16 or newer |
| Node.js and npm | Node 18 or newer (required by Vite 6) |
| Git | any recent version |

The project was developed on Linux. macOS and Windows (for example through WSL2) are expected to work with the same tools but have not been verified.

### Clone

```bash
git clone https://github.com/sshreyanshh/GridFlow.git
cd GridFlow
```

### Backend

```bash
cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Debug
cmake --build backend/build -j

./backend/build/gridflow_tests        # run all unit tests
./backend/build/gridflow_server       # health endpoint on http://127.0.0.1:8080/api/health
```

Check that the server is up:

```bash
curl http://127.0.0.1:8080/api/health
```

To build with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake -S backend -B backend/build-asan -DCMAKE_BUILD_TYPE=Debug -DGRIDFLOW_ENABLE_SANITIZERS=ON
cmake --build backend/build-asan -j
./backend/build-asan/gridflow_tests
```

To re-download the pinned third-party headers and verify their SHA-256 checksums: `./scripts/fetch_deps.sh`.

### Frontend

```bash
cd frontend
npm install
npm run dev                            # http://localhost:5173
```

The dashboard starts in **mock mode** and needs no backend. The top bar shows which data source is active. Other scripts:

| Script | Purpose |
|---|---|
| `npm run build` | Production build |
| `npm run lint` | ESLint |
| `npm test` | Unit tests for the mock simulation engine (Vitest) |

**Mock and live mode.** The data source is chosen at start-up by the environment variable `VITE_USE_MOCKS`. Any value other than `false` selects the mock engine. Setting `VITE_USE_MOCKS=false` makes the dashboard call the real API through the Vite proxy (`/api` is forwarded to `http://127.0.0.1:8080`). This mode needs a backend that serves the full contract. In mock mode, appending `?fail=1` to the page URL makes every request fail, which is useful for checking the error handling.

### Troubleshooting

| Problem | Fix |
|---|---|
| `npm run dev` does nothing or `vite: not found` | Run `npm install` inside `frontend/` first |
| Frontend files missing after cloning | Check out the branch that contains the frontend (`git branch -a`) |
| Port 8080 or 5173 already in use | Stop the other process (`ss -ltnp \| grep 8080`) or change the port |
| New source files are not compiled | Re-run the `cmake -S backend -B backend/build` command; CMake re-scans `src/` and `tests/` |

## 10. Testing

| Suite | Command | Coverage |
|---|---|---|
| Backend (doctest) | `./backend/build/gridflow_tests` | 93 test cases across the data structures, graph, traversal, articulation points, disruption engine, impact analyzer, event log and serializer |
| Frontend (Vitest) | `cd frontend && npm test` | 4 tests for the mock simulation engine |

The backend tests cover empty, single-element, typical, boundary and failure cases, including rehashing, hash collisions, queue growth, memory ownership of copied containers, and validation failures. Tests that depend on reference data read the files in `contracts/examples/`.

## 11. Limitations

- The C++ backend does not yet serve the complete API. Route search and supply allocation currently exist only in the frontend mock engine.
- The model is a **steady-state snapshot** after each disruption, not a time-stepped simulation.
- A **single abstract commodity** is modelled. Oil, gas and electricity are not distinguished.
- The allocation is a **greedy heuristic** and is not guaranteed to be optimal.
- The dataset is **illustrative** and not based on real operational data.
- There is no persistence, authentication or multi-user support. Application state is held in memory.

## 12. Future Work

- Implement Dijkstra's algorithm and the priority-based allocation in C++ using the existing MinHeap, and serve the full API contract so the dashboard runs against the real backend.
- Replace the greedy allocation with a minimum-cost maximum-flow algorithm (for example successive shortest path) for globally optimal redistribution.
- Add further criticality metrics.
- Support real, publicly available energy network datasets.
- Support comparison of multiple disruption scenarios.
- Improve performance and recovery planning for larger networks.

## 13. Academic Context and Team

**Institution:** Jaypee Institute of Information Technology, Noida
**Department:** Computer Science and Engineering and Information Technology
**Programme:** Bachelor of Technology, Third Semester (2026-2027)
**Course:** Data Structures Lab (15B17CI371)
**Submitted to:** Dr. Ankita Wadhwa, Dr. Potukuchi Raghu Vamsi

| Name | Enrollment number | Batch |
|---|---|---|
| Shreyansh Srivastava | 2501030069 | B11 |
| Abhigya Narayan Awasthi | 2501030085 | B11 |

**Contributions**

| Area | Owner |
|---|---|
| Graph, HashTable, MinHeap, model types, project build setup | Shreyansh Srivastava |
| Queue, LinkedList, BFS, connected components, articulation points, disruption engine, event log, impact analyzer, serializer | Abhigya Narayan Awasthi |
| Frontend dashboard and API contract | Joint |

## 14. References

1. T. H. Cormen, C. E. Leiserson, R. L. Rivest and C. Stein, *Introduction to Algorithms*, 4th Edition, MIT Press, 2022.
2. R. Sedgewick and K. Wayne, *Algorithms*, 4th Edition, Addison-Wesley, 2011.
3. React documentation: https://react.dev/
4. Vite documentation: https://vite.dev/guide/
5. Cytoscape.js documentation: https://js.cytoscape.org/
6. C++ reference: https://en.cppreference.com/
7. CMake documentation: https://cmake.org/documentation/

## 15. License

This repository was created for academic coursework. No open-source licence has been specified.
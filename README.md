# GridFlow

GridFlow is a prototype energy-supply network resilience dashboard. It visualizes a directed network, lets users apply infrastructure disruptions or run scenarios, and presents service impacts, shortages, recovery paths, routes, events, and critical-node analysis.

This README is an AI handoff: use it as orientation, then verify behavior in the source files named below. The repository is evolving; the draft contract and backend specification describe intended behavior and are not proof that the C++ backend implements it.

## Project status and grounding

- The frontend is a React 18 + Vite single-page app. It defaults to a local in-memory mock API.
- The mock simulator implements the interactive prototype behavior; it is a simple greedy path allocator, not a globally optimal network-flow solver.
- The C++17 backend is at the initial scaffold stage. `backend/src/main.cpp` currently serves only `GET /api/health`; the other endpoints are specified but not implemented there.
- `contracts/api-contract.md` is **v0.1 draft**. It is the intended frontend/backend interface, not a description of fully working server endpoints.
- `backend/BACKEND_SPEC.md` is the backend design and milestone plan. It marks B0 as done and later milestones as incomplete.
- The baseline network and scenarios currently used by the mock are in `contracts/examples/`. `data/` currently contains only `.gitkeep`; do not assume `data/network.json` exists.
- Names and approximate positions on the India map are illustrative. The 40 simulated links are not verified real-world routes or infrastructure.
- If code, this README, and a specification disagree, report the discrepancy and follow the relevant source of truth for the requested change. Do not silently invent missing behavior.

## Repository map

```text
GridFlow/
├── README.md                         # Project and AI handoff guide
├── backend/
│   ├── BACKEND_SPEC.md               # C++ design, DSA constraints, roadmap
│   ├── CMakeLists.txt                # C++17 server and test targets
│   ├── src/main.cpp                  # Current HTTP server scaffold (/api/health only)
│   ├── tests/                        # Current smoke tests
│   └── third_party/                  # Vendored httplib, nlohmann/json, doctest
├── contracts/
│   ├── api-contract.md               # Draft v0.1 API/data contract
│   └── examples/
│       ├── network.json              # Mock baseline graph (22 nodes, 40 directed edges)
│       ├── scenarios.json            # Five sample disruption scenarios
│       ├── state.baseline.json       # Baseline state example
│       ├── disruption-response.example.json
│       └── error.example.json
├── data/                             # Reserved for runtime/backend data; currently empty
├── docs/                             # Reserved for documentation; currently empty
├── scripts/fetch_deps.sh             # Dependency helper
└── frontend/
    ├── package.json                  # npm scripts and dependencies
    ├── vite.config.js                # Vite config and /api dev proxy
    ├── .env.example                  # Frontend environment options
    ├── CONTRACT_QUESTIONS.md         # Open API/mock assumptions
    └── src/
        ├── api/                      # API selector, real/mock clients, errors
        ├── components/NetworkGraph.jsx
        ├── hooks/SimulationContext.jsx
        ├── mocks/                    # In-memory simulator and unit tests
        ├── pages/Dashboard.jsx        # Dashboard and tabs
        └── styles/                   # Theme variables and dashboard CSS
```

## Frontend behavior

The dashboard uses shared simulation state from `SimulationContext.jsx`, which loads network, state, scenarios, and events through the API client. `api/client.js` selects the mock unless `VITE_USE_MOCKS` is exactly `false`.

The dashboard currently exposes:

- A graph where nodes and links can be selected; disruptions and status are visualized on the network.
- Disruption controls and five sample scenarios (`SC01`–`SC05`).
- Summary and shortages, recovery allocations/recommendations, critical-node analysis, route finder, and event history tabs.
- Reset to baseline.

`frontend/src/mocks/simulate.js` contains the mock’s allocator and analysis helpers. Disruptions on a target replace earlier disruptions on that target. The mock's state is in memory and resets on page reload; mock calls include a short delay, and `?fail=1` simulates an API failure.

## Visual theme

The interface is a dense telemetry console with coordinated light and dark palettes. Use the header theme button to switch; the choice is saved in browser storage, and dark mode is the default when no preference has been saved.

- Theme tokens and the Tailwind v4 `@theme inline` mapping are in `frontend/src/styles/variables.css`.
- UI typography uses JetBrains Mono with a monospace fallback. Pink and purple are the primary brand colors; chart tokens distinguish network states.
- The center panel is a directed Cytoscape network, overlaid on a subdued Survey of India state map. The map is contextual only; links are explicitly illustrative.
- The dashboard uses a utility header and a full-canvas network topology workspace. Collapsible simulation-control and telemetry drawers sit beside the map in a horizontal flex layout, pushing and resizing it as they open; the graph is re-fitted after drawer transitions. On narrow screens the drawers open one at a time; telemetry views use icon-and-label navigation and structured tables.
- Network nodes use semantic zoom: the overview stays compact, while zooming in reveals labels; hovering or selecting a node focuses its connected neighborhood. The map HUD provides zoom, fit-to-network, live zoom percentage, and a dismissible onboarding hint.
- Keep button hover, active, focus, and disabled states explicit; avoid neon or bloom effects. Respect `prefers-reduced-motion` when changing the UI.

## Data and API

The reference graph represents one abstract energy commodity. Units in the draft contract are EU/day, cost-units per EU, and hours. Edges are directed. The sample graph has 22 nodes and 40 links, and the draft data totals 2,600 supply and 2,000 demand.

The intended API is documented in `contracts/api-contract.md`. It defines `/api/health`, `/api/network`, `/api/state`, `/api/disruptions`, `/api/scenarios`, `/api/scenarios/{id}/run`, `/api/analysis/critical-nodes`, `/api/routes`, `/api/events`, and `/api/reset`. The dev server proxies `/api` to `http://127.0.0.1:8080`. The real frontend client expects JSON responses, typically wrapped in `{ "ok": true, "data": ... }`.

**Important:** these are contract endpoints. Only `/api/health` is currently registered by `backend/src/main.cpp`. Selecting the real API mode before implementing the remaining server endpoints will not provide a functioning full dashboard.

The API contract is the shared boundary for frontend/backend work. It calls for camelCase JSON fields, string IDs, a single shared in-memory simulation, and a version increment on successful mutations. It is explicitly draft; see `frontend/CONTRACT_QUESTIONS.md` for assumptions and unresolved details. Contract changes should be coordinated and the contract version/changelog updated.

## Backend design constraints

The intended backend is C++17. Its architecture and DSA requirements are detailed in `backend/BACKEND_SPEC.md`; read that file before implementing backend logic. In particular:

- The evaluated data structures and algorithms should be implemented by the project, not replaced with the forbidden STL containers listed in the spec (`std::map`, `std::unordered_map`, `std::queue`, `std::priority_queue`, and others).
- Prefer IDs and indices rather than raw pointers between graph objects.
- The allocation strategy is specified as a greedy successive-cheapest-path heuristic, not min-cost max-flow.
- Build flags in the CMake setup include `-Wall -Wextra -Wpedantic`; backend spec milestones also call for tests and memory-safety validation.

Do not implement planned classes or endpoints as though they already exist. Check the current files and milestone checklist before extending the backend.

## Setup and commands

Frontend, from the repository root:

```sh
cd frontend
npm install
npm run dev
```

Frontend scripts (run from `frontend/`):

```sh
npm run build
npm run lint
npm test
```

The frontend runs in mock mode by default (`frontend/.env.development` and `.env.example` set `VITE_USE_MOCKS=true`). Set `VITE_USE_MOCKS=false` to use the HTTP client; `VITE_API_BASE` defaults to `/api`. In development, Vite forwards `/api` to `127.0.0.1:8080`.

The backend uses CMake and a C++17 compiler. Configure/build/test from the repository root with:

```sh
cmake -S backend -B backend/build
cmake --build backend/build
ctest --test-dir backend/build --output-on-failure
```

The server listens on `127.0.0.1:8080`. Its currently implemented route is only `/api/health`.

## Guidance for AI assistants

When helping with this repository:

1. Inspect the relevant source and tests before proposing or editing behavior. Do not infer implementation from a filename, contract example, or roadmap.
2. Clearly distinguish **implemented**, **mock-only**, **specified/planned**, and **unknown** behavior. Never claim that a draft API endpoint works on the backend unless its route is present and tested.
3. Keep the frontend in React/JSX and follow its existing API, context, component, and CSS patterns. Preserve mock mode and API contract compatibility unless the task explicitly changes them.
4. For API work, consult `contracts/api-contract.md` and `frontend/CONTRACT_QUESTIONS.md`; flag ambiguous or conflicting requirements instead of silently choosing a behavior.
5. For backend algorithms or data structures, follow `backend/BACKEND_SPEC.md`, including its custom-DSA constraints and milestone order.
6. Preserve the dark control-room design and the distinction between normal, degraded, failed, exposed, and highlighted visual states.
7. Treat all sample facilities, positions, and links as illustrative simulation data, not verified infrastructure facts.
8. Make focused changes, add or update targeted tests, and report exactly what was changed and which checks were run.

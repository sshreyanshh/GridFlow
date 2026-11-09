# GridFlow Backend Specification (v0.1)

Owner: Shreyansh. This file is the single source of truth for the backend. It is meant to be pasted into a new chat so the mentor has full context.

---

## 0. How to use this file (instructions for the AI mentor)

You are my senior C++ architect and mentor. I am implementing this backend myself. **Do not dump the whole backend.** Work one task at a time, in the order of section 8:

1. For each task: explain WHAT, WHY, HOW, the DSA involved, the files touched, then give the implementation in small chunks (or the skeleton and let me fill in the rest), then give test cases and expected outputs.
2. Wait for me to share my code, then review it for bugs, complexity, memory safety, naming, and spec compliance, before moving on.
3. Keep every decision consistent with this spec and with `contracts/api-contract.md`. If you think the spec is wrong, say so and propose a change instead of silently deviating.
4. I must be able to defend every line in a viva. Explain trade-offs and complexities.
5. At the start of each session I will tell you which task I'm on and paste the relevant code.

---

## 1. Goal and scope

A C++17 HTTP server that holds a directed energy-supply graph (22 nodes, 40 edges, see the contract) and exposes it through the REST API in `contracts/api-contract.md`. It applies disruptions, reallocates energy greedily by priority, and reports impact, recovery plans, critical infrastructure, and routes.

Single commodity (EU/day). One shared in-memory simulation. No database. The baseline network is loaded from `data/network.json` at startup and never mutated. A working copy holds the disrupted state.

## 2. Constraints

**Allowed:** `std::string`, `std::vector` (as raw storage inside my own structures), `<iostream>`, `<fstream>`, `<chrono>`, `<mutex>`, `<memory>`, `<functional>`, `<algorithm>` for `std::sort` and `std::reverse` only, `<limits>`, `<cstdint>`.
**Third-party (vendored, pinned, in `backend/third_party/`):** cpp-httplib (HTTP), nlohmann/json (parse/serialize only), doctest (tests).
**Forbidden (the evaluated DSA must be mine):** `std::unordered_map`, `std::map`, `std::set`, `std::queue`, `std::priority_queue`, `std::list`, `std::stack`, `std::deque`.
Prefer **IDs and indices over raw pointers** between objects. Use `std::unique_ptr` only where ownership is needed. Build with `-Wall -Wextra -Wpedantic`. Run tests under valgrind and/or `-fsanitize=address,undefined` before each milestone.

## 3. Architecture (each layer calls only downward)

```
api/        Server, Routes, Serializer (JSON <-> structs; no logic)
sim/        DisruptionQueue, DisruptionEngine, ImpactAnalyzer, ShortageManager,
            RecoveryPlanner, EventLog, Simulation (facade owning baseline + working state)
algo/       Traversal (BFS, DFS, components), Dijkstra, ArticulationPoints
core/       Graph, NetworkLoader
model/      Types, Facility, Route, Disruption
ds/         HashTable, MinHeap, Queue, LinkedList
```

Files live under `backend/include/gridflow/<layer>/` (headers; templates are header-only) and `backend/src/`. Tests are in `backend/tests/`, one `test_<module>.cpp` per module.

## 4. Data structure specs (`ds/`)

| Structure | Spec |
|---|---|
| `HashTable<K,V>` | Separate chaining with my own node chain. String hash = FNV-1a 64-bit. Initial 16 buckets, doubles and rehashes when load factor > 0.75. API: `put` (insert or overwrite), `get` (pointer or optional-like, null if missing), `contains`, `remove`, `size`, iteration over all entries. Average O(1). Used for `id → node index`, `id → edge index`, scenario lookup. |
| `MinHeap<T, Compare>` | Binary heap on a `std::vector<T>`. `push`, `pop`, `top`, `empty`, `size`. O(log n). **No decrease-key; use lazy deletion** (push duplicates and skip stale entries on pop). Used by Dijkstra (key = distance) and ShortageManager (comparator = priority, then shortage). |
| `Queue<T>` | FIFO, circular buffer or linked nodes (my choice, justify it). `enqueue`, `dequeue`, `front`, `empty`, `size`, all O(1). Used by BFS frontier and by DisruptionQueue. |
| `LinkedList<T>` | Singly linked **with a tail pointer** so `append` is O(1). `append`, `size`, forward iteration, `toVector`. Owns its nodes, with a proper destructor (no leaks) and deleted or correct copy semantics (rule of 3/5). Used by EventLog (append-only history). |

Each gets unit tests including edge cases (empty, one element, many elements, resize/rehash, duplicate keys, remove from a chain).

## 5. Model and graph

**Facility** (node): `id`, `name`, `type` (enum), `capacity`, `supply`, `demand`, `priority` (0 if not demand), `position {x,y}`, plus mutable state: `status`, `effectiveSupply`, `effectiveDemand`, `served`, `shortage`, `throughput`.
**Route** (edge): `id`, `from`, `to` (node indices), `capacity`, `cost`, `transitTime`, plus mutable: `status`, `effectiveCapacity`, `flow`.

**Graph:** vectors of facilities and routes (indexed by integer), two adjacency lists (`outEdges[node]` and `inEdges[node]` as lists of edge indices), and two HashTables for id → index. Operations: `addFacility`, `addRoute` (validates both endpoints exist), `findFacility(id)`, `findRoute(id)`, `outEdges(i)`, `inEdges(i)`, `nodeCount`, `edgeCount`. The graph can be deep-copied to create the working copy from the baseline (`Simulation::reset` re-copies).

**NetworkLoader** validates: duplicate ids, dangling edge endpoints, negative or zero capacities, missing required fields, bad enums. Throws a descriptive exception, and the server refuses to start with a clear message.

## 6. Algorithms

### 6.1 Traversal (`algo/Traversal`)
- **BFS** from a start node over **active** out-edges, using my `Queue`. Returns visited nodes and traversed edges. Use: **downstream exposure** after a disruption (`impact.exposedNodes/Edges`). O(V+E).
- **DFS** (recursive or explicit stack) for **connected components** on the undirected projection of active edges. Use: `impact.components` (count and which demand centres have no active connection to any supply node). O(V+E).

### 6.2 Dijkstra (`algo/Dijkstra`)
- Parameterised by a weight selector (`cost` or `transitTime`) and by direction (forward over `outEdges`, reverse over `inEdges`).
- Skips edges that are `FAILED` or have residual capacity ≤ 0, and nodes that are `FAILED` or have no residual throughput.
- Uses my `MinHeap` with lazy deletion. Deterministic tie-breaking by node index. Returns distance array and parent-edge array, with a helper to reconstruct the path.
- Public use 1: `GET /api/routes` (forward from the source node, metric as requested).
- Public use 2: allocation, via a **multi-target reverse search** from a demand centre until the nearest node with remaining supply is settled.
- O((V+E) log V).

### 6.3 Articulation points (`algo/ArticulationPoints`)
- DFS with discovery time and low-link on the **undirected projection of currently active edges**. Handle the root rule (a root is an articulation point iff it has ≥ 2 DFS children). Handle disconnected graphs (run from every unvisited node). O(V+E).
- Document in the code and report: articulation points are structural, undirected, so complement with the flow-based ranking (6.5).

### 6.4 Allocation, greedy successive cheapest path (`sim/ShortageManager` + `sim/RecoveryPlanner`)
This is **not** min-cost max-flow. It is a documented greedy heuristic.

1. Compute effective values: effective edge capacity = `capacity × (1 − severity)` (0 if failed or either endpoint failed); effective supply for sources = `supply × (1 − severity)` (0 if failed); effective demand = `demand × (1 + surge)`; node residual throughput = `capacity` (0 if failed). Zero out all flows.
2. Push every demand centre with unmet demand > 0 into a **MinHeap** ordered by (priority ascending, shortage descending, index ascending).
3. Loop: pop the top demand centre D. Run reverse Dijkstra (by `cost`) from D over in-edges with residual > 0 to find the cheapest node S with remaining supply > 0.
   - No path → record `unresolved` (`NO_PATH`) for D and continue to the next (don't re-push).
   - Path found → `delta = min(D.unmet, S.remainingSupply, min residual edge capacity on path, min residual node throughput on path)`. Push `delta` along the path (increase edge flow, decrease residuals, increase node throughput, decrease S supply, increase D served). Append an allocation record (path from S to D, edge ids, flow, total cost, total time).
   - If D still has unmet demand and `delta > 0`, push D back onto the heap. Because it is re-popped immediately if it is still the highest priority, higher priorities are fully served first. If `delta == 0` (bottleneck exhausted), record `CAPACITY_EXHAUSTED`.
4. Anything left over becomes `unresolved` entries. `shortage = effectiveDemand − served`.

Complexity: O(A · (V+E) log V), where A = number of augmentations (each saturates at least one edge, node, supply, or demand, so A ≤ E + 2V in practice). The limitation (no flow cancellation, so not guaranteed optimal) is documented. Upgrading to a residual-graph min-cost max-flow is a stretch goal only.

The same routine runs once at load to compute the baseline (`baselineServedPercent`), and again after each disruption.

### 6.5 Criticality ranking (`sim/ImpactAnalyzer`)
For each non-demand node: temporarily mark it `FAILED` on a scratch copy, re-run allocation, record `unmetDemandIfRemoved` and `increasePercent` relative to the current unmet demand, then sort descending. 22 nodes × one allocation each is cheap. Cache the result until the next mutation.

## 7. Simulation layer

- **DisruptionQueue:** my `Queue<Disruption>`. `POST /api/disruptions` enqueues one event, a scenario run enqueues several. The engine drains the queue FIFO, and **each event is applied then logged** before the allocation re-runs **once at the end of the drain**. (Intermediate allocation is unnecessary.)
- **DisruptionEngine:** validates (target exists, type matches target kind, severity in range, else `INVALID_DISRUPTION`) and applies the semantics table from the contract to the working graph. Replace-not-compound: store `severity` per target.
- **ImpactAnalyzer:** captures `unmetDemandBefore`, runs BFS exposure (from each disrupted element), DFS components, and builds the sorted shortage list (using the heap ordering).
- **RecoveryPlanner:** produces `allocations`, `unresolved`, and rule-based `recommendations` (REROUTE if a path now carries flow that wasn't on the baseline plan, ADD_CAPACITY for any saturated edge on an active allocation, LOAD_SHED for unresolved priority-3 shortages, RESTORE for each disrupted element).
- **EventLog:** my `LinkedList<Event>`, append-only. `Event { seq, tick, kind, message, refs }`. `tick` is a simulation counter, not wall time (deterministic in tests).
- **Simulation (facade):** owns the baseline graph, working graph, event queue, event log, cached results, and `version`. Methods: `network()`, `state()`, `applyDisruption()`, `runScenario(id)`, `criticalNodes()`, `route(from,to,metric)`, `events()`, `reset()`. **Guarded by one `std::mutex`**, because cpp-httplib handles requests on multiple threads.

## 8. Build order (milestones, each ends with passing tests and a commit)

| # | Milestone | Done when |
|---|---|---|
| B0 | Toolchain and skeleton (done in Phase 0) | `/api/health` works, doctest runs |
| B1 | `Queue`, `LinkedList` (can be handed to Abhigya) | Unit tests pass under ASan/valgrind |
| B2 | `HashTable` | Tests incl. rehash and collisions |
| B3 | `MinHeap` | Tests incl. random sequences vs sorted order |
| B4 | Model types + `Graph` + `NetworkLoader` | `data/network.json` loads, bad files rejected with clear errors |
| B5 | `GET /api/health`, `/api/network`, `/api/state` (initial) with **Serializer** | Frontend in `VITE_USE_MOCKS=false` draws the real network (**walking skeleton**) |
| B6 | BFS, DFS/components, Dijkstra, `GET /api/routes` | Route S1→D1 matches hand calculation |
| B7 | Allocation (ShortageManager + RecoveryPlanner), baseline served ≈ 100% | Baseline test passes; tune dataset if not |
| B8 | DisruptionQueue, Engine, ImpactAnalyzer, EventLog, `POST /api/disruptions`, `/api/events`, `/api/reset` | All 4 disruption types work end to end |
| B9 | Scenarios endpoints | All 5 presets run |
| B10 | ArticulationPoints + criticality ranking, `GET /api/analysis/critical-nodes` | Matches hand-checked graphs |
| B11 | Hardening: validation, error envelopes, mutex, ASan/valgrind clean, perf check | Fuzzed bad input never crashes |
| B12 | Docs and viva notes | `docs/dsa-justification.md` complete |

## 9. Test plan (minimum)

- **DS:** empty, single, duplicates, large (10⁵ ops), rehash correctness, heap property after random pushes/pops, list append and traversal order, no leaks.
- **Graph/loader:** duplicate id, dangling edge, missing field, bad enum, valid file.
- **Dijkstra:** hand-checked tiny graph (cost vs time giving different paths), unreachable target, failed-edge avoidance, deterministic tie-breaks.
- **Articulation points:** a path graph, a star, a cycle (none), two components, a root with 2 children.
- **Allocation:** baseline serves ≈ 100%. Priority ordering test: with scarce supply, P1 is served before P3. Capacity bottleneck test. Disconnected demand centre produces `NO_PATH`.
- **Scenarios (qualitative):**
  - SC02 (H2 outage): shortages appear at D2/D5, alternative routes through G1/G3 used.
  - SC03 (imports cut): total supply falls, so unmet demand is non-zero and P3 centres are shed first.
  - SC04 (demand surge): D1/D5 `effectiveDemand` rises, flows reroute.
  - Reset returns exactly to the baseline state.
- **API:** curl scripts or doctest + httplib client for each endpoint and each error code.

## 10. Risks and gotchas

- Dangling pointers/iterators in my own containers: use indices, run ASan.
- Heap with lazy deletion: always compare popped distance with the current best and skip stale entries.
- Floating-point equality for capacities: use a small epsilon or integer EU/day values. **Decision: use `double` with an `EPS = 1e-9` helper, or ints if the dataset stays integral.**
- Allocation infinite loops: guarantee progress (each loop iteration must reduce something or break).
- Concurrency: every public `Simulation` method locks the mutex.
- Contract drift: any shape change needs a PR to `contracts/`. Serializer is the only place that knows JSON shapes.

## 11. Viva cheat sheet (fill in as we go)

| Concept | Where used | Why this and not something else | Complexity |
|---|---|---|---|
| Adjacency list | `Graph` | Sparse graph (E ≈ 2V); matrix would waste O(V²) | O(V+E) space |
| Hash table | id lookups | O(1) vs O(n) scan on every API call | avg O(1) |
| Min-heap | Dijkstra, shortage priority | Fast extract-min | O(log n) |
| Queue | BFS, disruption events | FIFO order semantics | O(1) |
| Linked list | EventLog | Append-only history, O(1) append with tail pointer | O(1) append |
| BFS | Downstream exposure | Level-by-level reachability | O(V+E) |
| DFS | Components, articulation points | Needs discovery/low-link | O(V+E) |
| Dijkstra | Routes, allocation | Non-negative weights | O((V+E) log V) |

## 12. Progress tracker

- [x] B0 toolchain
- [ ] B1 Queue, LinkedList
- [ ] B2 HashTable
- [ ] B3 MinHeap
- [ ] B4 Graph + loader
- [ ] B5 walking skeleton
- [ ] B6 traversal + Dijkstra + routes
- [ ] B7 allocation
- [ ] B8 disruptions + events + reset
- [ ] B9 scenarios
- [ ] B10 critical analysis
- [ ] B11 hardening
- [ ] B12 docs

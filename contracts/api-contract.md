# GridFlow API Contract — v0.1 (DRAFT)

**Owners:** Shreyansh (backend), Abhigya (frontend).
**Change policy:** any change to this file goes through a PR approved by both. Bump the version and add a line to the Changelog.

---

## 1. Conventions

- Backend base URL: `http://127.0.0.1:8080`. All routes are under `/api`.
- The frontend dev server proxies `/api` to the backend (Vite proxy), so the browser sees one origin. The backend also sends permissive CORS headers in dev.
- JSON only, UTF-8, **camelCase** keys, all IDs are strings.
- Units: energy = **EU/day** (abstract energy units per day), cost = cost-units per EU, time = hours.
- The server holds **one shared simulation** (single session). No auth.
- `state.version` is an integer that increments after every successful mutation (disruption, scenario run, reset).

### Response envelope (every endpoint)

```json
{ "ok": true,  "data": { } }
{ "ok": false, "error": { "code": "NODE_NOT_FOUND", "message": "Node 'X9' does not exist", "details": { "id": "X9" } } }
```

### HTTP status codes

| Status | When |
|---|---|
| 200 | Success (including "no path found", which is a normal result) |
| 400 | Malformed body, missing or invalid parameter |
| 404 | Unknown node, edge, scenario, or route |
| 500 | Internal error |

### Error codes

`INVALID_JSON`, `INVALID_PARAMETER`, `INVALID_DISRUPTION`, `NODE_NOT_FOUND`, `EDGE_NOT_FOUND`, `SCENARIO_NOT_FOUND`, `NOT_FOUND`, `INTERNAL_ERROR`

---

## 2. Enums

| Enum | Values |
|---|---|
| NodeType | `SOURCE`, `IMPORT_TERMINAL`, `PROCESSING`, `DISTRIBUTION_HUB`, `POWER_PLANT`, `DEMAND_CENTRE` |
| NodeStatus | `ACTIVE`, `DEGRADED`, `FAILED` |
| EdgeStatus | `ACTIVE`, `REDUCED`, `FAILED` |
| DisruptionType | `NODE_OUTAGE`, `EDGE_DISRUPTION`, `SUPPLY_REDUCTION`, `DEMAND_SURGE` |
| RecommendationType | `REROUTE`, `RESTORE`, `ADD_CAPACITY`, `LOAD_SHED` |

### Disruption semantics

| Type | Target | `severity` | Effect |
|---|---|---|---|
| `NODE_OUTAGE` | any node | ignored (always full) | Node `FAILED`. All its edges carry nothing. |
| `EDGE_DISRUPTION` | an edge | (0,1], fraction of capacity lost | `REDUCED` if <1, `FAILED` if =1 |
| `SUPPLY_REDUCTION` | `SOURCE` or `IMPORT_TERMINAL` | (0,1], fraction of supply lost | `DEGRADED` if <1, `FAILED` if =1 |
| `DEMAND_SURGE` | `DEMAND_CENTRE` | (0,1], fractional demand increase (0.4 = +40%) | Node stays `ACTIVE`; `effectiveDemand` rises |

A later disruption on the same target **replaces** the earlier one (no compounding). `POST /api/reset` clears all.

---

## 3. Data shapes

### Node (static, from `GET /api/network`)

```json
{ "id": "H2", "name": "Central Hub", "type": "DISTRIBUTION_HUB",
  "capacity": 800, "supply": 0, "demand": 0, "priority": null,
  "position": { "x": 500, "y": 300 } }
```

- `capacity` = max throughput through the node, for every node.
- `supply` is non-zero only for `SOURCE` / `IMPORT_TERMINAL`.
- `demand` and `priority` (1 = critical, 3 = lowest) apply only to `DEMAND_CENTRE`; `priority` is `null` elsewhere.

### Edge (static)

```json
{ "id": "E13", "from": "P1", "to": "H2", "capacity": 400, "cost": 3, "transitTime": 3 }
```

Edges are **directed** (energy flows from → to).

### NodeState / EdgeState (dynamic, inside `state`)

```json
{ "id": "D1", "status": "ACTIVE", "effectiveSupply": 0, "effectiveDemand": 600,
  "served": 480, "shortage": 120, "throughput": 480 }

{ "id": "E13", "status": "REDUCED", "effectiveCapacity": 200, "flow": 180 }
```

`served` and `shortage` are meaningful for demand centres and `0` elsewhere. `throughput` = total flow passing through the node.

### State

```json
{ "version": 3,
  "nodes": [ /* NodeState */ ],
  "edges": [ /* EdgeState */ ],
  "summary": { "totalSupply": 2600, "totalDemand": 2000, "totalServed": 1740,
               "unmetDemand": 260, "servedPercent": 87.0, "baselineServedPercent": 100.0 } }
```

### Impact report

```json
{ "exposedNodes": ["H2", "G1", "G2", "D1"],
  "exposedEdges": ["E14", "E27"],
  "components": { "count": 2, "isolatedDemandCentres": ["D4"] },
  "shortages": [ { "nodeId": "D5", "name": "Capital Region", "priority": 1,
                   "demand": 400, "served": 250, "shortage": 150 } ],
  "unmetDemandBefore": 0, "unmetDemandAfter": 260 }
```

- `exposedNodes` / `exposedEdges`: everything downstream of the disrupted element (BFS exposure).
- `shortages` is sorted by priority (1 first), then by shortage descending.

### Recovery plan

```json
{ "allocations": [ { "demandCentreId": "D1", "path": ["S1","P1","H1","G1","D1"],
                     "edgeIds": ["E01","E12","E25","E33"], "flow": 300, "totalCost": 6, "totalTime": 8 } ],
  "unresolved": [ { "nodeId": "D4", "shortage": 80, "reason": "CAPACITY_EXHAUSTED" } ],
  "recommendations": [ { "type": "REROUTE", "targetId": "D1", "priority": 1,
                         "message": "Serve D1 via G1 using spare capacity on E25." } ] }
```

- `allocations` is the **complete** post-disruption flow plan (not only the changes).
- `unresolved.reason` is `NO_PATH` or `CAPACITY_EXHAUSTED`.

---

## 4. Endpoints

### `GET /api/health`
`data: { "service": "gridflow", "version": "0.1" }`

### `GET /api/network`
`data: { "nodes": [Node], "edges": [Edge] }`. Static, fetched once.

### `GET /api/state`
`data: State`.

### `POST /api/disruptions`
Request: `{ "type": "NODE_OUTAGE", "targetId": "H2", "severity": 1.0 }`
Response: `data: { "disruptions": [ { "seq": 1, "type": "...", "targetId": "...", "severity": 1.0 } ], "impact": Impact, "recovery": Recovery, "state": State }`
Errors: 400 `INVALID_DISRUPTION`, 404 `NODE_NOT_FOUND` / `EDGE_NOT_FOUND`.

### `GET /api/scenarios`
`data: { "scenarios": [ { "id": "SC02", "name": "...", "description": "...", "disruptions": [ { "type": "...", "targetId": "...", "severity": 1.0 } ] } ] }`

### `POST /api/scenarios/{id}/run`
Same response shape as `POST /api/disruptions`, with `disruptions` listing every applied event in order. The scenario is applied on top of the **current** state. The UI should offer a reset first. Errors: 404 `SCENARIO_NOT_FOUND`.

### `GET /api/analysis/critical-nodes`
```json
{ "articulationPoints": ["H2", "P3"],
  "ranking": [ { "nodeId": "H2", "name": "Central Hub", "unmetDemandIfRemoved": 420, "increasePercent": 21.0 } ] }
```
`articulationPoints` = structural weak points (undirected projection of the currently active network). `ranking` = flow-based criticality, sorted descending by `unmetDemandIfRemoved`.

### `GET /api/routes?from=S1&to=D1&metric=cost`
`metric` is `cost` or `time` (default `cost`).
```json
{ "metric": "cost", "found": true, "path": ["S1","P1","H1","G1","D1"],
  "edgeIds": ["E01","E12","E25","E33"], "totalCost": 6, "totalTime": 8, "bottleneckCapacity": 350 }
```
If no path exists: `found: false` with an empty `path`, still HTTP 200.

### `GET /api/events`
`data: { "events": [ { "seq": 1, "tick": 1, "kind": "DISRUPTION_APPLIED", "message": "H2 outage", "refs": ["H2"] } ] }`, oldest first.
`kind` is one of `DISRUPTION_APPLIED`, `ALLOCATION_RUN`, `SCENARIO_RUN`, `RESET`.

### `POST /api/reset`
Restores the baseline. `data: { "state": State }`.

---

## 5. Reference network (22 nodes, 40 edges, DRAFT dataset)

Single commodity. Numbers are draft. Backend will verify the baseline serves about 100% of demand, and we will tune capacities if not. Positions: x = layer × 200 + 100, y spread evenly per layer (the frontend may use any layout, but use `position` from the API).

### Nodes

| ID | Name | Type | Capacity | Supply | Demand | Priority |
|---|---|---|---|---|---|---|
| S1 | Offshore Field Alpha | SOURCE | 600 | 600 | 0 | – |
| S2 | Onshore Gas Field Beta | SOURCE | 400 | 400 | 0 | – |
| S3 | Domestic Coal Basin | SOURCE | 350 | 350 | 0 | – |
| T1 | Western Port Terminal | IMPORT_TERMINAL | 500 | 500 | 0 | – |
| T2 | Eastern Port Terminal | IMPORT_TERMINAL | 450 | 450 | 0 | – |
| T3 | Northern Pipeline Station | IMPORT_TERMINAL | 300 | 300 | 0 | – |
| P1 | Coastal Refinery | PROCESSING | 700 | 0 | 0 | – |
| P2 | Gas Processing Plant | PROCESSING | 600 | 0 | 0 | – |
| P3 | Inland Refinery | PROCESSING | 500 | 0 | 0 | – |
| P4 | Coal Handling Plant | PROCESSING | 350 | 0 | 0 | – |
| H1 | North Hub | DISTRIBUTION_HUB | 700 | 0 | 0 | – |
| H2 | Central Hub | DISTRIBUTION_HUB | 800 | 0 | 0 | – |
| H3 | South Hub | DISTRIBUTION_HUB | 700 | 0 | 0 | – |
| H4 | East Hub | DISTRIBUTION_HUB | 600 | 0 | 0 | – |
| G1 | Thermal Plant North | POWER_PLANT | 550 | 0 | 0 | – |
| G2 | Gas Power Plant Central | POWER_PLANT | 700 | 0 | 0 | – |
| G3 | Thermal Plant South | POWER_PLANT | 550 | 0 | 0 | – |
| D1 | Metro City | DEMAND_CENTRE | 900 | 0 | 600 | 2 |
| D2 | Industrial Zone | DEMAND_CENTRE | 750 | 0 | 500 | 3 |
| D3 | Port City | DEMAND_CENTRE | 450 | 0 | 300 | 3 |
| D4 | Rural District | DEMAND_CENTRE | 300 | 0 | 200 | 2 |
| D5 | Capital Region | DEMAND_CENTRE | 600 | 0 | 400 | 1 |

Total supply = 2600, total demand = 2000.

### Edges (directed: from → to)

| ID | From | To | Capacity | Cost | Time |
|---|---|---|---|---|---|
| E01 | S1 | P1 | 500 | 2 | 4 |
| E02 | S1 | P2 | 300 | 3 | 5 |
| E03 | S2 | P2 | 400 | 2 | 3 |
| E04 | S2 | P3 | 200 | 4 | 6 |
| E05 | S3 | P4 | 350 | 1 | 2 |
| E06 | T1 | P1 | 400 | 3 | 2 |
| E07 | T1 | P2 | 200 | 4 | 3 |
| E08 | T2 | P3 | 400 | 3 | 3 |
| E09 | T2 | P4 | 150 | 5 | 4 |
| E10 | T3 | P3 | 300 | 2 | 5 |
| E11 | T3 | P2 | 150 | 4 | 6 |
| E12 | P1 | H1 | 400 | 2 | 2 |
| E13 | P1 | H2 | 400 | 3 | 3 |
| E14 | P2 | H2 | 450 | 2 | 2 |
| E15 | P2 | H3 | 300 | 3 | 3 |
| E16 | P3 | H3 | 350 | 2 | 3 |
| E17 | P3 | H4 | 300 | 3 | 4 |
| E18 | P4 | H4 | 250 | 2 | 2 |
| E19 | P4 | H3 | 150 | 4 | 4 |
| E20 | H1 | H2 | 200 | 1 | 1 |
| E21 | H2 | H3 | 200 | 1 | 1 |
| E22 | H3 | H4 | 150 | 1 | 1 |
| E23 | H2 | H1 | 200 | 1 | 1 |
| E24 | H3 | H2 | 200 | 1 | 1 |
| E25 | H1 | G1 | 450 | 1 | 1 |
| E26 | H2 | G1 | 200 | 2 | 2 |
| E27 | H2 | G2 | 500 | 1 | 1 |
| E28 | H3 | G2 | 250 | 2 | 2 |
| E29 | H3 | G3 | 400 | 1 | 1 |
| E30 | H4 | G3 | 200 | 2 | 2 |
| E31 | H2 | D2 | 250 | 2 | 2 |
| E32 | H4 | D3 | 150 | 2 | 2 |
| E33 | G1 | D1 | 350 | 1 | 1 |
| E34 | G1 | D5 | 250 | 2 | 2 |
| E35 | G2 | D1 | 300 | 2 | 2 |
| E36 | G2 | D2 | 300 | 1 | 1 |
| E37 | G2 | D5 | 200 | 2 | 2 |
| E38 | G3 | D3 | 250 | 1 | 1 |
| E39 | G3 | D4 | 200 | 1 | 1 |
| E40 | G3 | D2 | 150 | 3 | 3 |

---

## 6. Scenario presets

| ID | Name | Disruptions |
|---|---|---|
| SC01 | Cyclone on Western Coast | `NODE_OUTAGE T1`; `EDGE_DISRUPTION E12 0.6` |
| SC02 | Central Hub Fire | `NODE_OUTAGE H2` |
| SC03 | Import Restrictions | `SUPPLY_REDUCTION T2 0.7`; `SUPPLY_REDUCTION T3 1.0` |
| SC04 | Heatwave Demand Surge | `DEMAND_SURGE D1 0.4`; `DEMAND_SURGE D5 0.3` |
| SC05 | Cascading Failure | `NODE_OUTAGE P2`; `EDGE_DISRUPTION E13 1.0` |

---

## 7. Changelog

- v0.1: initial draft.

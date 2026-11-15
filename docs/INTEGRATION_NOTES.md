# Backend integration notes

## Existing Track B interfaces kept intact

- Traversal is implemented in `backend/src/algo/Traversal.h`, not `include/gridflow/algo/Traversal.h`. `TraversalResult` exposes both string IDs and node/edge indices; `ComponentInfo` contains `count` and isolated demand-centre IDs rather than a component-label array. The facade consumes the existing signatures.
- Articulation points live in `backend/src/algo/ArticulationPoints.h` and return node indices.
- `DisruptionQueue`, `EventLog`, `ImpactAnalyzer`, `DisruptionEngine`, and `Serializer` are in `backend/src/...`, not the prompt's expected include paths. Includes were adapted to those actual locations.
- `Serializer.h` already declares JSON-facing `PathResult`, `Allocation`, `UnresolvedDemand`, `Recommendation`, and `RecoveryPlan`. The core modules therefore use `SearchPath`, `AllocationEntry`, `Unresolved`, and `RecoveryPlanData`, then translate at the API boundary. No existing public type was renamed or redefined.
- The event log assigns `tick` from its monotonic sequence. The facade preserves that deterministic behavior and logs `SCENARIO_RUN` before scenario disruptions, then `ALLOCATION_RUN`.
- `stateToJson(const Graph&)` uses placeholder version and baseline percentage. An overload accepts the facade's real version and cached baseline percentage; the original function remains available.
- `kEpsilon` is defined once in `backend/include/gridflow/model/Constants.h`; the existing Track B modules had no shared epsilon constant.

## Minimal existing-code integration fix

`backend/src/sim/ImpactAnalyzer.cpp` used `std::sort` without including `<algorithm>`, causing the integrated project not to compile. Added the missing include only. The existing ImpactAnalyzer tests exercise shortage ordering and pass.

## Formula decision

Criticality `increasePercent` uses `(unmetDemandIfRemoved - currentUnmet) / totalEffectiveDemand * 100`, matching the API contract example (420 of 2000 is 21%). This resolves the different denominator wording in the technical spec.

## Dataset

`data/network.json` is copied byte-for-byte from `contracts/examples/network.json`. The baseline allocation serves at least 99.5% and has no unresolved priority-1 demand, so no capacities were tuned.

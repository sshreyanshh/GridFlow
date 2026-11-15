# GridFlow backend data structures and algorithms

| Structure or algorithm | Use | Why this choice | Time and space |
|---|---|---|---|
| Adjacency-list `Graph` | Stores the directed energy network and indexes node/edge IDs | The network is sparse; in/out adjacency avoids a quadratic matrix and supports both route directions | O(V+E) storage; average O(1) ID lookup through the custom hash table |
| `HashTable` | Graph ID lookup and scenario ID lookup | IDs are referenced on every API request and disruption; average O(1) lookup avoids repeated scans | Average O(1) lookup/insert; O(n) worst case; O(n) storage |
| `MinHeap` | Dijkstra frontier and demand priority ordering | Both algorithms repeatedly need the smallest deterministic key; lazy duplicate entries avoid a decrease-key implementation | O(log V) push/pop; Dijkstra O((V+E) log V) |
| Dijkstra | Forward route queries and reverse residual search during allocation | Non-negative cost/time weights give a shortest path; reverse traversal finds the least-cost available supplier | O((V+E) log V) time; O(V+E) auxiliary storage |
| Greedy successive cheapest path | Assigns energy to demand centres in priority order | It is explainable and honors critical-load priority while respecting supply, edge, and node limits | O(A(V+E)logV), A path augmentations; O(V+E) storage |
| `ScenarioLibrary` vector + custom hash table | Ordered scenario listing and ID lookup | The vector preserves file order; the hash table provides fast lookup | O(S) storage; average O(1) lookup |
| Graph value copies and one mutex | Simulation baseline/working state and concurrent HTTP access | Copies support rollback and reset; the mutex makes each facade operation a consistent snapshot or mutation | Copy O(V+E); each public call serializes behind one mutex |
| Flow criticality | Reallocates after hypothetically failing each non-demand node | Measures actual service loss, which an undirected structural measure cannot capture | O(V·A(V+E)logV) for V candidate nodes |

## Allocation limitation

Allocation is a deterministic greedy heuristic. It does not cancel or reroute flow chosen in earlier iterations, so it is not guaranteed to find a globally optimal flow. Min-cost max-flow with a residual graph is the stretch goal.

## Structural articulation versus flow criticality

Articulation points identify nodes whose removal splits the undirected projection of the working network. Criticality reruns the directed, capacity-limited allocation after removing each eligible node. The first is a structural property; the second measures added unmet demand. They complement one another.

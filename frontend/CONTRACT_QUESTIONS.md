# Contract questions and implementation assumptions

- The contract’s edge state has no explicit disrupted-capacity field beyond `effectiveCapacity`; the mock reports post-disruption capacity there.
- Event refresh is oldest-first in the API and is reversed for display as required.
- The mock allocator uses repeated least-cost paths with residual capacities and serves demand centres in priority order. It does not model a globally optimal multi-commodity flow.
- The contract says scenario runs apply to current state; the UI runs them as requested and leaves reset available at the top of the page.
- Component counts and recommendation rules are not fully specified; the mock provides simple exposure and shortage based results.
- The India view uses approximate site coordinates for the sample facilities; city hubs are illustrative, and the retained 40 links are simulation topology rather than verified infrastructure routes.

# GridFlow Frontend

GridFlow is a dark control room dashboard for inspecting a directed energy supply network, testing disruptions, and exploring service impacts and recovery routes.

## Setup

From this folder, run `npm install` then `npm run dev`. The app starts in offline mock mode by default. The mock simulator adds a short delay to make request states visible.

## Scripts

- `npm run dev` starts the Vite development server.
- `npm run build` creates the production bundle in `dist/`.
- `npm run lint` runs ESLint.
- `npm test` runs the Vitest simulator tests.

## Environment

Copy `.env.example` as needed. `VITE_USE_MOCKS=true` selects the local in-memory simulator. Set `VITE_USE_MOCKS=false` for the HTTP API. `VITE_API_BASE` defaults to `/api`; Vite proxies this path to `http://127.0.0.1:8080` during development.

## Structure

`src/api/` contains the real/mock clients and errors. `src/mocks/` holds the mock simulation and its unit tests. `src/hooks/` contains shared simulation state. `src/components/` contains the Cytoscape network view, `src/pages/` contains the dashboard, and `src/styles/` holds the theme and layout. Contract reference fixtures live in `../contracts/examples/`.

## Mock simulation

The simulator applies target disruptions as replacements, allocates supply over least-cost paths in priority order, tracks flow and shortages, derives downstream exposure, computes routes, and ranks critical nodes by re-running allocation with each node removed. The mock is intended for interface development; it does not reproduce an optimized backend flow solver. `?fail=1` simulates a mock request failure.

## India network map

The network view places the 22 sample nodes at approximate locations of Indian energy facilities and demand centres over a state map by the [Survey of India](https://commons.wikimedia.org/wiki/File:States_of_India_(Survey_of_India).svg), released into the public domain. Facility examples include ONGC Mumbai High, Reliance KG-D6 and Jamnagar, Petronet Dahej and Kochi LNG, and NTPC Vindhyachal, Ramagundam, Simhadri, and Sipat. City distribution hubs and demand centres are illustrative. The 40 simulated links are retained from the sample network and do not represent verified pipeline or transmission routes.

Facility references: [ONGC Mumbai High](https://ongcindia.com/web/eng/detail?assetClassPK=70712619&assetEntry=70712624), [Reliance manufacturing locations](https://www.ril.com/about/manufacturing-locations), [Petronet LNG terminals](https://www.petronetlng.in/), and [NTPC station pages](https://ntpc.co.in/sipat).

## Known limitations

The draft API contract defines impact and recovery outputs but leaves some details such as component counting and recovery recommendation rules open. The mock uses a simple path allocator; its flow plan and criticality rankings may differ from the eventual backend. The shared simulation is in-memory and resets on page reload.

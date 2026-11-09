export function shortestPath(network, start, goal, metric = 'cost', residual = null, removed = new Set()) {
  const dist = new Map([[start, 0]]); const prev = new Map(); const pending = new Set(network.nodes.map((n) => n.id));
  while (pending.size) {
    let id = null; let best = Infinity;
    for (const candidate of pending) if ((dist.get(candidate) ?? Infinity) < best) { id = candidate; best = dist.get(candidate); }
    if (id === null || id === goal) break;
    pending.delete(id);
    for (const edge of network.edges.filter((e) => e.from === id && !removed.has(e.to) && (residual?.get(e.id) ?? e.capacity) > 0)) {
      const cost = metric === 'time' ? edge.transitTime : edge.cost; const next = best + cost;
      if (next < (dist.get(edge.to) ?? Infinity)) { dist.set(edge.to, next); prev.set(edge.to, edge); }
    }
  }
  if (!dist.has(goal)) return null;
  const edges = []; let cursor = goal;
  while (cursor !== start) { const edge = prev.get(cursor); if (!edge) return null; edges.unshift(edge); cursor = edge.from; }
  return { nodes: [start, ...edges.map((e) => e.to)], edges, total: dist.get(goal) };
}

function allocate(network, disruptionMap, removed = new Set()) {
  const edgeResidual = new Map(); const nodeResidual = new Map(); const supply = new Map(); const initialSupply = new Map(); const demand = new Map(); const nodeStatus = new Map(); const edgeStatus = new Map();
  for (const node of network.nodes) {
    const d = disruptionMap.get(node.id); let available = node.supply; let effectiveDemand = node.demand;
    if (d?.type === 'NODE_OUTAGE') available = 0;
    if (d?.type === 'SUPPLY_REDUCTION') available *= 1 - d.severity;
    if (d?.type === 'DEMAND_SURGE') effectiveDemand *= 1 + d.severity;
    supply.set(node.id, available); initialSupply.set(node.id, available); demand.set(node.id, effectiveDemand); nodeResidual.set(node.id, removed.has(node.id) ? 0 : node.capacity);
    nodeStatus.set(node.id, removed.has(node.id) || d?.type === 'NODE_OUTAGE' ? 'FAILED' : d?.type === 'SUPPLY_REDUCTION' && d.severity === 1 ? 'FAILED' : d?.type === 'SUPPLY_REDUCTION' ? 'DEGRADED' : 'ACTIVE');
  }
  for (const edge of network.edges) {
    const d = disruptionMap.get(edge.id); let capacity = edge.capacity;
    if (d?.type === 'EDGE_DISRUPTION') capacity *= 1 - d.severity;
    if (disruptionMap.get(edge.from)?.type === 'NODE_OUTAGE' || disruptionMap.get(edge.to)?.type === 'NODE_OUTAGE' || removed.has(edge.from) || removed.has(edge.to)) capacity = 0;
    edgeResidual.set(edge.id, capacity); edgeStatus.set(edge.id, capacity === 0 ? 'FAILED' : capacity < edge.capacity ? 'REDUCED' : 'ACTIVE');
  }
  const flows = new Map(network.edges.map((e) => [e.id, 0])); const throughput = new Map(network.nodes.map((n) => [n.id, 0])); const allocations = [];
  const centres = network.nodes.filter((n) => n.type === 'DEMAND_CENTRE' && !removed.has(n.id)).sort((a, b) => a.priority - b.priority || a.id.localeCompare(b.id));
  for (const centre of centres) {
    let left = demand.get(centre.id); let guard = 0;
    while (left > 0.001 && guard++ < 50) {
      let best = null; let supplier = null;
      for (const source of network.nodes.filter((n) => (supply.get(n.id) || 0) > 0.001 && nodeResidual.get(n.id) > 0.001)) {
        const path = shortestPath(network, source.id, centre.id, 'cost', edgeResidual, new Set(network.nodes.filter((n) => nodeResidual.get(n.id) <= 0.001).map((n) => n.id)));
        if (path && (!best || path.total < best.total)) { best = path; supplier = source; }
      }
      if (!best) break;
      const bottleneck = Math.min(left, supply.get(supplier.id), nodeResidual.get(centre.id), ...best.nodes.map((id) => nodeResidual.get(id)), ...best.edges.map((e) => edgeResidual.get(e.id)));
      if (bottleneck <= 0.001) break;
      supply.set(supplier.id, supply.get(supplier.id) - bottleneck);
      for (const id of best.nodes) { nodeResidual.set(id, nodeResidual.get(id) - bottleneck); throughput.set(id, throughput.get(id) + bottleneck); }
      for (const edge of best.edges) { edgeResidual.set(edge.id, edgeResidual.get(edge.id) - bottleneck); flows.set(edge.id, flows.get(edge.id) + bottleneck); }
      allocations.push({ demandCentreId: centre.id, path: best.nodes, edgeIds: best.edges.map((e) => e.id), flow: bottleneck, totalCost: best.total, totalTime: best.edges.reduce((sum, e) => sum + e.transitTime, 0) }); left -= bottleneck;
    }
  }
  const nodeStates = network.nodes.map((n) => { const served = n.type === 'DEMAND_CENTRE' ? allocations.filter((a) => a.demandCentreId === n.id).reduce((s, a) => s + a.flow, 0) : 0; const effectiveDemand = demand.get(n.id); return { id: n.id, status: nodeStatus.get(n.id), effectiveSupply: initialSupply.get(n.id), effectiveDemand, served, shortage: Math.max(0, effectiveDemand - served), throughput: throughput.get(n.id) }; });
  const edgeStates = network.edges.map((e) => ({ id: e.id, status: edgeStatus.get(e.id), effectiveCapacity: e.capacity - (e.capacity - (edgeResidual.get(e.id) + flows.get(e.id))), flow: flows.get(e.id) }));
  const totalDemand = nodeStates.reduce((s, n) => s + n.effectiveDemand, 0); const totalServed = nodeStates.reduce((s, n) => s + n.served, 0);
  return { nodeStates, edgeStates, allocations, summary: { totalSupply: nodeStates.reduce((s, n) => s + n.effectiveSupply, 0), totalDemand, totalServed, unmetDemand: Math.max(0, totalDemand - totalServed), servedPercent: totalDemand ? 100 * totalServed / totalDemand : 100, baselineServedPercent: 100 } };
}

export function createState(network, version = 0) { const result = allocate(network, new Map()); const state = { version, nodes: result.nodeStates, edges: result.edgeStates, summary: result.summary }; return { version, state, disruptions: [], response: { disruptions: [], impact: { exposedNodes: [], exposedEdges: [], components: { count: 1, isolatedDemandCentres: [] }, shortages: [], unmetDemandBefore: 0, unmetDemandAfter: 0 }, recovery: { allocations: result.allocations, unresolved: [], recommendations: [] }, state } }; }

function exposed(network, disruptions) { const nodes = new Set(); const edges = new Set(); const queue = [];
  for (const d of disruptions) if (d.type === 'NODE_OUTAGE' || d.type === 'SUPPLY_REDUCTION') queue.push(d.targetId); else if (d.type === 'EDGE_DISRUPTION') { const edge = network.edges.find((e) => e.id === d.targetId); if (edge) { edges.add(edge.id); queue.push(edge.to); } }
  while (queue.length) { const id = queue.shift(); if (nodes.has(id)) continue; nodes.add(id); for (const edge of network.edges.filter((e) => e.from === id)) if (!edges.has(edge.id)) { edges.add(edge.id); queue.push(edge.to); } }
  return { exposedNodes: [...nodes], exposedEdges: [...edges] };
}

export function apply(network, previous, disruptions) {
  const map = new Map(); for (const item of [...previous.disruptions, ...disruptions]) map.set(item.targetId, item);
  const result = allocate(network, map); const state = { version: (previous.version || 0) + 1, nodes: result.nodeStates, edges: result.edgeStates, summary: result.summary };
  const shortages = result.nodeStates.filter((n) => n.shortage > 0).map((n) => ({ nodeId: n.id, name: network.nodes.find((x) => x.id === n.id).name, priority: network.nodes.find((x) => x.id === n.id).priority, demand: n.effectiveDemand, served: n.served, shortage: n.shortage })).sort((a, b) => a.priority - b.priority || b.shortage - a.shortage);
  const recommendations = disruptions.map((d) => ({ type: 'RESTORE', targetId: d.targetId, priority: 1, message: `Restore ${d.targetId} to recover its baseline capacity.` }));
  for (const edge of state.edges.filter((e) => e.effectiveCapacity > 0 && e.flow >= e.effectiveCapacity - 0.001)) recommendations.push({ type: 'ADD_CAPACITY', targetId: edge.id, priority: 2, message: `${edge.id} is saturated at ${edge.flow.toFixed(0)} EU/day.` });
  for (const s of shortages.filter((item) => item.priority === 3)) recommendations.push({ type: 'LOAD_SHED', targetId: s.nodeId, priority: 3, message: `${s.shortage.toFixed(0)} EU/day remains unserved at ${s.name}.` });
  for (const a of result.allocations.filter((item) => item.path.length > 3)) recommendations.push({ type: 'REROUTE', targetId: a.demandCentreId, priority: 2, message: `Route ${a.demandCentreId} through ${a.path.slice(1, -1).join(' → ')} to carry ${a.flow.toFixed(0)} EU/day.` });
  const before = previous.state.summary.unmetDemand; const response = { disruptions: [...previous.disruptions, ...disruptions].map((d, i) => ({ seq: i + 1, ...d })), impact: { ...exposed(network, disruptions), components: { count: 1, isolatedDemandCentres: [] }, shortages, unmetDemandBefore: before, unmetDemandAfter: state.summary.unmetDemand }, recovery: { allocations: result.allocations, unresolved: shortages.map((s) => ({ nodeId: s.nodeId, shortage: s.shortage, reason: 'CAPACITY_EXHAUSTED' })), recommendations }, state };
  return { version: state.version, state, disruptions: [...previous.disruptions, ...disruptions], response };
}

export function critical(network, state) {
  const adjacency = new Map(network.nodes.map((n) => [n.id, new Set()]));
  for (const e of network.edges) if (state.edges.find((x) => x.id === e.id)?.status !== 'FAILED') { adjacency.get(e.from).add(e.to); adjacency.get(e.to).add(e.from); }
  let time = 0; const disc = new Map(); const low = new Map(); const parents = new Map(); const points = new Set();
  function visit(u) { disc.set(u, ++time); low.set(u, time); let children = 0; for (const v of adjacency.get(u)) { if (!disc.has(v)) { children++; parents.set(v, u); visit(v); low.set(u, Math.min(low.get(u), low.get(v))); if (!parents.has(u) && children > 1) points.add(u); if (parents.has(u) && low.get(v) >= disc.get(u)) points.add(u); } else if (v !== parents.get(u)) low.set(u, Math.min(low.get(u), disc.get(v))); } }
  for (const n of network.nodes) if (!disc.has(n.id)) visit(n.id);
  const activeDisruptions = new Map();
  for (const n of network.nodes) {
    const current = state.nodes.find((x) => x.id === n.id);
    if (current.status === 'FAILED' && n.supply === 0) activeDisruptions.set(n.id, { type: 'NODE_OUTAGE', targetId: n.id, severity: 1 });
    else if (n.supply > 0 && current.effectiveSupply < n.supply) activeDisruptions.set(n.id, { type: 'SUPPLY_REDUCTION', targetId: n.id, severity: 1 - current.effectiveSupply / n.supply });
    else if (n.demand > 0 && current.effectiveDemand > n.demand) activeDisruptions.set(n.id, { type: 'DEMAND_SURGE', targetId: n.id, severity: current.effectiveDemand / n.demand - 1 });
  }
  for (const e of network.edges) {
    const current = state.edges.find((x) => x.id === e.id);
    if (current.effectiveCapacity < e.capacity) activeDisruptions.set(e.id, { type: 'EDGE_DISRUPTION', targetId: e.id, severity: 1 - current.effectiveCapacity / e.capacity });
  }
  const ranking = network.nodes.map((n) => { const result = allocate(network, activeDisruptions, new Set([n.id])); return { nodeId: n.id, name: n.name, unmetDemandIfRemoved: result.summary.unmetDemand, increasePercent: result.summary.unmetDemand / Math.max(1, state.summary.totalDemand) * 100 }; }).sort((a, b) => b.unmetDemandIfRemoved - a.unmetDemandIfRemoved);
  return { articulationPoints: [...points], ranking };
}

export function route(network, state, from, to, metric) { const failed = new Set(state.nodes.filter((n) => n.status === 'FAILED').map((n) => n.id)); const path = shortestPath(network, from, to, metric, new Map(state.edges.map((e) => [e.id, e.effectiveCapacity])), failed); if (!path) return { metric, found: false, path: [], edgeIds: [], totalCost: 0, totalTime: 0, bottleneckCapacity: 0 }; return { metric, found: true, path: path.nodes, edgeIds: path.edges.map((e) => e.id), totalCost: path.edges.reduce((s, e) => s + e.cost, 0), totalTime: path.edges.reduce((s, e) => s + e.transitTime, 0), bottleneckCapacity: Math.min(...path.edges.map((e) => state.edges.find((x) => x.id === e.id).effectiveCapacity)) }; }

export const simulate = { apply, critical, route };

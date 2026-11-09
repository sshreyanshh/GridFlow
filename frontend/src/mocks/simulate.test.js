import { describe, expect, it } from 'vitest';
import network from '../../../contracts/examples/network.json';
import { apply, createState, shortestPath } from './simulate.js';

describe('simulation helpers', () => {
  it('finds the cheapest directed route', () => {
    const path = shortestPath(network, 'S1', 'D1', 'cost');
    expect(path.nodes[0]).toBe('S1');
    expect(path.nodes.at(-1)).toBe('D1');
    expect(path.total).toBeGreaterThan(0);
  });
  it('allocates critical demand before lower priority demand', () => {
    const baseline = createState(network);
    const disrupted = apply(network, baseline, [{ type: 'SUPPLY_REDUCTION', targetId: 'S1', severity: 1 }]);
    const criticalShortage = disrupted.response.impact.shortages.filter((s) => s.priority === 1).reduce((s, r) => s + r.shortage, 0);
    const lowerShortage = disrupted.response.impact.shortages.filter((s) => s.priority > 1).reduce((s, r) => s + r.shortage, 0);
    expect(criticalShortage).toBeLessThanOrEqual(lowerShortage);
  });
  it('detects articulation points in the active undirected projection', async () => {
    const { simulate } = await import('./simulate.js');
    const baseline = createState(network);
    const result = simulate.critical(network, baseline.state);
    expect(Array.isArray(result.articulationPoints)).toBe(true);
    expect(result.ranking.length).toBe(network.nodes.length);
  });
  it('replaces a target disruption instead of compounding it', () => {
    const baseline = createState(network);
    const first = apply(network, baseline, [{ type: 'SUPPLY_REDUCTION', targetId: 'T2', severity: 0.5 }]);
    const second = apply(network, first, [{ type: 'SUPPLY_REDUCTION', targetId: 'T2', severity: 0.2 }]);
    expect(second.state.nodes.find((n) => n.id === 'T2').effectiveSupply).toBe(360);
  });
});

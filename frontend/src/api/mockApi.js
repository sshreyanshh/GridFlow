import network from '../../../contracts/examples/network.json';
import { scenarios } from '../../../contracts/examples/scenarios.json';
import { createState, simulate } from '../mocks/simulate.js';
import { ApiError } from './errors.js';
let current = createState(network);
let events = [];
let seq = 0;
const wait = () => new Promise((resolve) => setTimeout(resolve, 350));
async function done(value) { await wait(); if (new URLSearchParams(location.search).get('fail') === '1') throw new ApiError('INTERNAL_ERROR', 'Simulated mock failure', null, 500); return structuredClone(value); }
export const getHealth = async () => done({ service: 'gridflow', version: '0.1' });
export const getNetwork = async () => done(network);
export const getState = async () => done(current.state);
export const getScenarios = async () => done({ scenarios });
export const getEvents = async () => done({ events });
export const getCriticalNodes = async () => done(simulate.critical(network, current.state));
export const getRoute = async (from, to, metric) => done(simulate.route(network, current.state, from, to, metric));
export async function applyDisruption(disruption) { current = simulate.apply(network, current, [disruption]); events.push({ seq: ++seq, tick: seq, kind: 'DISRUPTION_APPLIED', message: `${disruption.type.replaceAll('_', ' ')} · ${disruption.targetId}`, refs: [disruption.targetId] }); return done(current.response); }
export async function runScenario(id) { const scenario = scenarios.find((item) => item.id === id); if (!scenario) throw new ApiError('SCENARIO_NOT_FOUND', `Scenario ${id} was not found`, { id }, 404); current = simulate.apply(network, current, scenario.disruptions); events.push({ seq: ++seq, tick: seq, kind: 'SCENARIO_RUN', message: `${scenario.name} run`, refs: [id] }); return done(current.response); }
export async function resetSimulation() { current = createState(network, (current.version || 0) + 1); events.push({ seq: ++seq, tick: seq, kind: 'RESET', message: 'Simulation reset to baseline', refs: [] }); return done({ state: current.state }); }

import { ApiError } from './errors.js';
const base = import.meta.env.VITE_API_BASE || '/api';
async function request(path, options = {}) {
  let response;
  try { response = await fetch(`${base}${path}`, { headers: { 'Content-Type': 'application/json' }, ...options }); }
  catch { throw new ApiError('BACKEND_UNREACHABLE', 'Backend is unreachable. Start the GridFlow API or switch to mock mode.'); }
  const body = await response.json();
  if (!response.ok || body.ok === false) { const error = body.error || {}; throw new ApiError(error.code || 'HTTP_ERROR', error.message || 'Request failed', error.details, response.status); }
  return body.data ?? body;
}
export const getHealth = () => request('/health');
export const getNetwork = () => request('/network');
export const getState = () => request('/state');
export const getScenarios = () => request('/scenarios');
export const getEvents = () => request('/events');
export const getCriticalNodes = () => request('/analysis/critical-nodes');
export const applyDisruption = (d) => request('/disruptions', { method: 'POST', body: JSON.stringify(d) });
export const runScenario = (id) => request(`/scenarios/${encodeURIComponent(id)}/run`, { method: 'POST' });
export const getRoute = (from, to, metric) => request(`/routes?${new URLSearchParams({ from, to, metric })}`);
export const resetSimulation = () => request('/reset', { method: 'POST' });

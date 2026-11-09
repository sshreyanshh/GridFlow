import { createContext, useCallback, useContext, useEffect, useState } from 'react';
import { api } from '../api/client.js';
const Context = createContext(null);
export function SimulationProvider({ children }) {
  const [data, setData] = useState(null); const [error, setError] = useState(null); const [busy, setBusy] = useState(''); const [events, setEvents] = useState([]); const [critical, setCritical] = useState(null);
  const load = useCallback(async () => { setError(null); try { const [network, state, scenarios, eventData] = await Promise.all([api.getNetwork(), api.getState(), api.getScenarios(), api.getEvents()]); setData({ network, state, scenarios: scenarios.scenarios, impact: null, recovery: null }); setEvents(eventData.events); } catch (e) { setError(e); } }, []);
  useEffect(() => { load(); }, [load]);
  const refreshEvents = async () => setEvents((await api.getEvents()).events);
  const mutate = async (name, fn) => { if (busy) return; setBusy(name); setError(null); try { const res = await fn(); if (res.state.version >= (data?.state.version ?? -1)) setData((d) => ({ ...d, state: res.state, impact: res.impact ?? null, recovery: res.recovery ?? null })); await refreshEvents(); return res; } catch (e) { setError(e); throw e; } finally { setBusy(''); } };
  const apply = (d) => mutate('apply', () => api.applyDisruption(d)); const run = (id) => mutate('scenario', () => api.runScenario(id));
  const reset = () => mutate('reset', async () => { const res = await api.resetSimulation(); return { state: res.state, impact: null, recovery: null }; });
  const loadCritical = async () => { setBusy('critical'); try { setCritical(await api.getCriticalNodes()); } catch (e) { setError(e); } finally { setBusy(''); } };
  const clearError = () => setError(null);
  return <Context.Provider value={{ data, error, busy, events, critical, load, apply, run, reset, loadCritical, refreshEvents, clearError, setError }}>{children}</Context.Provider>;
}
export const useSimulation = () => useContext(Context);

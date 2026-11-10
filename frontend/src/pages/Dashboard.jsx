import { useCallback, useEffect, useMemo, useState } from 'react';
import { api } from '../api/client.js';
import { useSimulation } from '../hooks/SimulationContext.jsx';
import NetworkGraph from '../components/NetworkGraph.jsx';

const tabs = [
  { id: 'Summary', label: 'Summary', icon: 'summary' },
  { id: 'Shortages', label: 'Shortages', icon: 'shortages' },
  { id: 'Recovery', label: 'Recovery', icon: 'recovery' },
  { id: 'Critical', label: 'Critical Nodes', icon: 'critical' },
  { id: 'Routes', label: 'Route Finder', icon: 'routes' },
  { id: 'Events', label: 'Event Log', icon: 'events' },
];
const kindLabels = { SOURCE: 'Sources', IMPORT_TERMINAL: 'Terminals', PROCESSING: 'Processing', DISTRIBUTION_HUB: 'Hubs', POWER_PLANT: 'Power plants', DEMAND_CENTRE: 'Demand centres' };
const utilityIconPaths = {
  controls: <><path d="M4 21v-7m0-4V3m8 18v-9m0-4V3m8 18v-5m0-4V3"/><path d="M2 14h4m4-6h4m4 8h4"/></>,
  telemetry: <><path d="M3 12h4l3-8 4 16 3-8h4"/></>,
  reset: <><path d="M3 12a9 9 0 1 0 2.64-6.36L3 8"/><path d="M3 3v5h5"/></>,
  sun: <><circle cx="12" cy="12" r="4"/><path d="M12 2v2m0 16v2M4.93 4.93l1.42 1.42m11.3 11.3 1.42 1.42M2 12h2m16 0h2M4.93 19.07l1.42-1.42m11.3-11.3 1.42-1.42"/></>,
  moon: <path d="M20.985 12.486a9 9 0 1 1-9.473-9.472c.405-.022.617.463.402.807a6.5 6.5 0 0 0 8.268 8.268c.344-.215.829-.003.803.397Z"/>,
  chevronLeft: <path d="m15 18-6-6 6-6"/>,
  chevronRight: <path d="m9 18 6-6-6-6"/>,
};
const navIconPaths = {
  summary: <><rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/></>,
  shortages: <><path d="M12 3 2.8 19h18.4L12 3Z"/><path d="M12 9v4m0 3h.01"/></>,
  recovery: <><path d="M3 12a9 9 0 0 1 15.4-6.4L21 8"/><path d="M21 3v5h-5M21 12a9 9 0 0 1-15.4 6.4L3 16"/><path d="M3 21v-5h5"/></>,
  critical: <><path d="M12 3v5m0 8v5M3 12h5m8 0h5"/><circle cx="12" cy="12" r="3"/></>,
  routes: <><circle cx="6" cy="6" r="2"/><circle cx="18" cy="18" r="2"/><path d="M8 6h3a3 3 0 0 1 3 3v3a3 3 0 0 0 3 3h1"/></>,
  events: <><path d="M4 5h16M4 12h16M4 19h16"/><circle cx="2.5" cy="5" r=".5" fill="currentColor"/><circle cx="2.5" cy="12" r=".5" fill="currentColor"/><circle cx="2.5" cy="19" r=".5" fill="currentColor"/></>,
};

function NavIcon({ name }) {
  return <svg aria-hidden="true" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round">{navIconPaths[name]}</svg>;
}

function UtilityIcon({ name }) {
  return <svg aria-hidden="true" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round">{utilityIconPaths[name]}</svg>;
}

function groupAllocations(allocations = []) {
  return allocations.reduce((groups, allocation) => {
    groups[allocation.demandCentreId] ||= [];
    groups[allocation.demandCentreId].push(allocation);
    return groups;
  }, {});
}

export default function Dashboard() {
  const { data, error, busy, events, critical, load, apply, run, reset, loadCritical, refreshEvents, clearError, setError } = useSimulation();
  const [darkMode, setDarkMode] = useState(() => document.documentElement.classList.contains('dark'));
  const [type, setType] = useState('NODE_OUTAGE');
  const [target, setTarget] = useState('');
  const [severity, setSeverity] = useState(0.5);
  const [tab, setTab] = useState('Summary');
  const [selection, setSelection] = useState(null);
  const [highlight, setHighlight] = useState(null);
  const [routeFrom, setRouteFrom] = useState('S1');
  const [routeTo, setRouteTo] = useState('D1');
  const [metric, setMetric] = useState('cost');
  const [route, setRoute] = useState(null);
  const [pending, setPending] = useState(false);
  const [controlsOpen, setControlsOpen] = useState(true);
  const [telemetryOpen, setTelemetryOpen] = useState(true);
  const [layoutVersion, setLayoutVersion] = useState(0);
  const mock = import.meta.env.VITE_USE_MOCKS !== 'false';

  useEffect(() => {
    const theme = darkMode ? 'dark' : 'light';
    document.documentElement.classList.toggle('dark', darkMode);
    document.documentElement.dataset.theme = theme;
    document.querySelector('meta[name="theme-color"]')?.setAttribute('content', darkMode ? '#221d27' : '#faf5fa');
    try {
      window.localStorage.setItem('gridflow-theme', theme);
    } catch (storageError) {
      console.warn('Could not save the theme preference.', storageError);
    }
  }, [darkMode]);

  useEffect(() => {
    const compactLayout = window.matchMedia('(max-width: 850px)');
    const closeTelemetryWhenCompact = () => {
      if (compactLayout.matches) setTelemetryOpen(false);
    };
    closeTelemetryWhenCompact();
    compactLayout.addEventListener('change', closeTelemetryWhenCompact);
    return () => compactLayout.removeEventListener('change', closeTelemetryWhenCompact);
  }, []);

  const targets = useMemo(() => !data ? [] : type === 'EDGE_DISRUPTION'
    ? data.network.edges
    : data.network.nodes.filter((node) => type === 'NODE_OUTAGE'
      || (type === 'SUPPLY_REDUCTION' && ['SOURCE', 'IMPORT_TERMINAL'].includes(node.type))
      || (type === 'DEMAND_SURGE' && node.type === 'DEMAND_CENTRE')), [data, type]);

  const selectGraph = useCallback((item) => {
    setSelection(item);
    if (item?.from) {
      setType('EDGE_DISRUPTION');
      setTarget(item.id);
    } else if (item?.id) {
      setTarget(item.id);
      setType(item.type === 'DEMAND_CENTRE' ? 'DEMAND_SURGE' : item.type === 'SOURCE' || item.type === 'IMPORT_TERMINAL' ? 'SUPPLY_REDUCTION' : 'NODE_OUTAGE');
    }
  }, []);

  const doApply = async (event) => {
    event.preventDefault();
    try {
      const result = await apply({ type, targetId: target, severity: type === 'NODE_OUTAGE' ? 1 : severity });
      setHighlight({ nodes: result.impact.exposedNodes, edges: result.impact.exposedEdges });
    } catch {
      // The simulation context displays the request error.
    }
  };

  const doRoute = async (event) => {
    event.preventDefault();
    setPending(true);
    try {
      const result = await api.getRoute(routeFrom, routeTo, metric);
      setRoute(result);
      setHighlight(result.found ? { nodes: result.path, edges: result.edgeIds } : null);
    } catch (requestError) {
      setError(requestError);
    } finally {
      setPending(false);
    }
  };

  const doReset = async () => {
    if (!window.confirm('Reset the simulation to baseline?')) return;
    try {
      await reset();
      setHighlight(null);
      setRoute(null);
    } catch {
      // The simulation context displays the request error.
    }
  };

  const selectTab = (nextTab) => {
    setTab(nextTab);
    if (nextTab === 'Critical' && !critical) loadCritical();
  };

  const toggleDrawer = (drawer) => {
    const compactLayout = window.matchMedia('(max-width: 850px)').matches;
    if (drawer === 'controls') {
      const nextOpen = !controlsOpen;
      setControlsOpen(nextOpen);
      if (nextOpen && compactLayout) setTelemetryOpen(false);
    } else {
      const nextOpen = !telemetryOpen;
      setTelemetryOpen(nextOpen);
      if (nextOpen && compactLayout) setControlsOpen(false);
    }
    setLayoutVersion((version) => version + 1);
  };

  const handleTabKeyDown = (event, index) => {
    const next = { ArrowRight: index + 1, ArrowLeft: index - 1, Home: 0, End: tabs.length - 1 }[event.key];
    if (next === undefined) return;
    event.preventDefault();
    const nextIndex = (next + tabs.length) % tabs.length;
    event.currentTarget.parentElement.querySelectorAll('[role="tab"]')[nextIndex]?.focus();
    selectTab(tabs[nextIndex].id);
  };

  if (error && !data) {
    return <main className="load-error"><h1>GridFlow could not connect</h1><p>{error.message}</p><button type="button" onClick={load}>Retry</button></main>;
  }
  if (!data) return <main className="load-error"><span className="connection-indicator" aria-hidden="true"/>Loading network…</main>;

  const summary = data.state.summary;
  const shortages = data.impact?.shortages || [];
  const recoveryGroups = groupAllocations(data.recovery?.allocations);
  const choosePath = (allocation) => setHighlight({ nodes: allocation.path, edges: allocation.edgeIds });
  const demandCentres = data.network.nodes.filter((node) => node.type === 'DEMAND_CENTRE');

  const renderTabContent = () => {
    if (tab === 'Summary') {
      const metrics = [
        ['Supply', summary.totalSupply.toFixed(0), 'EU/day'],
        ['Demand', summary.totalDemand.toFixed(0), 'EU/day'],
        ['Served', summary.totalServed.toFixed(0), 'EU/day'],
        ['Unmet', summary.unmetDemand.toFixed(0), 'EU/day'],
        ['Service level', `${summary.servedPercent.toFixed(1)}%`, `baseline ${summary.baselineServedPercent.toFixed(1)}%`],
      ];
      return <>
        <section className="insight-section">
          <div className="section-heading"><div><p className="eyebrow">CURRENT STATE</p><h3>Network summary</h3></div><span className="data-stamp">VERSION {data.state.version}</span></div>
          <div className="kpis">{metrics.map(([label, value, unit]) => <article className="kpi" key={label}><span>{label}</span><strong>{value}</strong><small>{unit}</small></article>)}</div>
          <div className="service-track" aria-label={`Current service ${summary.servedPercent.toFixed(1)} percent`}><span style={{ width: `${Math.min(100, summary.servedPercent)}%` }}/></div>
          <p className="track-caption">BASELINE <b>{summary.baselineServedPercent.toFixed(1)}%</b><span>CURRENT <b>{summary.servedPercent.toFixed(1)}%</b></span></p>
        </section>
        <section className="insight-section">
          <div className="section-heading"><div><p className="eyebrow">DEMAND</p><h3>Demand centre telemetry</h3></div><span className="data-stamp">{demandCentres.length} CENTRES</span></div>
          <div className="table-scroll"><table className="data-table"><thead><tr><th>Centre</th><th>Status</th><th>Demand</th><th>Served</th><th>Shortage</th><th>Prio</th></tr></thead><tbody>
            {demandCentres.map((node) => {
              const state = data.state.nodes.find((entry) => entry.id === node.id);
              return <tr key={node.id}><td><button className="table-link" type="button" onClick={() => setHighlight({ nodes: [node.id], edges: [] })}>{node.id}<small>{node.name}</small></button></td><td>{state.status}</td><td>{state.effectiveDemand.toFixed(0)}</td><td>{state.served.toFixed(0)}</td><td className={state.shortage > 0 ? 'value-alert' : ''}>{state.shortage.toFixed(0)}</td><td>P{node.priority}</td></tr>;
            })}
          </tbody></table></div>
        </section>
      </>;
    }

    if (tab === 'Shortages') {
      return <section className="insight-section">
        <div className="section-heading"><div><p className="eyebrow">SERVICE IMPACT</p><h3>Unmet demand</h3></div><span className="data-stamp">{shortages.length} AFFECTED</span></div>
        {shortages.length ? <div className="table-scroll"><table className="data-table"><thead><tr><th>Centre</th><th>Priority</th><th>Demand</th><th>Served</th><th>Shortage</th><th>Share</th></tr></thead><tbody>
          {shortages.map((shortage) => <tr key={shortage.nodeId}><td><button className="table-link" type="button" onClick={() => setHighlight({ nodes: [shortage.nodeId], edges: [] })}>{shortage.nodeId}<small>{shortage.name}</small></button></td><td>P{shortage.priority}</td><td>{shortage.demand.toFixed(0)}</td><td>{shortage.served.toFixed(0)}</td><td className="value-alert">{shortage.shortage.toFixed(0)}</td><td><div className="mini-track"><i className="danger" style={{ width: `${shortage.demand ? shortage.shortage / shortage.demand * 100 : 0}%` }}/></div></td></tr>)}
        </tbody></table></div> : <p className="empty">No shortages. Current supply meets demand.</p>}
      </section>;
    }

    if (tab === 'Recovery') {
      return <>
        <section className="insight-section">
          <div className="section-heading"><div><p className="eyebrow">FLOW PLAN</p><h3>Recommended allocations</h3></div><span className="data-stamp">{data.recovery?.allocations?.length || 0} ROUTES</span></div>
          {Object.keys(recoveryGroups).length ? <div className="allocation-table">{Object.entries(recoveryGroups).map(([centre, allocations]) => <div className="allocation-group" key={centre}><h4>{centre}</h4>{allocations.map((allocation, index) => <button className="allocation-row" type="button" key={`${centre}-${index}`} onClick={() => choosePath(allocation)}><span className="path-value">{allocation.path.join(' → ')}</span><span>{allocation.flow.toFixed(0)} EU/day</span><span>Cost {allocation.totalCost}</span><span>{allocation.totalTime} h</span><span className="row-action">Show route</span></button>)}</div>)}</div> : <p className="empty">No recovery plan yet. Apply a disruption to generate allocations.</p>}
        </section>
        {data.recovery?.unresolved?.length > 0 && <section className="insight-section"><div className="section-heading"><div><p className="eyebrow">UNRESOLVED</p><h3>Remaining shortages</h3></div></div><div className="table-scroll"><table className="data-table"><thead><tr><th>Centre</th><th>Shortage</th><th>Reason</th></tr></thead><tbody>{data.recovery.unresolved.map((item) => <tr key={item.nodeId}><td>{item.nodeId}</td><td className="value-alert">{item.shortage.toFixed(0)} EU/day</td><td>{item.reason}</td></tr>)}</tbody></table></div></section>}
        {data.recovery?.recommendations?.length > 0 && <section className="insight-section"><div className="section-heading"><div><p className="eyebrow">ACTIONS</p><h3>Recommendations</h3></div></div><div className="table-scroll"><table className="data-table"><thead><tr><th>Type</th><th>Target</th><th>Priority</th><th>Recommendation</th></tr></thead><tbody>{data.recovery.recommendations.map((item, index) => <tr key={`${item.type}-${item.targetId || index}`}><td>{item.type}</td><td>{item.targetId || '—'}</td><td>{item.priority ?? '—'}</td><td className="wrap-cell">{item.message}</td></tr>)}</tbody></table></div></section>}
      </>;
    }

    if (tab === 'Critical') {
      return <section className="insight-section">
        <div className="section-heading"><div><p className="eyebrow">RESILIENCE ANALYSIS</p><h3>Critical nodes</h3></div><button className="button-compact" type="button" onClick={loadCritical} disabled={busy === 'critical'}>{busy === 'critical' ? 'Analyzing…' : critical ? 'Refresh analysis' : 'Run analysis'}</button></div>
        <p className="section-note">Structural weak points are articulation points in the active, undirected network projection.</p>
        {critical ? <>
          <div className="critical-points"><span>ARTICULATION POINTS</span>{critical.articulationPoints.length ? critical.articulationPoints.map((id) => <button className="inline-action" type="button" key={id} onClick={() => setHighlight({ nodes: [id], edges: [] })}>{id}</button>) : <b>None detected</b>}</div>
          <div className="section-heading subheading"><div><p className="eyebrow">FLOW-BASED RANKING</p><h3>Impact if removed</h3></div></div>
          <div className="table-scroll"><table className="data-table"><thead><tr><th>Rank</th><th>Node</th><th>Additional unmet demand</th><th>Increase</th></tr></thead><tbody>
            {critical.ranking.slice(0, 8).map((item, index) => <tr key={item.nodeId}><td>{String(index + 1).padStart(2, '0')}</td><td><button className="table-link" type="button" onClick={() => setHighlight({ nodes: [item.nodeId], edges: [] })}>{item.nodeId}<small>{item.name}</small></button></td><td>{item.unmetDemandIfRemoved.toFixed(0)} EU/day</td><td>{item.increasePercent?.toFixed(1) ?? '—'}%</td></tr>)}
          </tbody></table></div>
        </> : <p className="empty">Run an analysis to inspect network criticality.</p>}
      </section>;
    }

    if (tab === 'Routes') {
      return <section className="insight-section">
        <div className="section-heading"><div><p className="eyebrow">PATH ANALYSIS</p><h3>Route finder</h3></div></div>
        <form className="route-form" onSubmit={doRoute}>
          <label>From<select value={routeFrom} onChange={(event) => setRouteFrom(event.target.value)}>{data.network.nodes.filter((node) => ['SOURCE', 'IMPORT_TERMINAL'].includes(node.type)).map((node) => <option key={node.id} value={node.id}>{node.id} · {node.name}</option>)}</select></label>
          <label>To<select value={routeTo} onChange={(event) => setRouteTo(event.target.value)}>{demandCentres.map((node) => <option key={node.id} value={node.id}>{node.id} · {node.name}</option>)}</select></label>
          <label>Optimize by<select value={metric} onChange={(event) => setMetric(event.target.value)}><option value="cost">Total cost</option><option value="time">Transit time</option></select></label>
          <button className="action-button" type="submit" disabled={pending}>{pending ? 'Finding route…' : 'Find route'}</button>
        </form>
        {route && (route.found ? <div className="route-result"><p className="eyebrow">SELECTED PATH</p><strong className="path-value">{route.path.join(' → ')}</strong><div className="route-metrics"><span>COST <b>{route.totalCost}</b></span><span>TIME <b>{route.totalTime} h</b></span><span>CAPACITY <b>{route.bottleneckCapacity} EU/day</b></span></div></div> : <p className="empty">No available route between these locations.</p>)}
      </section>;
    }

    return <section className="insight-section">
      <div className="section-heading"><div><p className="eyebrow">SIMULATION HISTORY</p><h3>Event log</h3></div><button className="button-compact" type="button" onClick={async () => { try { await refreshEvents(); setError(null); } catch (requestError) { setError(requestError); } }}>Refresh log</button></div>
      {events.length ? <div className="table-scroll"><table className="data-table"><thead><tr><th>Sequence</th><th>Event type</th><th>Details</th></tr></thead><tbody>{[...events].reverse().map((event) => <tr key={event.seq}><td>#{String(event.seq).padStart(3, '0')}</td><td>{event.kind.replaceAll('_', ' ')}</td><td className="wrap-cell">{event.message}</td></tr>)}</tbody></table></div> : <p className="empty">No simulation events recorded.</p>}
    </section>;
  };

  return <div className="app-shell">
    <header className="topbar">
      <div className="brand"><strong>GridFlow</strong><span className="brand-divider">//</span><span className="brand-subtitle">Simulation Engine</span><span className={`live-tag ${error?.code === 'BACKEND_UNREACHABLE' ? 'offline' : ''}`}><i/> {error?.code === 'BACKEND_UNREACHABLE' ? 'API OFFLINE' : 'LIVE'}</span></div>
      <nav className="header-drawer-actions" aria-label="Workspace drawers">
        <button className="header-drawer-toggle" type="button" aria-expanded={controlsOpen} onClick={() => toggleDrawer('controls')}><UtilityIcon name="controls"/>Controls</button>
        <button className="header-drawer-toggle" type="button" aria-expanded={telemetryOpen} onClick={() => toggleDrawer('telemetry')}><UtilityIcon name="telemetry"/>Telemetry</button>
      </nav>
      <div className="top-actions">
        <span className="system-mode"><span>STATE</span><b>v{data.state.version}</b><span className="mode-badge">{mock ? 'IN-MEMORY MOCK' : error?.code === 'BACKEND_UNREACHABLE' ? 'API OFFLINE' : 'GRIDFLOW API'}</span></span>
        <button className="utility-button reset" type="button" onClick={doReset} disabled={busy === 'reset'}><UtilityIcon name="reset"/>{busy === 'reset' ? 'Resetting…' : 'Reset State'}</button>
        <button className="utility-button theme-toggle" type="button" onClick={() => setDarkMode((isDark) => !isDark)} aria-label={`Switch to ${darkMode ? 'light' : 'dark'} mode`} aria-pressed={darkMode}><UtilityIcon name={darkMode ? 'sun' : 'moon'}/><span>{darkMode ? 'Light' : 'Dark'}</span></button>
      </div>
    </header>
    {error && <div className="toast" role="alert"><span><b>{error.message}</b><small>{error.code}</small></span><button type="button" onClick={clearError} aria-label="Dismiss error">×</button></div>}
    <main className={`workspace ${controlsOpen ? '' : 'controls-collapsed'} ${telemetryOpen ? '' : 'telemetry-collapsed'}`} onTransitionEnd={(event) => {
      if (event.target.classList.contains('drawer') && event.propertyName === 'flex-basis') setLayoutVersion((version) => version + 1);
    }}>
      <aside className={`controls drawer panel ${controlsOpen ? 'drawer-open' : 'drawer-closed'}`} aria-label="Simulation controls">
        <button className="drawer-toggle drawer-toggle-left" type="button" aria-expanded={controlsOpen} aria-label={`${controlsOpen ? 'Collapse' : 'Expand'} controls drawer`} onClick={() => toggleDrawer('controls')}><UtilityIcon name={controlsOpen ? 'chevronLeft' : 'controls'}/><span>Controls</span></button>
        <div className="drawer-content" inert={controlsOpen ? undefined : ''}>
        <div className="panel-heading"><p className="eyebrow">OPERATIONS</p><h2>Simulation controls</h2></div>
        <section className="control-section">
          <div className="section-heading"><div><p className="eyebrow">MANUAL INPUT</p><h3>Apply disruption</h3></div></div>
          <form onSubmit={doApply}>
            <label>Disruption type<select value={type} onChange={(event) => { setType(event.target.value); setTarget(''); }}><option value="NODE_OUTAGE">Node outage</option><option value="EDGE_DISRUPTION">Edge disruption</option><option value="SUPPLY_REDUCTION">Supply reduction</option><option value="DEMAND_SURGE">Demand surge</option></select></label>
            <label>Target asset<select required value={target} onChange={(event) => setTarget(event.target.value)}><option value="">Select target</option>{targets.map((item) => <option key={item.id} value={item.id}>{item.id} · {item.name || `${item.from} → ${item.to}`}</option>)}</select></label>
            {type !== 'NODE_OUTAGE' && <label className="severity-label"><span>Severity <b>{Math.round(severity * 100)}%</b></span><input type="range" min="0.05" max="1" step="0.05" value={severity} onChange={(event) => setSeverity(Number(event.target.value))}/></label>}
            <button className="action-button" type="submit" disabled={Boolean(busy) || !target}>{busy === 'apply' ? 'Applying disruption…' : 'Apply disruption'}</button>
          </form>
        </section>
        <section className="control-section">
          <div className="section-heading"><div><p className="eyebrow">PRESET SCENARIOS</p><h3>Scenario runner</h3></div><span className="data-stamp">{data.scenarios.length} PRESETS</span></div>
          <p className="section-note">Scenarios apply to current state. Reset first for a clean run.</p>
          <div className="scenario-list">{data.scenarios.map((scenario) => <article className="scenario" key={scenario.id}><div className="scenario-info"><span className="scenario-id">{scenario.id}</span><b>{scenario.name}</b><p>{scenario.description}</p><small>{scenario.disruptions.map((disruption) => `${disruption.type.replaceAll('_', ' ')} ${disruption.targetId}`).join(' · ')}</small></div><button className="scenario-run" type="button" onClick={async () => { try { const result = await run(scenario.id); setHighlight({ nodes: result.impact.exposedNodes, edges: result.impact.exposedEdges }); } catch { /* The simulation context displays the request error. */ } }} disabled={Boolean(busy)}>{busy === 'scenario' ? '…' : 'Run'}</button></article>)}</div>
        </section>
        <section className="control-section selection">
          <div className="section-heading"><div><p className="eyebrow">GRAPH INSPECTOR</p><h3>Selected asset</h3></div></div>
          {selection ? <><div className="selection-title"><b>{selection.id}</b><span>{selection.type || 'NETWORK EDGE'}</span></div><p className="selection-name">{selection.name || `${selection.from} → ${selection.to}`}</p><div className="selection-grid">{selection.from ? <><span>FLOW <b>{selection.flow ?? 0}</b></span><span>CAPACITY <b>{selection.effectiveCapacity ?? selection.capacity}</b></span><span>COST <b>{selection.cost}</b></span><span>TIME <b>{selection.transitTime} h</b></span></> : <><span>CAPACITY <b>{selection.capacity}</b></span><span>SUPPLY <b>{selection.effectiveSupply ?? selection.supply}</b></span><span>SERVED <b>{selection.served ?? 0} / {selection.effectiveDemand ?? selection.demand}</b></span><span>THROUGHPUT <b>{selection.throughput ?? 0}</b></span></>}</div></> : <p className="empty compact-empty">Select a network node or link to inspect its current state.</p>}
        </section>
        </div>
      </aside>
      <section className="graph-panel" aria-label="Network topology canvas">
        <div className="panel-heading graph-heading"><div><p className="eyebrow">GEOGRAPHIC NETWORK</p><h2>Energy supply topology</h2></div><div className="graph-meta"><span>{data.network.nodes.length} NODES</span><span>{data.network.edges.length} LINKS</span><button className="button-compact" type="button" onClick={() => setHighlight(null)} disabled={!highlight}>Clear highlight</button></div></div>
        <NetworkGraph data={{ ...data, network: data.network }} impact={data.impact} highlight={highlight} selection={selection} theme={darkMode ? 'dark' : 'light'} layoutVersion={layoutVersion} onSelect={selectGraph}/>
        <div className="legend" aria-label="Network legend">{Object.entries(kindLabels).map(([typeName, label]) => <span key={typeName}><i className={`shape ${typeName.toLowerCase()}`}/>{label}</span>)}<span><i className="status-dot active"/>Active</span><span><i className="status-dot degraded"/>Degraded</span><span><i className="status-dot failed"/>Failed</span><span><i className="status-outline"/>Exposed</span><span><i className="status-outline cyan"/>Selected</span></div>
      </section>
      <aside className={`insights drawer panel ${telemetryOpen ? 'drawer-open' : 'drawer-closed'}`} aria-label="Telemetry and analytics">
        <button className="drawer-toggle drawer-toggle-right" type="button" aria-expanded={telemetryOpen} aria-label={`${telemetryOpen ? 'Collapse' : 'Expand'} telemetry drawer`} onClick={() => toggleDrawer('telemetry')}><UtilityIcon name={telemetryOpen ? 'chevronRight' : 'telemetry'}/><span>Telemetry</span></button>
        <div className="drawer-content telemetry-drawer-content" inert={telemetryOpen ? undefined : ''}>
        <nav className="tabs" role="tablist" aria-label="Telemetry views">{tabs.map((item, index) => <button id={`tab-${item.id}`} role="tab" aria-selected={tab === item.id} aria-controls="telemetry-panel" tabIndex={tab === item.id ? 0 : -1} className={tab === item.id ? 'active' : ''} key={item.id} onClick={() => selectTab(item.id)} onKeyDown={(event) => handleTabKeyDown(event, index)}><NavIcon name={item.icon}/><span>{item.label}</span></button>)}</nav>
        <div className="tab-content" id="telemetry-panel" role="tabpanel" aria-labelledby={`tab-${tab}`} tabIndex={0}>
          <div className="telemetry-content">{renderTabContent()}</div>
        </div>
        <footer className="panel-footer"><span>{mock ? 'IN-MEMORY MOCK' : 'GRIDFLOW API'}</span><span>STATE v{data.state.version}</span></footer>
        </div>
      </aside>
    </main>
  </div>;
}

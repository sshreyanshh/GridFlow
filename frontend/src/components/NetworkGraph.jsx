import { useEffect, useRef, useState } from 'react';
import cytoscape from 'cytoscape';

const typeShape = { SOURCE: 'triangle', IMPORT_TERMINAL: 'diamond', PROCESSING: 'round-rectangle', DISTRIBUTION_HUB: 'hexagon', POWER_PLANT: 'star', DEMAND_CENTRE: 'ellipse' };
const mapLabels = { S1: 'Mumbai High', S2: 'KG-D6', S3: 'Jharia', T1: 'Dahej LNG', T2: 'Kochi LNG', T3: 'Ennore LNG', P1: 'Jamnagar', P2: 'Panipat', P3: 'Mathura', P4: 'Paradip', H1: 'Jaipur', H2: 'Nagpur', H3: 'Hyderabad', H4: 'Patna', G1: 'Vindhyachal', G2: 'Ramagundam', G3: 'Simhadri', D1: 'Mumbai', D2: 'Delhi NCR', D3: 'Bengaluru', D4: 'Kolkata', D5: 'Chennai' };
export default function NetworkGraph({ data, onSelect, impact, highlight }) {
  const container = useRef(null); const cyRef = useRef(null); const didFit = useRef(false); const [hover, setHover] = useState(null);
  useEffect(() => {
    if (!container.current) return undefined;
    const cy = cytoscape({ container: container.current, elements: [], layout: { name: 'preset', fit: true, padding: 30 }, userZoomingEnabled: false, userPanningEnabled: false, autoungrabify: true, boxSelectionEnabled: false, style: [
      { selector: 'node', style: { label: 'data(label)', 'text-wrap': 'wrap', 'text-max-width': 92, 'font-size': 14, color: '#f0f6fa', 'text-valign': 'bottom', 'text-margin-y': 8, 'text-outline-color': '#101a24', 'text-outline-width': 1, 'background-color': '#36c98f', width: 40, height: 40, 'border-width': 1.5, 'border-color': '#e0f5ef', 'transition-property': 'background-color, border-color, border-width, opacity', 'transition-duration': 220 } },
      ...Object.entries(typeShape).map(([type, shape]) => ({ selector: `node[type = "${type}"]`, style: { shape } })),
      { selector: 'node[status = "DEGRADED"]', style: { 'background-color': '#e7a53c' } }, { selector: 'node[status = "FAILED"]', style: { 'background-color': '#e44e5b', label: 'data(failedLabel)', 'border-color': '#ff7982', 'border-width': 2 } },
      { selector: 'node[shortage > 0]', style: { 'border-color': '#ff505e', 'border-width': 4 } }, { selector: 'node.exposed', style: { 'border-color': '#f59a48', 'border-width': 4 } }, { selector: '.dim', style: { opacity: 0.16 } }, { selector: '.highlighted', style: { 'border-color': '#45d9ed', 'border-width': 4, opacity: 1 } },
      { selector: 'edge', style: { width: 'mapData(flow, 0, 600, 0.7, 2.4)', 'line-color': '#68a48f', 'target-arrow-color': '#68a48f', 'target-arrow-shape': 'triangle', 'curve-style': 'bezier', 'arrow-scale': 0.55, opacity: 0.48, 'transition-property': 'line-color, width, opacity', 'transition-duration': 220 } },
      { selector: 'edge[status = "REDUCED"]', style: { 'line-color': '#e7a53c', 'target-arrow-color': '#e7a53c', opacity: 0.9 } }, { selector: 'edge[status = "FAILED"]', style: { 'line-color': '#e44e5b', 'target-arrow-color': '#e44e5b', 'line-style': 'dashed', opacity: 0.9 } }, { selector: 'edge.exposed', style: { 'line-color': '#f59a48', 'target-arrow-color': '#f59a48', width: 3, opacity: 1 } }, { selector: 'edge.highlighted', style: { 'line-color': '#45d9ed', 'target-arrow-color': '#45d9ed', width: 3.5, opacity: 1 } },
    ], minZoom: 0.25, maxZoom: 3 });
    cyRef.current = cy;
    cy.on('tap', 'node, edge', (event) => onSelect(event.target.data())); cy.on('mouseover', 'node, edge', (event) => setHover(event.target.data())); cy.on('mouseout', 'node, edge', () => setHover(null));
    const observer = new ResizeObserver(() => { cy.resize(); const width = container.current.clientWidth; const height = container.current.clientHeight; const zoom = Math.min((width - 18) / 794, (height - 18) / 1123); cy.viewport({ zoom, pan: { x: (width - 794 * zoom) / 2, y: (height - 1123 * zoom) / 2 } }); }); observer.observe(container.current);
    return () => { observer.disconnect(); cy.destroy(); cyRef.current = null; };
  }, [onSelect]);
  useEffect(() => {
    const cy = cyRef.current; if (!cy || !data) return;
    const nodes = data.network.nodes.map((n) => { const state = data.state.nodes.find((s) => s.id === n.id); return { data: { ...n, ...state, id: n.id, label: mapLabels[n.id] || n.name, failedLabel: `${mapLabels[n.id] || n.name} ×`, type: n.type }, position: n.position, classes: impact?.exposedNodes?.includes(n.id) ? 'exposed' : '' }; });
    const edges = data.network.edges.map((e) => { const state = data.state.edges.find((s) => s.id === e.id); return { data: { ...e, ...state, id: e.id, source: e.from, target: e.to, label: e.id }, classes: impact?.exposedEdges?.includes(e.id) ? 'exposed' : '' }; });
    cy.batch(() => { cy.elements().remove(); cy.add([...nodes, ...edges]); }); if (!didFit.current) { didFit.current = true; const width = container.current.clientWidth; const height = container.current.clientHeight; const zoom = Math.min((width - 18) / 794, (height - 18) / 1123); cy.viewport({ zoom, pan: { x: (width - 794 * zoom) / 2, y: (height - 1123 * zoom) / 2 } }); }
    cy.nodes().unselectify(); cy.edges().unselectify();
  }, [data, impact]);
  useEffect(() => { const cy = cyRef.current; if (!cy) return; cy.elements().removeClass('dim highlighted'); if (!highlight) return; cy.elements().addClass('dim'); cy.nodes(highlight.nodes?.map((id) => `#${id}`).join(',') || 'node').filter((e) => highlight.nodes?.includes(e.id())).removeClass('dim').addClass('highlighted'); cy.edges(highlight.edges?.map((id) => `#${id}`).join(',') || 'edge').filter((e) => highlight.edges?.includes(e.id())).removeClass('dim').addClass('highlighted'); }, [highlight]);
  return <div className="graph-wrap">
    <img className="india-map" src="/india-states-survey-of-india.svg" alt="Survey of India state map" />
    <div className="graph" ref={container} aria-label="Interactive directed energy network" />
    <div className="map-note">Survey of India map · illustrative network links</div>
    {hover && <div className="tooltip">{hover.id} · {hover.name || `${hover.from} → ${hover.to}`}<br/>{hover.status} · {hover.flow ?? hover.served ?? 0} / {hover.effectiveCapacity ?? hover.effectiveDemand ?? hover.capacity ?? 0} EU/day</div>}
  </div>;
}

import { useEffect, useRef, useState } from 'react';
import cytoscape from 'cytoscape';

const typeShape = { SOURCE: 'triangle', IMPORT_TERMINAL: 'diamond', PROCESSING: 'round-rectangle', DISTRIBUTION_HUB: 'hexagon', POWER_PLANT: 'star', DEMAND_CENTRE: 'ellipse' };
const mapLabels = { S1: 'Mumbai High', S2: 'KG-D6', S3: 'Jharia', T1: 'Dahej LNG', T2: 'Kochi LNG', T3: 'Ennore LNG', P1: 'Jamnagar', P2: 'Panipat', P3: 'Mathura', P4: 'Paradip', H1: 'Jaipur', H2: 'Nagpur', H3: 'Hyderabad', H4: 'Patna', G1: 'Vindhyachal', G2: 'Ramagundam', G3: 'Simhadri', D1: 'Mumbai', D2: 'Delhi NCR', D3: 'Bengaluru', D4: 'Kolkata', D5: 'Chennai' };
const typeLabels = { SOURCE: 'Source', IMPORT_TERMINAL: 'Import terminal', PROCESSING: 'Processing', DISTRIBUTION_HUB: 'Distribution hub', POWER_PLANT: 'Power plant', DEMAND_CENTRE: 'Demand centre' };
const mapCanvas = { width: 793.76, height: 1122.56 };
const mapBounds = { minX: 0, maxX: 793.76, minY: 0, maxY: 1122.56 };
const zoomThreshold = 1.2;

function projectMapPosition(position) {
  // The Survey of India SVG already uses a conic projection; fixture positions use its native pixel frame.
  return {
    x: ((position.x - mapBounds.minX) / (mapBounds.maxX - mapBounds.minX)) * mapCanvas.width,
    y: ((position.y - mapBounds.minY) / (mapBounds.maxY - mapBounds.minY)) * mapCanvas.height,
  };
}

function getMapViewport(container) {
  const { width, height } = container.getBoundingClientRect();
  const zoom = Math.min(width / mapCanvas.width, height / mapCanvas.height);
  return {
    zoom,
    pan: {
      x: (width - mapCanvas.width * zoom) / 2,
      y: (height - mapCanvas.height * zoom) / 2,
    },
  };
}

function syncMapToViewport(cy, container, mapElement) {
  if (!container || !mapElement) return;
  const base = getMapViewport(container);
  const scale = cy.zoom() / base.zoom;
  const pan = cy.pan();
  mapElement.style.transform = `translate(${pan.x - base.pan.x * scale}px, ${pan.y - base.pan.y * scale}px) scale(${scale})`;
}

function alignGraphToMap(cy, container, mapElement) {
  const viewport = getMapViewport(container);
  cy.viewport({
    zoom: viewport.zoom,
    pan: viewport.pan,
  });
  syncMapToViewport(cy, container, mapElement);
}

function applyInspectionFocus(cy, id) {
  cy.batch(() => {
    cy.elements().removeClass('inspected inspection-dim inspection-edge');
    if (!id) return;
    const node = cy.getElementById(id);
    if (!node.length || !node.isNode()) return;
    cy.elements().addClass('inspection-dim');
    node.removeClass('inspection-dim').addClass('inspected');
    const neighborhood = node.neighborhood();
    neighborhood.nodes().removeClass('inspection-dim');
    neighborhood.edges().removeClass('inspection-dim').addClass('inspection-edge');
  });
}

function createGraphStyle() {
  const styles = window.getComputedStyle(document.documentElement);
  const color = (token, fallback) => styles.getPropertyValue(token).trim() || fallback;
  const duration = window.matchMedia('(prefers-reduced-motion: reduce)').matches ? 0 : 220;
  const colors = {
    active: color('--chart-1', '#d926a2'),
    degraded: color('--chart-4', '#e8c468'),
    failed: color('--destructive', '#ab4347'),
    exposed: color('--chart-5', '#f4a462'),
    highlight: color('--chart-2', '#6c12b9'),
    edge: color('--chart-3', '#274754'),
    text: color('--foreground', '#501854'),
    outline: color('--card', '#faf5fa'),
    failedOutline: color('--destructive-foreground', '#ffffff'),
  };
  return [
    { selector: 'node', style: { label: 'data(label)', 'font-family': 'JetBrains Mono, monospace', 'text-wrap': 'wrap', 'text-max-width': 104, 'font-size': 10, 'text-opacity': 0, color: colors.text, 'text-valign': 'bottom', 'text-margin-y': 7, 'text-background-color': colors.outline, 'text-background-opacity': 0.85, 'text-background-padding': 2, 'text-outline-color': colors.outline, 'text-outline-width': 1, 'background-color': colors.active, width: 12, height: 12, 'border-width': 1, 'border-color': colors.outline, 'transition-property': 'background-color, border-color, border-width, width, height, opacity, text-opacity, font-size', 'transition-duration': duration } },
    { selector: 'edge', style: { width: 1, 'line-color': colors.edge, 'target-arrow-color': colors.edge, 'target-arrow-shape': 'triangle', 'curve-style': 'bezier', 'arrow-scale': 0.45, opacity: 0.3, 'transition-property': 'line-color, width, opacity', 'transition-duration': duration } },
    { selector: 'node#S1', style: { 'text-halign': 'left', 'text-valign': 'center', 'text-margin-x': -7, 'text-margin-y': 0 } },
    { selector: 'node#D1', style: { 'text-halign': 'right', 'text-valign': 'center', 'text-margin-x': 7, 'text-margin-y': 0 } },
    { selector: 'node#P2', style: { 'text-valign': 'top', 'text-margin-y': -7 } },
    { selector: 'node#P3', style: { 'text-halign': 'right', 'text-valign': 'center', 'text-margin-x': 7, 'text-margin-y': 0 } },
    { selector: 'node#H1', style: { 'text-halign': 'left', 'text-valign': 'center', 'text-margin-x': -7, 'text-margin-y': 0 } },
    { selector: 'node#T3', style: { 'text-valign': 'top', 'text-margin-y': -7 } },
    { selector: 'node#D5', style: { 'text-valign': 'bottom', 'text-margin-y': 7 } },
    ...Object.entries(typeShape).map(([type, shape]) => ({ selector: `node[type = "${type}"]`, style: { shape } })),
    { selector: 'node[status = "DEGRADED"]', style: { 'background-color': colors.degraded } },
    { selector: 'node[status = "FAILED"]', style: { 'background-color': colors.failed, label: 'data(failedLabel)', 'border-color': colors.failedOutline, 'border-width': 2 } },
    { selector: 'node[shortage > 0]', style: { 'border-color': colors.failed, 'border-width': 4 } },
    { selector: 'node.exposed', style: { 'border-color': colors.exposed, 'border-width': 4 } },
    { selector: '.dim', style: { opacity: 0.16 } },
    { selector: '.highlighted', style: { 'border-color': colors.highlight, 'border-width': 4, opacity: 1 } },
    { selector: 'edge[status = "REDUCED"]', style: { 'line-color': colors.degraded, 'target-arrow-color': colors.degraded, opacity: 0.9 } },
    { selector: 'edge[status = "FAILED"]', style: { 'line-color': colors.failed, 'target-arrow-color': colors.failed, 'line-style': 'dashed', opacity: 0.9 } },
    { selector: 'edge.exposed', style: { 'line-color': colors.exposed, 'target-arrow-color': colors.exposed, width: 3, opacity: 1 } },
    { selector: 'edge.highlighted', style: { 'line-color': colors.highlight, 'target-arrow-color': colors.highlight, width: 3.5, opacity: 1 } },
    { selector: 'node#S1.inspected, node#D1.inspected, node#P2.inspected, node#P3.inspected, node#H1.inspected, node#D5.inspected', style: { 'text-halign': 'center', 'text-valign': 'top', 'text-margin-x': 0, 'text-margin-y': -10 } },
    { selector: 'node.exposed, node.highlighted, node[shortage > 0]', style: { 'border-width': 1.5 } },
    { selector: 'node.zoomed-in', style: { width: 25, height: 25, 'border-width': 1.5, 'text-opacity': 1 } },
    { selector: 'node.inspected', style: { label: 'data(inspectLabel)', width: 18, height: 18, 'border-width': 2, 'text-opacity': 1, 'font-size': 9, 'text-max-width': 150, 'text-halign': 'center', 'text-valign': 'top', 'text-margin-y': -10, 'text-background-opacity': 0.96 } },
    { selector: 'node.zoomed-in.inspected', style: { width: 37.5, height: 37.5 } },
    { selector: 'node.inspected[status]', style: { label: 'data(inspectLabel)' } },
    { selector: 'node.inspection-dim', style: { opacity: 0.14 } },
    { selector: 'edge.exposed, edge.highlighted', style: { width: 1.2, opacity: 0.72 } },
    { selector: 'edge.zoomed-in', style: { width: 1.4, opacity: 0.55 } },
    { selector: 'edge.zoomed-in.exposed', style: { width: 2, opacity: 0.9 } },
    { selector: 'edge.zoomed-in.highlighted', style: { width: 2.5, opacity: 0.95 } },
    { selector: 'edge.inspection-dim', style: { opacity: 0.08 } },
    { selector: 'edge.inspection-edge', style: { width: 2.5, 'line-color': colors.highlight, 'target-arrow-color': colors.highlight, opacity: 0.98 } },
  ];
}

export default function NetworkGraph({ data, onSelect, impact, highlight, selection, theme, layoutVersion }) {
  const container = useRef(null); const mapRef = useRef(null); const cyRef = useRef(null); const didFit = useRef(false); const hoveredId = useRef(null); const selectedId = useRef(null); const zoomedIn = useRef(false); const pointerStart = useRef(null); const [hover, setHover] = useState(null); const [zoomPercent, setZoomPercent] = useState(100); const [showZoomHint, setShowZoomHint] = useState(true);
  useEffect(() => {
    const graphContainer = container.current;
    if (!graphContainer) return undefined;
    const cy = cytoscape({ container: graphContainer, elements: [], layout: { name: 'preset', fit: false }, zoomingEnabled: true, panningEnabled: true, userZoomingEnabled: false, userPanningEnabled: true, autoungrabify: true, boxSelectionEnabled: false, style: createGraphStyle(), minZoom: 0.1, maxZoom: 3 });
    cyRef.current = cy;
    const updateZoomTier = () => {
      const nextZoomedIn = cy.zoom() >= zoomThreshold;
      if (nextZoomedIn === zoomedIn.current) return;
      zoomedIn.current = nextZoomedIn;
      cy.batch(() => {
        cy.nodes().toggleClass('zoomed-in', nextZoomedIn);
        cy.edges().toggleClass('zoomed-in', nextZoomedIn);
      });
    };
    const syncViewport = () => {
      updateZoomTier();
      syncMapToViewport(cy, container.current, mapRef.current);
      const baseZoom = getMapViewport(graphContainer).zoom;
      setZoomPercent(Math.round((cy.zoom() / baseZoom) * 100));
      if (cy.zoom() >= zoomThreshold) setShowZoomHint(false);
    };
    const refreshInspection = () => applyInspectionFocus(cy, hoveredId.current || selectedId.current);
    const inspectNode = (node) => {
      selectedId.current = node.id();
      refreshInspection();
      onSelect(node.data());
    };
    const handleWheel = (event) => {
      event.preventDefault();
      setShowZoomHint(false);
      const bounds = graphContainer.getBoundingClientRect();
      const delta = event.deltaY * (event.deltaMode === 1 ? 16 : event.deltaMode === 2 ? bounds.height : 1);
      const level = Math.min(cy.maxZoom(), Math.max(cy.minZoom(), cy.zoom() * (10 ** (-delta * 0.001))));
      cy.zoom({ level, renderedPosition: { x: event.clientX - bounds.left, y: event.clientY - bounds.top } });
      syncViewport();
    };
    const handlePointerDown = (event) => {
      pointerStart.current = { x: event.clientX, y: event.clientY };
    };
    const handlePointerMove = (event) => {
      if (!pointerStart.current || !(event.buttons || event.pressure)) return;
      if (Math.hypot(event.clientX - pointerStart.current.x, event.clientY - pointerStart.current.y) > 4) {
        setShowZoomHint(false);
        pointerStart.current = null;
      }
    };
    const clearPointerStart = () => { pointerStart.current = null; };
    graphContainer.addEventListener('wheel', handleWheel, { passive: false });
    graphContainer.addEventListener('pointerdown', handlePointerDown);
    graphContainer.addEventListener('pointermove', handlePointerMove);
    graphContainer.addEventListener('pointerup', clearPointerStart);
    graphContainer.addEventListener('pointercancel', clearPointerStart);
    cy.on('zoom pan', syncViewport);
    cy.on('tap', 'node', (event) => inspectNode(event.target));
    cy.on('tap', 'edge', (event) => {
      const position = event.renderedPosition || {
        x: event.position.x * cy.zoom() + cy.pan().x,
        y: event.position.y * cy.zoom() + cy.pan().y,
      };
      let nearestNode = null;
      let nearestDistance = Infinity;
      cy.nodes().forEach((node) => {
        const nodePosition = node.renderedPosition();
        const distance = Math.hypot(nodePosition.x - position.x, nodePosition.y - position.y);
        if (distance <= Math.max(12, node.renderedOuterWidth() / 2 + 4) && distance < nearestDistance) {
          nearestNode = node;
          nearestDistance = distance;
        }
      });
      if (nearestNode) inspectNode(nearestNode);
      else {
        selectedId.current = null;
        refreshInspection();
        onSelect(event.target.data());
      }
    });
    cy.on('tap', (event) => {
      if (event.target !== cy) return;
      selectedId.current = null;
      hoveredId.current = null;
      refreshInspection();
      setHover(null);
    });
    cy.on('mouseover', 'node', (event) => {
      hoveredId.current = event.target.id();
      refreshInspection();
      setHover(event.target.data());
    });
    cy.on('mouseout', 'node', (event) => {
      if (hoveredId.current !== event.target.id()) return;
      hoveredId.current = null;
      refreshInspection();
      setHover(null);
    });
    const observer = new ResizeObserver(() => {
      cy.resize();
      alignGraphToMap(cy, container.current, mapRef.current);
    });
    observer.observe(container.current);
    return () => { observer.disconnect(); graphContainer.removeEventListener('wheel', handleWheel); graphContainer.removeEventListener('pointerdown', handlePointerDown); graphContainer.removeEventListener('pointermove', handlePointerMove); graphContainer.removeEventListener('pointerup', clearPointerStart); graphContainer.removeEventListener('pointercancel', clearPointerStart); cy.destroy(); cyRef.current = null; hoveredId.current = null; selectedId.current = null; zoomedIn.current = false; pointerStart.current = null; };
  }, [onSelect]);
  useEffect(() => {
    const cy = cyRef.current;
    if (cy) cy.style().fromJson(createGraphStyle()).update();
  }, [theme]);
  useEffect(() => {
    const cy = cyRef.current;
    if (!cy) return undefined;
    const frame = window.requestAnimationFrame(() => {
      cy.resize();
      alignGraphToMap(cy, container.current, mapRef.current);
    });
    return () => window.cancelAnimationFrame(frame);
  }, [layoutVersion]);
  useEffect(() => {
    const cy = cyRef.current; if (!cy || !data) return;
    const nodes = data.network.nodes.map((n) => { const state = data.state.nodes.find((s) => s.id === n.id); const label = mapLabels[n.id] || n.name; return { data: { ...n, ...state, id: n.id, label, inspectLabel: `${label} · ${typeLabels[n.type] || n.type}`, failedLabel: `${label} ×`, type: n.type }, position: projectMapPosition(n.position), classes: impact?.exposedNodes?.includes(n.id) ? 'exposed' : '' }; });
    const edges = data.network.edges.map((e) => { const state = data.state.edges.find((s) => s.id === e.id); return { data: { ...e, ...state, id: e.id, source: e.from, target: e.to, label: e.id }, classes: impact?.exposedEdges?.includes(e.id) ? 'exposed' : '' }; });
    cy.batch(() => { cy.elements().remove(); cy.add([...nodes, ...edges]); }); if (!didFit.current) { didFit.current = true; cy.resize(); alignGraphToMap(cy, container.current, mapRef.current); }
    cy.nodes().toggleClass('zoomed-in', zoomedIn.current);
    cy.edges().toggleClass('zoomed-in', zoomedIn.current);
    applyInspectionFocus(cy, hoveredId.current || selectedId.current);
    cy.nodes().unselectify(); cy.edges().unselectify();
  }, [data, impact]);
  useEffect(() => {
    const cy = cyRef.current;
    if (!cy) return;
    selectedId.current = selection?.id || null;
    applyInspectionFocus(cy, hoveredId.current || selectedId.current);
  }, [selection]);
  const stopHudEvent = (event) => event.stopPropagation();
  const changeZoom = (factor) => {
    const cy = cyRef.current;
    if (!cy) return;
    setShowZoomHint(false);
    const level = Math.min(cy.maxZoom(), Math.max(cy.minZoom(), cy.zoom() * factor));
    const renderedCenter = { x: cy.width() / 2, y: cy.height() / 2 };
    if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
      cy.zoom({ level, renderedPosition: renderedCenter });
      return;
    }
    cy.animate({ zoom: { level, renderedPosition: renderedCenter } }, { duration: 200 });
  };
  const fitBounds = () => {
    const cy = cyRef.current;
    if (!cy) return;
    setShowZoomHint(false);
    if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
      cy.fit(cy.elements(), 30);
      return;
    }
    cy.animate({ fit: { eles: cy.elements(), padding: 30 } }, { duration: 250 });
  };
  useEffect(() => { const cy = cyRef.current; if (!cy) return; cy.elements().removeClass('dim highlighted'); if (!highlight) return; cy.elements().addClass('dim'); cy.nodes(highlight.nodes?.map((id) => `#${id}`).join(',') || 'node').filter((e) => highlight.nodes?.includes(e.id())).removeClass('dim').addClass('highlighted'); cy.edges(highlight.edges?.map((id) => `#${id}`).join(',') || 'edge').filter((e) => highlight.edges?.includes(e.id())).removeClass('dim').addClass('highlighted'); }, [highlight]);
  return <div className="graph-wrap">
    <img className="india-map" ref={mapRef} src="/india-states-survey-of-india.svg" alt="Survey of India state map" />
    <div className="graph" ref={container} aria-label="Interactive directed energy network" />
    <div className="map-hud" aria-label="Map viewport controls" onPointerDown={stopHudEvent} onPointerUp={stopHudEvent} onPointerMove={stopHudEvent} onPointerCancel={stopHudEvent} onMouseDown={stopHudEvent} onClick={stopHudEvent} onWheel={stopHudEvent} onTouchStart={stopHudEvent} onTouchMove={stopHudEvent} onTouchEnd={stopHudEvent}>
      <button className="map-hud-button" type="button" aria-label="Zoom in" title="Zoom in" onClick={() => changeZoom(1.25)}><svg aria-hidden="true" viewBox="0 0 24 24"><path d="M12 5v14M5 12h14"/></svg></button>
      <span className="map-zoom-level" aria-live="polite" aria-label={`Zoom ${zoomPercent}%`}>{zoomPercent}%</span>
      <button className="map-hud-button" type="button" aria-label="Zoom out" title="Zoom out" onClick={() => changeZoom(0.8)}><svg aria-hidden="true" viewBox="0 0 24 24"><path d="M5 12h14"/></svg></button>
      <button className="map-hud-button" type="button" aria-label="Fit network to view" title="Fit network to view" onClick={fitBounds}><svg aria-hidden="true" viewBox="0 0 24 24"><path d="M8 3H5a2 2 0 0 0-2 2v3m13-5h3a2 2 0 0 1 2 2v3M3 16v3a2 2 0 0 0 2 2h3m13-5v3a2 2 0 0 1-2 2h-3"/></svg></button>
    </div>
    {showZoomHint && <div className="map-zoom-hint" role="status"><span>Scroll or use + to zoom for detailed telemetry &amp; asset labels</span><button type="button" aria-label="Dismiss zoom hint" onPointerDown={stopHudEvent} onPointerUp={stopHudEvent} onPointerCancel={stopHudEvent} onClick={(event) => { event.stopPropagation(); setShowZoomHint(false); }}>×</button></div>}
    <div className="map-note">Survey of India map · illustrative network links</div>
    {hover && <div className="tooltip">{hover.id} · {hover.name || `${hover.from} → ${hover.to}`}<br/>{hover.status} · {hover.flow ?? hover.served ?? 0} / {hover.effectiveCapacity ?? hover.effectiveDemand ?? hover.capacity ?? 0} EU/day</div>}
  </div>;
}

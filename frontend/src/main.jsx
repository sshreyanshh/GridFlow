import React from 'react';
import { createRoot } from 'react-dom/client';
import Dashboard from './pages/Dashboard.jsx';
import { SimulationProvider } from './hooks/SimulationContext.jsx';
import './styles/global.css';

class DashboardBoundary extends React.Component {
  constructor(props) { super(props); this.state = { error: null }; }
  static getDerivedStateFromError(error) { return { error }; }
  render() { return this.state.error ? <main className="load-error"><h1>Dashboard error</h1><p>{this.state.error.message}</p><button onClick={() => window.location.reload()}>Reload</button></main> : this.props.children; }
}

createRoot(document.getElementById('root')).render(<React.StrictMode><DashboardBoundary><SimulationProvider><Dashboard/></SimulationProvider></DashboardBoundary></React.StrictMode>);

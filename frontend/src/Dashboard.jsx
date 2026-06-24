import { useEffect, useMemo, useState } from "react";
import Graph from "./Graph";
import "./index.css";

const API_BASE = "http://localhost:8080";

const emptyStatus = {
  projectName: "OptiBuild",
  projectRoot: "",
  lastScanTime: "not_scanned",
  watchMode: false,
  totalFiles: 0,
  changedFiles: [],
  affectedFiles: [],
  rebuildFiles: [],
  skippedFiles: 0,
  rebuildReductionPercent: 0,
  buildStatus: "not_started",
  testStatus: "not_started",
  events: [],
  files: [],
  graph: { edges: [] },
};

export default function Dashboard() {
  const [status, setStatus] = useState(emptyStatus);
  const [connected, setConnected] = useState(false);
  const [message, setMessage] = useState("");

  const fetchStatus = async () => {
    try {
      const response = await fetch(`${API_BASE}/api/status`);
      if (!response.ok) throw new Error("API unavailable");
      const data = await response.json();
      setStatus({ ...emptyStatus, ...data });
      setConnected(true);
      setMessage("");
    } catch {
      setConnected(false);
      setMessage("Disconnected from OptiBuild API. Run ./build/optibuild dashboard in another terminal.");
    }
  };

  useEffect(() => {
    const initialLoad = setTimeout(fetchStatus, 0);
    const interval = setInterval(fetchStatus, 1000);
    return () => {
      clearTimeout(initialLoad);
      clearInterval(interval);
    };
  }, []);

  const files = useMemo(() => status.files || [], [status.files]);
  const nodes = useMemo(
    () => files.map((file) => ({ id: file.path, status: file.status })),
    [files],
  );

  async function runAction(path, label) {
    setMessage(`${label} requested...`);
    try {
      const response = await fetch(`${API_BASE}${path}`, { method: "POST" });
      if (!response.ok) throw new Error(`${label} failed`);
      const data = await response.json();
      setStatus({ ...emptyStatus, ...data });
      setConnected(true);
      setMessage(`${label} complete`);
    } catch {
      setConnected(false);
      setMessage(`Could not run ${label}. Check that the OptiBuild API is running.`);
    }
  }

  return (
    <main className="dashboard-shell">
      <section className="hero-panel">
        <div>
          <p className="eyebrow">Reusable C++ selective builds</p>
          <h1>{status.projectName || "OptiBuild Dashboard"}</h1>
          <p className="hero-copy">
            Live dependency analysis, affected-file detection, and build status for any configured C++ project.
          </p>
        </div>
        <div className="live-stack">
          <div className={`live-indicator ${connected ? "connected" : "disconnected"}`}>
            <span />
            {connected ? "Live" : "Disconnected"}
          </div>
          <div className="scan-badge">
            <span>Last scan</span>
            <strong>{status.lastScanTime}</strong>
          </div>
        </div>
      </section>

      {message && <div className={`notice ${connected ? "" : "error"}`}>{message}</div>}

      <section className="action-row">
        <button onClick={() => runAction("/api/scan", "Scan")}>Scan Now</button>
        <button onClick={() => runAction("/api/build", "Build")}>Build</button>
        <button onClick={() => runAction("/api/test", "Test")}>Test</button>
        <button disabled title="Start watch from the CLI with optibuild watch">
          {status.watchMode ? "Watching" : "Start Watch"}
        </button>
      </section>

      <section className="metric-grid">
        <MetricCard label="Files scanned" value={status.totalFiles} />
        <MetricCard label="Changed files" value={status.changedFiles.length} tone="changed" />
        <MetricCard label="Affected files" value={status.affectedFiles.length} tone="affected" />
        <MetricCard label="Skipped files" value={status.skippedFiles} tone="skipped" />
        <MetricCard label="Reduction" value={`${Number(status.rebuildReductionPercent).toFixed(1)}%`} tone="good" />
        <MetricCard label="Build status" value={status.buildStatus} />
      </section>

      <section className="content-grid">
        <article className="panel graph-panel">
          <div className="panel-header">
            <div>
              <h2>Dependency Graph</h2>
              <p>{status.graph?.edges?.length ?? 0} directed include relationships</p>
            </div>
          </div>
          <Graph nodes={nodes} edges={status.graph?.edges || []} />
        </article>

        <article className="panel">
          <div className="panel-header">
            <div>
              <h2>Latest Events</h2>
              <p>Updated by scan, watch, build, and test actions</p>
            </div>
          </div>
          <div className="event-list">
            {(status.events || []).length === 0 ? (
              <p className="muted">No events yet. Run a scan to begin.</p>
            ) : (
              status.events.slice(-8).reverse().map((event, index) => (
                <div className="event-item" key={`${event.time}-${index}`}>
                  <strong>{event.message}</strong>
                  <span>{event.time}</span>
                </div>
              ))
            )}
          </div>
        </article>
      </section>

      <section className="panel">
        <div className="panel-header">
          <div>
            <h2>File Impact Table</h2>
            <p>{status.projectRoot}</p>
          </div>
        </div>
        <div className="table-wrap">
          <table>
            <thead>
              <tr>
                <th>File</th>
                <th>Status</th>
                <th>Dependencies</th>
              </tr>
            </thead>
            <tbody>
              {files.map((file) => (
                <tr key={file.path}>
                  <td><code>{file.path}</code></td>
                  <td><span className={`status ${file.status}`}>{file.status}</span></td>
                  <td>{file.dependencyCount}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </section>
    </main>
  );
}

function MetricCard({ label, value, tone = "" }) {
  return (
    <article className={`metric-card ${tone}`}>
      <span>{label}</span>
      <strong>{value}</strong>
    </article>
  );
}

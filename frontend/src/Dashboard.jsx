import { useEffect, useMemo, useState } from "react";
import Graph from "./Graph";
import "./index.css";

const emptyStatus = {
  projectName: "OptiBuild",
  projectRoot: "",
  lastScanTime: "not_scanned",
  totalFiles: 0,
  sourceFiles: 0,
  dependencyEdges: 0,
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
  const [loaded, setLoaded] = useState(false);
  const [message, setMessage] = useState("");

  const fetchReport = async () => {
    try {
      const [outputResponse, graphResponse] = await Promise.all([
        fetch("/output.json"),
        fetch("/graph.json"),
      ]);
      if (!outputResponse.ok) throw new Error("No OptiBuild report found");
      const output = await outputResponse.json();
      const graph = graphResponse.ok ? await graphResponse.json() : { edges: [] };
      setStatus(normalizeReport(output, graph));
      setLoaded(true);
      setMessage("");
    } catch {
      setLoaded(false);
      setMessage("No report loaded. Run optibuild --scan --output frontend/public, then refresh this page.");
    }
  };

  useEffect(() => {
    fetchReport();
    const interval = setInterval(fetchReport, 1000);
    return () => clearInterval(interval);
  }, []);

  const files = useMemo(() => status.files || [], [status.files]);
  const nodes = useMemo(
    () => files.map((file) => ({ id: file.path, status: file.status })),
    [files],
  );

  return (
    <main className="dashboard-shell">
      <section className="hero-panel">
        <div>
          <p className="eyebrow">Reusable C++ selective builds</p>
          <h1>{status.projectName || "OptiBuild Dashboard"}</h1>
          <p className="hero-copy">
            Live dependency analysis, affected-file detection, and build impact data for any configured C++ project.
          </p>
        </div>
        <div className="live-stack">
          <div className={`live-indicator ${loaded ? "connected" : "disconnected"}`}>
            <span />
            {loaded ? "Live report" : "No report"}
          </div>
          <div className="scan-badge">
            <span>Last scan</span>
            <strong>{status.lastScanTime}</strong>
          </div>
        </div>
      </section>

      {message && <div className={`notice ${loaded ? "" : "error"}`}>{message}</div>}

      <section className="action-row">
        <button onClick={fetchReport}>Reload Report</button>
        <button disabled title="Run optibuild --scan from the terminal">Scan</button>
        <button disabled title="Run optibuild --build from the terminal">Build</button>
        <button disabled title="Run optibuild --test from the terminal">Test</button>
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
              <p>Updated while optibuild --watch writes JSON output</p>
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

function normalizeReport(output, graph) {
  if (output.project && output.summary) {
    return {
      ...emptyStatus,
      projectName: output.project.name,
      projectRoot: output.project.root,
      lastScanTime: output.summary.last_scan_time,
      totalFiles: output.summary.total_files,
      sourceFiles: output.summary.source_files,
      dependencyEdges: output.summary.dependency_edges,
      changedFiles: output.changed_files || [],
      affectedFiles: output.affected_files || [],
      rebuildFiles: output.built_files || [],
      skippedFiles: output.summary.skipped_files,
      rebuildReductionPercent: output.summary.optimization,
      files: output.files || [],
      graph: { edges: graph.edges || [] },
      events: [
        {
          time: output.summary.last_scan_time,
          message: `${output.summary.affected_files} affected files detected`,
        },
      ],
    };
  }

  return {
    ...emptyStatus,
    ...output,
    graph: output.graph || graph || { edges: [] },
  };
}

import ReactFlow from "reactflow";
import "reactflow/dist/style.css";

export default function Graph({ edges = [], impactedFiles = [] }) {

  const nodesMap = new Map();
  const flowEdges = [];

  let x = 0;
  let y = 0;

  edges.forEach((e, i) => {

    const isImpactedFrom = impactedFiles.some(f => e.from.includes(f));
    const isImpactedTo = impactedFiles.some(f => e.to.includes(f));

    // 🔹 FROM NODE
    if (!nodesMap.has(e.from)) {
      nodesMap.set(e.from, {
        id: e.from,
        position: { x, y }, // ✅ REQUIRED
        data: { label: e.from },
        style: {
          background: isImpactedFrom ? "#ff4d4d" : "#1e1e1e",
          color: "white",
          border: "1px solid #555"
        }
      });
      y += 80;
    }

    // 🔹 TO NODE
    if (!nodesMap.has(e.to)) {
      nodesMap.set(e.to, {
        id: e.to,
        position: { x: x + 250, y }, // ✅ REQUIRED
        data: { label: e.to },
        style: {
          background: isImpactedTo ? "#ff4d4d" : "#1e1e1e",
          color: "white",
          border: "1px solid #555"
        }
      });
      y += 80;
    }

    flowEdges.push({
      id: i.toString(),
      source: e.from,
      target: e.to,
      animated: isImpactedFrom || isImpactedTo
    });
  });

  const nodes = Array.from(nodesMap.values());

  return (
    <div style={{ height: 500 }}>
      <h3>🔗 Dependency Graph</h3>
      <ReactFlow nodes={nodes} edges={flowEdges} fitView />
    </div>
  );
}
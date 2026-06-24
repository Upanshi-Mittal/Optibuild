import ReactFlow, { Background, Controls } from "reactflow";
import "reactflow/dist/style.css";

const statusColor = {
  changed: "#dc2626",
  affected: "#f59e0b",
  skipped: "#2563eb",
};

export default function Graph({ nodes = [], edges = [] }) {
  const nodeStatus = new Map(nodes.map((node) => [node.id, node.status]));
  const flowNodes = buildNodes(nodes, edges);
  const flowEdges = edges.map((edge, index) => {
    const fromStatus = nodeStatus.get(edge.from);
    const toStatus = nodeStatus.get(edge.to);
    return {
      id: `${edge.from}-${edge.to}-${index}`,
      source: edge.from,
      target: edge.to,
      animated: fromStatus !== "skipped" || toStatus !== "skipped",
      style: {
        stroke: fromStatus === "changed" || toStatus === "changed" ? "#dc2626" : "#64748b",
        strokeWidth: 2,
      },
    };
  });

  return (
    <div className="flow-canvas">
      <ReactFlow nodes={flowNodes} edges={flowEdges} fitView>
        <Background color="#dbe3ef" gap={18} />
        <Controls />
      </ReactFlow>
    </div>
  );
}

function buildNodes(nodes, edges) {
  const allIds = new Set(nodes.map((node) => node.id));
  edges.forEach((edge) => {
    allIds.add(edge.from);
    allIds.add(edge.to);
  });

  const statusById = new Map(nodes.map((node) => [node.id, node.status]));
  return Array.from(allIds).map((id, index) => {
    const column = index % 3;
    const row = Math.floor(index / 3);
    const status = statusById.get(id) || "skipped";

    return {
      id,
      position: { x: column * 280, y: row * 110 },
      data: { label: id },
      style: {
        background: "#ffffff",
        border: `2px solid ${statusColor[status] || "#94a3b8"}`,
        borderRadius: 8,
        color: "#0f172a",
        fontSize: 12,
        padding: 10,
        width: 220,
      },
    };
  });
}

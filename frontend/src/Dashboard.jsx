import { useEffect, useState } from "react";
import Graph from "./Graph";
import "./index.css"
export default function Dashboard() {
    const [data, setData] = useState(null);
    const [graph, setGraph] = useState(null);

    useEffect(() => {
        const interval = setInterval(() => {
            Promise.all([
                fetch("/output.json").then(res => res.json()),
                fetch("/graph.json").then(res => res.json())
            ])
                .then(([output, graphData]) => {
                    setData(output);
                    setGraph(graphData);
                });
        }, 2000);

        return () => clearInterval(interval);
    }, []);

    if (!data || !graph) return <p>Loading...</p>;

    const uniqueFiles = [...new Set(data.built_files)];

    return (
        <div style={{ padding: "20px", color: "white" }}>
            <h1> OptiBuild Dashboard</h1>

            <div className="card"> <h3>Built Files</h3>
                <ul style={{ listStyle: "none", padding: 0 }}>
                    {uniqueFiles.map((file, i) => (
                        <li key={i}>{file}</li>
                    ))}
                </ul>
            </div>


            <div className="card">


                <h3> Stats</h3>
                <p> Total Files: {data.total_files ?? 0}</p>
                <p> Built Files: {data.built_count ?? 0}</p>

            </div>
            <div className="card">
                <p>
                    Optimization: {data.optimization ? data.optimization.toFixed(2) : "90"}%
                </p>


                <p>
                    Build Time: {data.time_taken ? data.time_taken.toFixed(3) : "1.2"}s
                </p>
            </div>
            <div className="card">
                <Graph edges={graph?.edges || []} impactedFiles={uniqueFiles || []} />    </div >

        </div>
    );
}
import Plot from "react-plotly.js";

export default function RadarPointPlot({ x, y, z }) {
  return (
    <div className="p-4 bg-white rounded shadow">
      <h2 className="text-xl font-semibold mb-2">Fall Detection Point Cloud</h2>
      <Plot
        data={[
          {
            x: x,
            y: y,
            z: z,
            mode: "markers",
            type: "scatter3d",
            marker: { 
              color: "blue", 
              size: 3,
              opacity: 0.7
            },
          },
        ]}
        layout={{ 
          width: 500, 
          height: 400, 
          margin: { l: 0, r: 0, b: 0, t: 0 },
          scene: {
            xaxis: { title: 'X (m)' },
            yaxis: { title: 'Y (m)' },
            zaxis: { title: 'Z (m)' },
            camera: {
              eye: { x: 1.5, y: 1.5, z: 1.5 }
            }
          }
        }}
      />
    </div>
  );
}

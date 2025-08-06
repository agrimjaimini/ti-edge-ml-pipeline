import { useMemo } from 'react';
import Plot from 'react-plotly.js';

function RadarPointPlot({ x, y, z }) {
  const data = useMemo(() => [{
    type: 'scatter3d',
    mode: 'markers',
    x: x,
    y: y,
    z: z,
    marker: {
      size: 3,
      color: z,
      colorscale: 'Viridis',
    }
  }], [x, y, z]);

  const layout = {
    autosize: true,
    height: 400,
    scene: {
      aspectmode: 'cube',
      xaxis: { title: 'X Position (m)' },
      yaxis: { title: 'Y Position (m)' },
      zaxis: { title: 'Z Position (m)' },
      camera: {
        eye: { x: 1.5, y: 1.5, z: 1.5 }
      }
    },
    margin: {
      l: 0, r: 0, t: 0, b: 0
    }
  };

  const config = {
    responsive: true,
    displayModeBar: false
  };

  return (
    <Plot
      data={data}
      layout={layout}
      config={config}
      style={{ width: '100%', height: '100%' }}
    />
  );
}

export default RadarPointPlot;

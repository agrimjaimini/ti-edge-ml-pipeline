import { useMemo } from 'react';
import Plot from 'react-plotly.js';

function ProbabilityBarChart({ probabilities }) {
  const data = useMemo(() => [{
    type: 'bar',
    x: probabilities.map((_, i) => `Class ${i}`),
    y: probabilities,
    marker: {
      color: probabilities.map(p => `rgba(66, 135, 245, ${p})`),
    },
  }], [probabilities]);

  const layout = {
    autosize: true,
    height: 300,
    yaxis: {
      title: 'Probability',
      range: [0, 1],
      tickformat: ',.0%'
    },
    margin: {
      l: 50, r: 20, t: 20, b: 40
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

export default ProbabilityBarChart;

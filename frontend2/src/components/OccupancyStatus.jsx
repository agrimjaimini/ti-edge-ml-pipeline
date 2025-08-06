export default function OccupancyStatus({ count }) {
    return (
      <div className="p-4 bg-white rounded shadow">
        <h2 className="text-2xl font-semibold">People Detected</h2>
        <p className="text-3xl mt-2 text-blue-600">{count}</p>
      </div>
    );
  }
  
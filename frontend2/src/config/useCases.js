export const useCases = {
  fallDetection: {
    name: "Fall Detection Dashboard",
    description: "Real-time fall detection monitoring system",
    metrics: [
      {
        id: "fallStatus",
        label: "Fall Status",
        dataKey: "fall_detected",
        type: "status",
        colors: {
          true: "red",
          false: "green"
        }
      },
      {
        id: "confidence",
        label: "Detection Confidence",
        dataKey: "confidence",
        type: "percentage"
      },
      {
        id: "lastFall",
        label: "Last Fall Detected",
        dataKey: "last_fall_time",
        type: "timestamp"
      }
    ],
    visualizations: [
      {
        type: "pointCloud",
        title: "3D Point Cloud",
        description: "Real-time visualization of radar points",
        dataKeys: {
          x: "x_pos",
          y: "y_pos",
          z: "z_pos"
        }
      },
      {
        type: "probability",
        title: "Fall Probability",
        description: "Real-time fall detection probability",
        dataKey: "probabilities"
      }
    ],
    dataFormat: {
      required: ["x_pos", "y_pos", "z_pos", "fall_detected", "confidence"]
    }
  },

  occupancy: {
    name: "Occupancy Dashboard",
    description: "Real-time occupancy monitoring system",
    metrics: [
      {
        id: "peopleCount",
        label: "People Detected",
        dataKey: "predicted_count",
        type: "number"
      },
      {
        id: "avgOccupancy",
        label: "Daily Average",
        dataKey: "daily_average",
        type: "number",
        precision: 1
      },
      {
        id: "maxOccupancy",
        label: "Max Occupancy",
        dataKey: "max_occupancy",
        type: "number"
      }
    ],
    visualizations: [
      {
        type: "pointCloud",
        title: "Room Overview",
        description: "Real-time visualization of detected points",
        dataKeys: {
          x: "x_pos",
          y: "y_pos",
          z: "z_pos"
        }
      },
      {
        type: "occupancyChart",
        title: "Occupancy Trend",
        description: "Real-time occupancy levels",
        dataKey: "occupancy_history"
      }
    ],
    dataFormat: {
      required: ["x_pos", "y_pos", "z_pos", "predicted_count"]
    }
  }
}; 
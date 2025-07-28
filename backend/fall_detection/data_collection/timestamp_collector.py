import json
from datetime import datetime
from pathlib import Path

class FallTimestampCollector:
    def __init__(self, output_file="data/timestamps/fall_timestamps.json"):
        self.timestamps = []
        self.output_file = output_file
        
    def add_timestamp(self):
        """Record current timestamp"""
        now = datetime.now()
        timestamp = now.strftime("%H:%M:%S,%f")[:-3]  # Format: HH:MM:SS,mmm
        self.timestamps.append({
            "timestamp": timestamp,
            "ms_of_day": (now.hour * 3600000 + 
                         now.minute * 60000 + 
                         now.second * 1000 + 
                         now.microsecond // 1000)
        })
        print(f"Fall recorded at: {timestamp}")
        
    def save_timestamps(self):
        """Save timestamps to JSON file"""
        with open(self.output_file, 'w') as f:
            json.dump({"fall_events": self.timestamps}, f, indent=2)
        print(f"\nSaved {len(self.timestamps)} timestamps to {self.output_file}")
        
    def start_recording(self):
        """Start recording session"""
        print("\nRecording started!")
        print("Press Enter to mark fall event")
        print("Type 'q' and press Enter to stop recording\n")
        
        while True:
            user_input = input()
            if user_input.lower() == 'q':
                break
            self.add_timestamp()
        
        self.save_timestamps()
        print("Recording stopped!")

if __name__ == "__main__":
    # Create timestamps directory if it doesn't exist
    Path("data/timestamps").mkdir(parents=True, exist_ok=True)
    
    # Start recording
    collector = FallTimestampCollector()
    collector.start_recording() 
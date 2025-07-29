import json
from pathlib import Path
from typing import List, Dict, Any

class SequenceBuilder:
    def __init__(self, sequence_length: int = 30, frame_variations: int = 0, no_fall_slide: int = 30):
        """
        Initialize sequence builder
        sequence_length: number of frames per sequence
        frame_variations: number of frame variations before/after fall (+/-)
        no_fall_slide: number of frames to skip for no-fall sequences
        """
        self.sequence_length = sequence_length
        self.frame_variations = frame_variations
        self.no_fall_slide = no_fall_slide
        
    def find_nearest_frame(self, frames: List[Dict], timestamp_ms: int) -> int:
        """Find index of frame closest to timestamp"""
        return min(range(len(frames)), 
                  key=lambda i: abs(frames[i]["Time Stamp (ms)"] - timestamp_ms))
    
    def create_sequence(self, frames: List[Dict], start_idx: int) -> Dict:
        """Create a single sequence starting from start_idx"""
        if start_idx + self.sequence_length > len(frames):
            return None
            
        sequence_frames = frames[start_idx:start_idx + self.sequence_length]
        return {
            "frames": sequence_frames,
            "start_frame": start_idx,
            "start_time": sequence_frames[0]["Time Stamp (ms)"],
            "end_time": sequence_frames[-1]["Time Stamp (ms)"]
        }
    
    def build_no_fall_sequences(self, input_json: str, output_json: str):
        """
        Create sequences from normal activity using sliding window with skip
        """
        # Load frames
        with open(input_json, 'r') as f:
            frames = json.load(f)
            
        sequences = []
        # If no_fall_slide is 0, use step size of 1
        step = max(1, self.no_fall_slide)
        
        # Slide window over frames with skip
        for i in range(0, len(frames) - self.sequence_length + 1, step):
            sequence = self.create_sequence(frames, i)
            if sequence:
                sequence["label"] = "no_fall"
                sequences.append(sequence)
                
        # Save sequences
        if Path(output_json).exists():
            with open(output_json, 'r') as f:
                existing_data = json.load(f)
                existing_sequences = existing_data.get("sequences", [])
        else:
            existing_sequences = []

        # Combine old and new sequences
        all_sequences = existing_sequences + sequences

        # Write combined sequences to file
        with open(output_json, 'w') as f:
            json.dump({"sequences": all_sequences}, f, indent=2)
            
        print(f"Created {len(sequences)} no-fall sequences")
    
    def build_fall_sequences(self, input_json: str, timestamps_json: str, output_json: str):
        """
        Create sequences from fall events using ±frame_variations approach
        """
        # Load frames and timestamps
        with open(input_json, 'r') as f:
            frames = json.load(f)
        with open(timestamps_json, 'r') as f:
            fall_events = json.load(f)["fall_events"]
            
        sequences = []
        
        # Process each fall event
        for event in fall_events:
            fall_time_ms = event["ms_of_day"]
            fall_frame_idx = self.find_nearest_frame(frames, fall_time_ms)
            
            # Create variations around fall frame
            for offset in range(-self.frame_variations, self.frame_variations + 1):
                start_idx = fall_frame_idx + offset
                if start_idx >= 0 and start_idx + self.sequence_length <= len(frames):
                    sequence = self.create_sequence(frames, start_idx)
                    if sequence:
                        sequence["label"] = "fall"
                        sequence["fall_frame_offset"] = offset
                        sequences.append(sequence)
        
        # Save sequences
        with open(output_json, 'w') as f:
            json.dump({"sequences": sequences}, f, indent=2)
            
        print(f"Created {len(sequences)} fall sequences from {len(fall_events)} events")

def process_data():
    """Process both fall and no-fall data"""
    # Create output directories
    Path("data/sequences").mkdir(parents=True, exist_ok=True)
    
    # Create builder with no variations or sliding
    builder = SequenceBuilder(
        sequence_length=30,
        frame_variations=0,  # No frame variations for falls
        no_fall_slide=30      # No sliding window for no-falls
    )
    
    # Process no-fall data
    no_fall_input = "/Users/saahi/Desktop/TI_MMWAVE_PROJ/ti-edge-ml-pipeline/data/json/no_fall_training1.json"
    no_fall_output = "data/sequences/no_fall_sequences.json"
    if Path(no_fall_input).exists():
        print("Processing no-fall data...")
        builder.build_no_fall_sequences(no_fall_input, no_fall_output)
    else:
        print(f"No-fall data not found at {no_fall_input}")
    
    # Process fall data
    fall_input = "/Users/saahi/Desktop/TI_MMWAVE_PROJ/ti-edge-ml-pipeline/data/json/fall_training.json"
    timestamps = "data/timestamps/fall_timestamps.json"
    fall_output = "data/sequences/fall_sequences.json"
    if Path(fall_input).exists() and Path(timestamps).exists():
        print("Processing fall data...")
        builder.build_fall_sequences(fall_input, timestamps, fall_output)
    else:
        print(f"Fall data or timestamps not found")

if __name__ == "__main__":
    process_data() 
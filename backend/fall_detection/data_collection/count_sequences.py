import json

def count_sequences(json_path: str):
    """Count sequences in a JSON file"""
    try:
        with open(json_path, 'r') as f:
            data = json.load(f)
            num_sequences = len(data["sequences"])
            
            # Get sequence length from first sequence
            if num_sequences > 0:
                first_sequence = data["sequences"][0]
                sequence_length = len(first_sequence["frames"])
            else:
                sequence_length = 0
            
            print(f"\nSequence Count in {json_path}:")
            print(f"Number of sequences: {num_sequences}")
            print(f"Each sequence length: {sequence_length} frames")
            return num_sequences
            
    except FileNotFoundError:
        print(f"File not found: {json_path}")
        return 0
    except json.JSONDecodeError:
        print(f"Error reading JSON from {json_path}")
        return 0
    except KeyError:
        print(f"No 'sequences' key found in {json_path}")
        return 0

if __name__ == "__main__":
    # Count fall sequences
    fall_count = count_sequences("data/sequences/fall_sequences.json")
    
    # Count no-fall sequences
    no_fall_count = count_sequences("data/sequences/no_fall_sequences.json")
    
    # Show totals
    if fall_count > 0 or no_fall_count > 0:
        print("\nTotal Counts:")
        print(f"Fall sequences: {fall_count}")
        print(f"No-fall sequences: {no_fall_count}")
        print(f"Total sequences: {fall_count + no_fall_count}") 
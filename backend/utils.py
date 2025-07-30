import os

def get_repo_root():
    """Get the absolute path to the repository root."""
    return os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

def get_models_dir():
    """Get the absolute path to the models directory."""
    return os.path.join(get_repo_root(), "models")

def get_data_dir():
    """Get the absolute path to the data directory."""
    return os.path.join(get_repo_root(), "data")

def get_model_path(model_name: str) -> str:
    """Get the absolute path to a specific model file."""
    return os.path.join(get_models_dir(), f"{model_name}.pth")

def get_data_subdir(subdir: str) -> str:
    """Get the absolute path to a subdirectory within the data directory."""
    return os.path.join(get_data_dir(), subdir) 
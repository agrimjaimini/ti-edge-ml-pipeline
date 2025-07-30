import sqlite3
import os
import json
from datetime import datetime
from typing import List, Dict, Optional

class ModelDatabase:
    def __init__(self, db_path: str = "models.db"):
        self.db_path = db_path
        self.init_database()
    
    def init_database(self):
        """Initialize the database with the models table."""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS models (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    name TEXT UNIQUE NOT NULL,
                    file_path TEXT NOT NULL,
                    num_classes INTEGER NOT NULL,
                    data_dir TEXT NOT NULL,
                    epochs INTEGER NOT NULL,
                    batch_size INTEGER NOT NULL,
                    learning_rate REAL NOT NULL,
                    weight_decay REAL NOT NULL,
                    file_size_mb REAL,
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    metadata TEXT
                )
            ''')
            conn.commit()

    def add_model(self, 
                  name: str,
                  file_path: str,
                  num_classes: int,
                  data_dir: str,
                  epochs: int,
                  batch_size: int,
                  learning_rate: float,
                  weight_decay: float,
                  metadata: Dict = None) -> bool:
        try:
            file_size_mb = os.path.getsize(file_path) / (1024 * 1024) if os.path.exists(file_path) else 0
                
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                cursor.execute('''
                    INSERT OR REPLACE INTO models 
                    (name, file_path, num_classes, data_dir, epochs, batch_size, 
                    learning_rate, weight_decay, file_size_mb, metadata)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ''', (
                    name, file_path, num_classes, data_dir, epochs, batch_size,
                    learning_rate, weight_decay, file_size_mb,
                    json.dumps(metadata) if metadata else None
                ))
                conn.commit()
                return True
        except Exception as e:
            print(f"Error adding model to database: {e}")
            return False

    def get_model(self, name: str) -> Optional[Dict]:
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute('SELECT * FROM models WHERE name = ?', (name,))
            row = cursor.fetchone()
            
            if row:
                columns = [description[0] for description in cursor.description]
                model_dict = dict(zip(columns, row))
                if model_dict.get('metadata'):
                    model_dict['metadata'] = json.loads(model_dict['metadata'])
                return model_dict
            return None
    
    def get_all_models(self) -> List[Dict]:
        """Get all models from the database."""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute('SELECT * FROM models ORDER BY created_at DESC')
            rows = cursor.fetchall()
            
            columns = [description[0] for description in cursor.description]
            models = []
            for row in rows:
                model_dict = dict(zip(columns, row))
                if model_dict.get('metadata'):
                    model_dict['metadata'] = json.loads(model_dict['metadata'])
                models.append(model_dict)
            
            return models

model_db = ModelDatabase()
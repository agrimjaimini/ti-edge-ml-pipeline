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
        """Initialize the database with the models and files tables."""
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
            
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS files (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    filename TEXT NOT NULL,
                    file_path TEXT UNIQUE NOT NULL,
                    file_size_bytes INTEGER NOT NULL,
                    content_type TEXT,
                    uploaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
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
            try:
                file_size_mb = os.path.getsize(file_path) / (1024 * 1024) if os.path.exists(file_path) else 0
            except OSError:
                file_size_mb = 0
                
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

    def add_file(self, 
                 filename: str,
                 file_path: str,
                 file_size_bytes: int,
                 content_type: str = None,
                 metadata: Dict = None) -> bool:
        """Add a file record to the database."""
        try:
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                cursor.execute('''
                    INSERT INTO files 
                    (filename, file_path, file_size_bytes, content_type, metadata)
                    VALUES (?, ?, ?, ?, ?)
                ''', (
                    filename, file_path, file_size_bytes, content_type,
                    json.dumps(metadata) if metadata else None
                ))
                conn.commit()
                return True
        except Exception as e:
            print(f"Error adding file to database: {e}")
            return False

    def get_file(self, filename: str) -> Optional[Dict]:
        """Get a specific file record from the database."""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute('SELECT * FROM files WHERE filename = ?', (filename,))
            row = cursor.fetchone()
            
            if row:
                columns = [description[0] for description in cursor.description]
                file_dict = dict(zip(columns, row))
                if file_dict.get('metadata'):
                    file_dict['metadata'] = json.loads(file_dict['metadata'])
                return file_dict
            return None

    def get_all_files(self) -> List[Dict]:
        """Get all files from the database."""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute('SELECT * FROM files ORDER BY uploaded_at DESC')
            rows = cursor.fetchall()
            
            columns = [description[0] for description in cursor.description]
            files = []
            for row in rows:
                file_dict = dict(zip(columns, row))
                if file_dict.get('metadata'):
                    file_dict['metadata'] = json.loads(file_dict['metadata'])
                files.append(file_dict)
            
            return files

    def delete_file(self, filename: str) -> bool:
        """Delete a file record from the database."""
        try:
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                cursor.execute('DELETE FROM files WHERE filename = ?', (filename,))
                conn.commit()
                return cursor.rowcount > 0
        except Exception as e:
            print(f"Error deleting file from database: {e}")
            return False

model_db = ModelDatabase()
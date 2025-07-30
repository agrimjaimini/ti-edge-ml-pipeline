FROM python:3.12

WORKDIR /app/backend

# Copy requirements first for better caching
COPY requirements.txt requirements.txt
RUN pip install --no-cache-dir -r requirements.txt

# Copy all backend code
COPY . .

# Set Python path to include backend directory
ENV PYTHONPATH=/app/backend

# Expose the port we're using in main.py
EXPOSE 8000

# Run training then start server (sequential)
CMD ["sh", "-c", "python -m fall_detection.model.pointnet_lstm && uvicorn main:app --host 0.0.0.0 --port 8000"] 
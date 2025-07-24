FROM python:3.12

WORKDIR /app

# deps first for better cache
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# app code
COPY . .

ENV PYTHONPATH=/app
EXPOSE 8080

# run training then start server (sequential)
CMD ["sh", "-c", "python -m backend.model.model && cd backend && python -m backend.main"]
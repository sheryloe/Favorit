FROM python:3.12-slim

WORKDIR /app

RUN pip install --no-cache-dir pillow

COPY . /app

ENTRYPOINT ["python", "/app/tools/test/crop_screenshots.py"]

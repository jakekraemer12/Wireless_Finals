# Impport libraries
import time, os
from picamera2 import Picamera2
import boto3
import requests

# Configure S3 variables
BUCKET = 'wireless-network-photos'
REGION = 'us-west-1'
ACCESS_KEY = 'AKIARPAZFN3YTZ7S5QWS'
SECRET_KEY = '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'
IMG_DIR = '/home/pi/images'
API_URL = "http://54.176.164.28:5000/s3-event" 

# Create file, give name of file current timestamp
ts = time.strftime('%Y%m%d-%H%M%S')
filename = f"{ts}.jpg"
path = f"/home/pi/images/{filename}"

# Camera Inititialization
camera = Picamera2()

# Ensures image directory exists
os.makedirs(IMG_DIR, exist_ok=True)

# Initialize boto3 S3 client
s3 = boto3.client('s3', region_name=REGION, aws_access_key_id=ACCESS_KEY, aws_secret_access_key=SECRET_KEY)

# Capture photo and upload to S3
camera.start()
camera.capture_file(path)
camera.close()
s3.upload_file(path, BUCKET, filename)

# Wait for file to upload
time.sleep(10)

# POST JSON body to Flask route
try:
    requests.post(API_URL, json={"object_key": filename})
except Exception as e:
    print("POST failed:", e)
from flask import Flask, render_template
from pymongo import MongoClient
from datetime import datetime, timedelta

app = Flask(__name__)

# Connect to MongoDB
client = MongoClient("mongodb://127.0.0.1:27017/")
db = client["food_monitoring"]
collection = db["sensor_data"]

@app.route('/')
def index():
    # Get the last 24 hours of data
    last_24h = datetime.now() - timedelta(hours=24)
    sensor_data = list(collection.find({
        "timestamp": {"$gte": last_24h.strftime("%Y-%m-%d %H:%M:%S")}
    }).sort("timestamp", -1))
    
    # Get the latest reading
    latest_data = sensor_data[0] if sensor_data else None
    
    return render_template('index.html', 
                         latest_data=latest_data,
                         historical_data=sensor_data)

if __name__ == '__main__':
    app.run(debug=True) 
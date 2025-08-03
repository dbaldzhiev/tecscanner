from flask import Flask
from .recording_manager import RecordingManager

manager = RecordingManager()

app = Flask(__name__)

import os

@app.route('/')
def index():
    from flask import render_template
    return render_template('index.html', status=manager.status(), recordings=manager.list_recordings())

@app.post('/start')
def start_recording():
    if manager.start_recording():
        return {'status': 'recording started'}
    return {'status': 'already recording'}, 400

@app.post('/stop')
def stop_recording():
    if manager.stop_recording():
        return {'status': 'recording stopped'}
    return {'status': 'not recording'}, 400

@app.get('/status')
def status():
    return manager.status()

@app.get('/recordings')
def recordings():
    return {'recordings': manager.list_recordings()}

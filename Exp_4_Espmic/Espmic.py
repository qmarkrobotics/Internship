10.247.234.22import socket
import struct
import wave
import io
import speech_recognition as sr
import webbrowser

UDP_IP   = "0.0.0.0"
UDP_PORT = 5005
SAMPLE_RATE = 16000
CHUNK_DURATION = 3  # seconds per recognition attempt

commands = {
    "youtube": "https://www.youtube.com",
    "google":  "https://www.google.com",
    "github":  "https://www.github.com",
    "netflix": "https://www.netflix.com",
}

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

r = sr.Recognizer()
print("Listening via ESP32 mic...")

samples_needed = SAMPLE_RATE * CHUNK_DURATION
buffer = b""

while True:
    data, _ = sock.recvfrom(4096)
    buffer += data

    if len(buffer) >= samples_needed * 2:  # 2 bytes per int16
        # wrap in WAV format for speech_recognition
        wav_io = io.BytesIO()
        with wave.open(wav_io, 'wb') as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(SAMPLE_RATE)
            wf.writeframes(buffer[:samples_needed * 2])
        wav_io.seek(0)

        with sr.AudioFile(wav_io) as source:
            audio = r.record(source)

        try:
            text = r.recognize_google(audio).lower()
            print("Heard:", text)
            for keyword, url in commands.items():
                if keyword in text:
                    print(f"Opening {keyword}...")
                    webbrowser.open(url)
                    break
        except sr.UnknownValueError:
            print("Didn't catch that")
        except sr.RequestError as e:
            print("API error:", e)

        buffer = buffer[samples_needed * 2:]  # slide window
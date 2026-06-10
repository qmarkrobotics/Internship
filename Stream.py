import asyncio
import socket
import wave
import io
import json
import speech_recognition as sr
import webbrowser
import websockets
from google import genai
from google.genai import types

client = genai.Client(api_key="API_Key", http_options={"api_version": "v1beta"})
MODEL = "gemini-2.5-flash-native-audio-preview-12-2025"
CONFIG = types.LiveConnectConfig(
    response_modalities=["AUDIO"],
    system_instruction="You are a helpful voice assistant. Keep answers short, maximum 2-3 sentences.",
    speech_config=types.SpeechConfig(
        voice_config=types.VoiceConfig(
            prebuilt_voice_config=types.PrebuiltVoiceConfig(voice_name="Puck")
        )
    ),
)

ESP32_WS    = "ws://10.247.234.83:8765"
UDP_IP      = "0.0.0.0"
UDP_PORT    = 5005
SAMPLE_RATE = 16000
CHUNK_SECS  = 3

commands = {
    "youtube": "https://www.youtube.com",
    "google":  "https://www.google.com",
    "github":  "https://www.github.com",
    "netflix": "https://www.netflix.com",
}

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.setblocking(False)

r = sr.Recognizer()

async def main():
    async with client.aio.live.connect(model=MODEL, config=CONFIG) as session:
        async with websockets.connect(ESP32_WS) as ws:
            print("Ready. Speak...\n")
            mic_buffer = b""
            samples_needed = SAMPLE_RATE * CHUNK_SECS * 2

            while True:
                try:
                    data, _ = sock.recvfrom(4096)
                    mic_buffer += data
                except BlockingIOError:
                    pass

                if len(mic_buffer) >= samples_needed:
                    wav_io = io.BytesIO()
                    with wave.open(wav_io, 'wb') as wf:
                        wf.setnchannels(1)
                        wf.setsampwidth(2)
                        wf.setframerate(SAMPLE_RATE)
                        wf.writeframes(mic_buffer[:samples_needed])
                    wav_io.seek(0)
                    mic_buffer = mic_buffer[samples_needed:]

                    with sr.AudioFile(wav_io) as source:
                        audio = r.record(source)
                    try:
                        text = r.recognize_google(audio).lower()
                        print(f"Heard: {text}")

                        for keyword, url in commands.items():
                            if keyword in text:
                                print(f"Opening {keyword}...")
                                webbrowser.open(url)
                                break
                        else:
                            print(f"You: {text}")
                            await session.send_client_content(
                                turns=types.Content(role="user", parts=[types.Part(text=text)]),
                                turn_complete=True
                            )

                            async for response in session.receive():
                                if response.server_content:
                                    if response.server_content.model_turn:
                                        for part in response.server_content.model_turn.parts:
                                            if hasattr(part, 'inline_data') and part.inline_data:
                                                pcm = part.inline_data.data
                                                for i in range(0, len(pcm), 1024):
                                                    await ws.send(pcm[i:i+1024])
                                    if response.server_content.turn_complete:
                                        await ws.send(json.dumps({"type": "end"}))
                                        print("Done.")
                                        break

                    except sr.UnknownValueError:
                        pass
                    except sr.RequestError as e:
                        print("STT error:", e)

                await asyncio.sleep(0.01)

if __name__ == "__main__":
    asyncio.run(main())
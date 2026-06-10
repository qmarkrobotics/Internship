import asyncio
import websockets
import json
import io
import wave
from google import genai
from google.genai import types

client = genai.Client(api_key="API", http_options={"api_version": "v1beta"})

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

ESP32_WS = "ws://10.247.234.83:8765"

def pcm_to_wav(pcm_bytes, sample_rate=24000, channels=1, sample_width=2):
    buf = io.BytesIO()
    with wave.open(buf, 'wb') as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(sample_width)
        wf.setframerate(sample_rate)
        wf.writeframes(pcm_bytes)
    buf.seek(0)
    return buf.read()

async def main():
    async with client.aio.live.connect(model=MODEL, config=CONFIG) as session:
        async with websockets.connect(ESP32_WS) as ws:
            print("Connected to Gemini and ESP32\n")
            while True:
                text = input("You: ").strip()
                if text.lower() == "quit":
                    break
                if not text:
                    continue

                await session.send_client_content(
                    turns=types.Content(role="user", parts=[types.Part(text=text)]),
                    turn_complete=True
                )

                response_audio = bytearray()
                async for response in session.receive():
                    if response.server_content:
                        if response.server_content.model_turn:
                            for part in response.server_content.model_turn.parts:
                                if hasattr(part, 'inline_data') and part.inline_data:
                                    response_audio.extend(part.inline_data.data)
                        if response.server_content.turn_complete:
                            break

                if response_audio:
                    wav = pcm_to_wav(bytes(response_audio))
                    CHUNK = 8192
                    for i in range(0, len(wav), CHUNK):
                        await ws.send(wav[i:i+CHUNK])
                    await ws.send(json.dumps({"type": "end"}))
                    print("Audio sent to ESP32.")

if __name__ == "__main__":
    asyncio.run(main())
import speech_recognition as sr
import webbrowser

commands = {
    "youtube": "https://www.youtube.com",
    "google":  "https://www.google.com",
    "github":  "https://www.github.com",
    "netflix": "https://www.netflix.com",
}

r = sr.Recognizer()

print("Listening... say a command")

with sr.Microphone() as source:
    r.adjust_for_ambient_noise(source)
    while True:
        try:
            audio = r.listen(source)
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
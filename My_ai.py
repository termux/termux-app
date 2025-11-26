from transformers import pipeline

# Load a small AI model
ai = pipeline("text-generation", model="distilgpt2")

print("Your AI is now running! Type 'exit' to stop.")

while True:
    user = input("You: ")
    if user.lower() == "exit":
        break

    response = ai(user, max_length=100, do_sample=True, top_p=0.95)[0]["generated_text"]
    print("\nAI:", response)
    print()

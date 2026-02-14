import telebot
import os
from flask import Flask, request

TOKEN = os.environ.get("TOKEN")
if not TOKEN:
    raise ValueError("Переменная окружения TOKEN не установлена!")

bot = telebot.TeleBot(TOKEN)
ALLOWED_EXTENSIONS = ["py", "java", "c", "cpp", "txt"]
user_data = {}

app = Flask(__name__)

@bot.message_handler(commands=['start'])
def start(message):
    bot.send_message(
        message.chat.id,
        "Привет! Отправь имя файла с расширением (например: example.py), "
        "а затем отправляй содержимое. Можно отправлять несколько сообщений, они объединятся в один файл."
    )

@bot.message_handler(func=lambda message: True)
def handle_message(message):
    chat_id = message.chat.id
    text = message.text

    if chat_id not in user_data:
        if '.' not in text:
            bot.send_message(chat_id, "Неверный формат имени файла. Пример: example.py")
            return
        name, ext = text.rsplit('.', 1)
        if ext not in ALLOWED_EXTENSIONS:
            bot.send_message(chat_id, f"Допустимые форматы: {', '.join(ALLOWED_EXTENSIONS)}")
            return
        user_data[chat_id] = {"file_name": text, "content": ""}
        bot.send_message(chat_id, f"Имя файла принято: {text}\nТеперь отправляй содержимое файла.")
        return

    user_data[chat_id]["content"] += text + "\n"

    if text.lower() in ["готово", "/end"]:
        file_name = user_data[chat_id]["file_name"]
        content = user_data[chat_id]["content"]
        with open(file_name, 'w', encoding='utf-8') as f:
            f.write(content)
        with open(file_name, 'rb') as f:
            bot.send_document(chat_id, f)
        bot.send_message(chat_id, "Файл отправлен! Можно создавать новый файл.")
        del user_data[chat_id]
    else:
        bot.send_message(chat_id, "Текст добавлен в файл. Отправь ещё или напиши 'Готово' для завершения.")

@app.route(f"/{TOKEN}", methods=["POST"])
def webhook():
    json_string = request.get_data().decode("utf-8")
    update = telebot.types.Update.de_json(json_string)
    bot.process_new_updates([update])
    return "!", 200

@app.route("/")
def index():
    return "Bot is running", 200

if __name__ == "__main__":
    bot.remove_webhook()
    # Установи URL твоего Railway проекта
    WEBHOOK_URL = f"https://fc91794f-f62f-498c-a774-d7d5df67f4ca.up.railway.app/{TOKEN}"
    bot.set_webhook(url=WEBHOOK_URL)
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", 5000)))

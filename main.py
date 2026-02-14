import telebot
import os

Railway
TOKEN = os.environ.get("TOKEN")
if not TOKEN:
    raise ValueError("Переменная окружения TOKEN не установлена!")

bot = telebot.TeleBot(TOKEN)

ALLOWED_EXTENSIONS = ["py", "java", "c", "cpp", "txt"]

user_data = {}

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

        user_data[chat_id] = {
            "file_name": text,
            "content": ""
        }
        bot.send_message(chat_id, f"Имя файла принято: {text}\nТеперь отправляй содержимое файла. "
                                  "Можно разделить на несколько сообщений, они объединятся.")
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

bot.polling(none_stop=True)

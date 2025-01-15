from telegram import Update
from telegram.ext import Updater, CommandHandler, CallbackContext

# ضع التوكن الخاص بك هنا
TOKEN = "7633245579:AAGuANfTg2GjLxXdyDm8XKTlFwVQ-DmjUJs"

# دالة الترحيب عند بدء البوت
def start(update: Update, context: CallbackContext) -> None:
    user = update.effective_user
    update.message.reply_text(
        f"أهلاً {user.first_name}!\nأنا بوت مساعدك الشخصي! 🚀\nاستخدم الأوامر للتفاعل معي."
    )

# دالة للرد على الأمر "help"
def help_command(update: Update, context: CallbackContext) -> None:
    update.message.reply_text(
        "🤖 قائمة الأوامر:\n"
        "/start - بدء المحادثة مع البوت\n"
        "/help - عرض قائمة الأوامر"
    )

# إعداد البوت وتشغيله
def main():
    # إعداد البوت باستخدام التوكن
    updater = Updater(TOKEN)

    # تسجيل الأوامر مع الدوال الخاصة بها
    dispatcher = updater.dispatcher
    dispatcher.add_handler(CommandHandler("start", start))
    dispatcher.add_handler(CommandHandler("help", help_command))

    # بدء تشغيل البوت
    print("🤖 البوت يعمل الآن...")
    updater.start_polling()
    updater.idle()

if __name__ == "__main__":
    main()
    

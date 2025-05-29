import os
import time
import logging
from aiogram import Bot, Dispatcher, types
from aiogram.types import ReplyKeyboardMarkup
from aiogram.utils import executor
from aiogram.contrib.fsm_storage.memory import MemoryStorage
import openai

# Environment Variables (deta secrets orqali saqlaysiz)
BOT_TOKEN = os.getenv("7663550436:AAF_PPXQepgz5sxldSmhWOauBb1kvyHbguo")
OPENAI_API_KEY = os.getenv("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")
ADMIN_USERNAME = os.getenv("@Whale_br0")

bot = Bot(token=BOT_TOKEN)
dp = Dispatcher(bot, storage=MemoryStorage())
openai.api_key = OPENAI_API_KEY

# In-memory database (oddiy dictionary)
users_db = {}

logging.basicConfig(level=logging.INFO)

# Asosiy menyu
def main_menu():
    keyboard = ReplyKeyboardMarkup(resize_keyboard=True)
    keyboard.add("📨 Do‘st taklif qilish", "💰 Balans")
    keyboard.add("📥 Slayd yaratish", "📂 Mening slaydlarim")
    keyboard.add("💳 Hisobni to‘ldirish", "⚙️ Admin Panel")
    return keyboard

# Foydalanuvchi qo'shish
def add_user(user_id):
    if user_id not in users_db:
        users_db[user_id] = {
            "balance": 6000,    # har bir yangi foydalanuvchiga 6000 so'm bonus
            "slides": [],
            "free_slides": 2
        }

def get_balance(user_id):
    return users_db.get(user_id, {}).get("balance", 0)

def add_balance(user_id, amount):
    if user_id in users_db:
        users_db[user_id]["balance"] += amount

def use_free_slide(user_id):
    if users_db[user_id]["free_slides"] > 0:
        users_db[user_id]["free_slides"] -= 1
        return True
    return False

def get_slides(user_id):
    return users_db[user_id]["slides"]

def save_slide(user_id, file_path):
    users_db[user_id]["slides"].append(file_path)

@dp.message_handler(commands=['start'])
async def start(message: types.Message):
    user_id = message.from_user.id
    args = message.get_args()
    if user_id not in users_db:
        add_user(user_id)
    if args.isdigit():
        ref_id = int(args)
        if ref_id != user_id and ref_id in users_db:
            add_balance(ref_id, 1000)  # Do'st taklif bonus
    await message.answer(
        "👋 Assalomu alaykum!\n\n"
        "Men sizga tez va oson, sifatli PowerPoint slaydlar tayyorlashda yordam beraman.\n\n"
        "📍 Har bir slayd: 3000 so'm (2 ta slayd bepul)\n"
        "🧑‍🤝‍🧑 Har bir do‘st uchun: 1000 so‘m bonus\n"
        "Boshlash uchun pastdagi tugmalardan foydalaning 👇",
        reply_markup=main_menu()
    )

@dp.message_handler(lambda msg: msg.text == "📨 Do‘st taklif qilish")
async def invite(message: types.Message):
    user_id = message.from_user.id
    link = f"https://t.me/your_bot_username?start={user_id}"  # Bot username ni o'zgartiring
    await message.answer(f"👥 Do‘stlaringizni quyidagi havola orqali taklif qiling:\n\n{link}\n\n💸 Har bir do‘st ro‘yxatdan o‘tsa, sizga 1000 so'm bonus beriladi!")

@dp.message_handler(lambda msg: msg.text == "💰 Balans")
async def balance(message: types.Message):
    user_id = message.from_user.id
    bal = get_balance(user_id)
    free = users_db[user_id]['free_slides']
    await message.answer(f"💳 Balans: {bal} so'm\n🎁 Bepul slaydlar: {free} ta")

@dp.message_handler(lambda msg: msg.text == "💳 Hisobni to‘ldirish")
async def deposit_info(message: types.Message):
    await message.answer(
        "💵 Hisobingizni to‘ldirish uchun quyidagi kartaga pul o‘tkazing:\n\n"
        "🏦 Humo karta: 9860 1901 1077 5664\n"
        "👤 Egasi: RAMSHID RUZIYEV\n\n"
        f"✅ To‘lov qilganingizdan so‘ng, chekni adminga yuboring.\n"
        f"📞 Admin: {ADMIN_USERNAME}"
    )

@dp.message_handler(lambda msg: msg.text == "📂 Mening slaydlarim")
async def my_slides(message: types.Message):
    user_id = message.from_user.id
    slides = get_slides(user_id)
    if not slides:
        return await message.answer("📂 Siz hali hech qanday slayd yaratmagansiz.")
    await message.answer("📁 Mana siz yaratgan slaydlar:")
    for path in slides:
        if os.path.exists(path):
            with open(path, "rb") as f:
                await bot.send_document(message.chat.id, f)

@dp.message_handler(lambda msg: msg.text == "📥 Slayd yaratish")
async def ask_slide_topic(message: types.Message):
    await message.answer("📌 Iltimos, slayd yaratish uchun mavzuni yozing:")
    dp.register_message_handler(ask_slide_pages, content_types=types.ContentTypes.TEXT, state=None)

async def ask_slide_pages(message: types.Message):
    topic = message.text.strip()
    message.conf = {}
    message.conf['topic'] = topic
    await message.answer("📄 Nechta betli slayd tayyorlaylik?")
    dp.register_message_handler(generate_slide, content_types=types.ContentTypes.TEXT, state=None)

async def generate_slide(message: types.Message):
    user_id = message.from_user.id
    topic = message.conf.get('topic', 'No topic')
    try:
        pages = int(message.text.strip())
    except:
        return await message.answer("❌ Iltimos, raqam kiriting (nechta betli slayd kerak?)")

    if use_free_slide(user_id):
        cost = 0
    else:
        if get_balance(user_id) < 3000:
            return await message.answer("❌ Sizda yetarli balans yo‘q. Har bir slayd 3000 so‘m turadi.")
        add_balance(user_id, -3000)
        cost = 3000

    await message.answer("🤖 Slayd tayyorlanmoqda, biroz kuting...")
    text = f"Mavzu: {topic}\nBetlar soni: {pages}\n\n" + "\n\n".join([f"{i+1}. {topic} haqida mazmunli ma'lumot" for i in range(pages)])
    folder = f"user_files/{user_id}/"
    os.makedirs(folder, exist_ok=True)
    filename = f"{folder}{user_id}_{int(time.time())}.txt"
    with open(filename, "w", encoding="utf-8") as f:
        f.write(text)
    with open(filename, "rb") as doc:
        await bot.send_document(message.chat.id, doc)
    save_slide(user_id, filename)
    await message.answer(f"✅ Slayd tayyorlandi! {'(Bepul)' if cost == 0 else '-3000 so‘m yechildi.'}")

@dp.message_handler(lambda msg: msg.text == "⚙️ Admin Panel")
async def admin_panel(message: types.Message):
    if message.from_user.username != ADMIN_USERNAME.replace("@", ""):
        return await message.answer("❌ Bu bo‘lim faqat admin uchun.")

    panel = (
        "🔧 Admin Panel:\n"
        "/userlist - Foydalanuvchilar ro‘yxati\n"
        "/add <user_id> <miqdor> - Balans qo‘shish\n"
        "/setfree <user_id> <soni> - Bepul slaydlar belgilash"
    )
    await message.answer(panel)

@dp.message_handler(commands=['userlist'])
async def user_list(message: types.Message):
    if message.from_user.username != ADMIN_USERNAME.replace("@", ""):
        return
    text = "👥 Foydalanuvchilar:\n" + "\n".join(
        [f"{uid} - Balans: {data['balance']} - Free: {data['free_slides']}" for uid, data in users_db.items()])
    await message.answer(text)

@dp.message_handler(commands=['add'])
async def add_money(message: types.Message):
    if message.from_user.username != ADMIN_USERNAME.replace("@", ""):
        return
    args = message.text.split()
    if len(args) == 3:
        user_id = int(args[1])
        amount = int(args[2])
        add_balance(user_id, amount)
        await message.answer(f"💰 {user_id} foydalanuvchiga {amount} so‘m qo‘shildi.")

@dp.message_handler(commands=['setfree'])
async def set_free(message: types.Message):
    if message.from_user.username != ADMIN_USERNAME.replace("@", ""):
        return
    args = message.text.split()
    if len(args) == 3:
        user_id = int(args[1])
        amount = int(args[2])
        users_db[user_id]['free_slides'] = amount
        await message.answer(f"🎁 {user_id} foydalanuvchiga {amount} ta bepul slayd belgilandi.")

if __name__ == '__main__':
    executor.start_polling(dp, skip_updates=True)

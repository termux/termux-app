# bot.py
import os
import json
import asyncio
import re
import logging
from typing import Dict, Any

from aiogram import Bot, Dispatcher, F
from aiogram.filters import Command
from aiogram.types import Message

from telethon import TelegramClient, events
from telethon.errors import SessionPasswordNeededError, FloodWaitError, PhoneCodeExpiredError
from telethon.tl.functions.channels import JoinChannelRequest

# ================== SOZLAMALAR ==================
BOT_TOKEN = "8410771294:AAHgNRhT3llwERiukidVh9Ufj_GOfu2Bls8"  # o'zingiz tokenni shu yerga qo'ying
ACCOUNTS_FILE = "accounts.json"
# Patrick bot username
PATRICK = "@patrickstarsrobot"

# meva tarjimasi
FRUITS = {
    'Виноград': '🍇', 'Клубника': '🍓', 'Арбуз': '🍉', 'Ананас': '🍍',
    'Помидор': '🍅', 'Кокос': '🥥', 'Банан': '🍌', 'Яблоко': '🍎',
    'Персик': '🍑', 'Вишня': '🍒',
}

# ================== LOG ==================
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

# ================== GLOBAL ==================
bot = Bot(BOT_TOKEN)
dp = Dispatcher()

# pending holatlar: chat_id -> state dict
pending: Dict[int, Dict[str, Any]] = {}
# ishlayotgan telethon clientlar: phone -> client
running_clients: Dict[str, TelegramClient] = {}


# ================== Helper fayl funksiyalari ==================
def ensure_accounts_file():
    if not os.path.exists(ACCOUNTS_FILE):
        with open(ACCOUNTS_FILE, "w") as f:
            json.dump([], f)


def load_accounts():
    ensure_accounts_file()
    with open(ACCOUNTS_FILE, "r") as f:
        try:
            return json.load(f)
        except json.JSONDecodeError:
            return []


def save_accounts(accounts):
    with open(ACCOUNTS_FILE, "w") as f:
        json.dump(accounts, f, indent=4)


def session_name(phone: str) -> str:
    return f"session_{phone.replace('+','').strip()}"


# ================== Clicker / userbot vazifalari ==================
async def run_clicker(client: TelegramClient, phone: str):
    """
    Patron bot bilan ishlovchi fon vazifa: /start -> Кликер tugmasini bosadi,
    obuna bo'ladi va meva captchani tanlaydi. Doimiy ishlaydi.
    """
    log.info(f"[{phone}] clicker ishga tushdi")
    # event handlerlar
    @client.on(events.NewMessage(from_users=PATRICK))
    async def handler(event):
        try:
            msg = event.message
            if not msg:
                return

            # obuna bloki
            if msg.buttons and 'Подписаться' in (msg.raw_text or ""):
                try:
                    # top Sponsor knopkalar
                    sponsor_buttons = [b for row in msg.buttons for b in row if 'Спонсор' in (b.text or "")]
                    for b in sponsor_buttons:
                        if getattr(b, "url", None):
                            try:
                                entity = await client.get_entity(b.url)
                                await client(JoinChannelRequest(entity))
                                log.info(f"[{phone}] Obuna qilindi: {getattr(entity, 'username', getattr(entity, 'title', 'chan'))}")
                            except Exception as e:
                                log.warning(f"[{phone}] Obuna xato: {e}")
                except Exception as e:
                    log.warning(f"[{phone}] Obuna blokida xato: {e}")

                # Проверить tugmasi
                try:
                    for row in msg.buttons:
                        for btn in row:
                            if 'Проверить' in (btn.text or ""):
                                await asyncio.sleep(2)
                                await event.click(text=btn.text)
                                log.info(f"[{phone}] Проверить bosildi")
                                return
                except Exception as e:
                    log.warning(f"[{phone}] Проверить bosilish xato: {e}")

            # meva captcha
            try:
                if "награду" in (msg.message or ""):
                    m = re.search(r'где изображено «(.+?)»', msg.message)
                    if m:
                        fruit = m.group(1).strip()
                        emoji = FRUITS.get(fruit)
                        if emoji and msg.buttons:
                            for row in msg.buttons:
                                for btn in row:
                                    if emoji in (btn.text or ""):
                                        await asyncio.sleep(1.5)
                                        await event.click(text=btn.text)
                                        log.info(f"[{phone}] Captcha yechildi: {fruit} -> {emoji}")
                                        return
            except Exception as e:
                log.warning(f"[{phone}] Captcha xato: {e}")

        except Exception as e:
            log.exception(f"[{phone}] handler umumiy xato: {e}")

    # asosiy doimiy sikl — /start va Кликерni bosish
    while True:
        try:
            await client.send_message(PATRICK, "/start")
            await asyncio.sleep(2)
            async for m in client.iter_messages(PATRICK, limit=8):
                if m and m.buttons:
                    clicked = False
                    for row in m.buttons:
                        for btn in row:
                            txt = btn.text or ""
                            if "Кликер" in txt:
                                try:
                                    await m.click(text=btn.text)
                                    log.info(f"[{phone}] Кликер bosildi")
                                    clicked = True
                                    break
                                except Exception as e:
                                    log.warning(f"[{phone}] Кликer click xato: {e}")
                        if clicked:
                            break
                    if clicked:
                        break
        except FloodWaitError as e:
            delay = int(getattr(e, "seconds", 60))
            log.warning(f"[{phone}] FloodWait {delay}s")
            await asyncio.sleep(delay)
        except Exception as e:
            log.exception(f"[{phone}] start/clicker xato: {e}")

        await asyncio.sleep(361)


async def start_userbot_after_auth(api_id: int, api_hash: str, phone: str, client: TelegramClient = None):
    """
    Client allaqachon authorised bo'lsa yoki sign_in qilingan bo'lsa shu funksiya orqali
    running_clients ga qo'shib fon klikerni ishga tushiramiz.
    """
    sess = session_name(phone)
    if client is None:
        client = TelegramClient(sess, api_id, api_hash)
        await client.connect()
    running_clients[phone] = client
    log.info(f"[{phone}] userbot ishga tushmoqda...")
    asyncio.create_task(run_clicker(client, phone))


# ================== Aiogram bot oqimi (state) ==================
@dp.message(Command("start"))
async def cmd_start(message: Message):
    pending[message.chat.id] = {"step": "phone"}
    await message.answer("👋 Salom! Telefon raqamingizni yuboring (+998...)")

@dp.message(F.text & (F.chat.id.in_(lambda: pending.keys())))
async def conversation(message: Message):
    uid = message.chat.id
    state = pending.get(uid)
    if not state:
        return

    step = state.get("step")

    # 1) phone
    if step == "phone":
        phone = message.text.strip()
        if not (phone.startswith("+") and len(phone) > 7):
            await message.answer("❌ Telefon noto‘g‘ri format. +998901234567 tarzida yuboring.")
            return
        state["phone"] = phone
        state["step"] = "api_id"
        await message.answer("🔢 Endi API_ID ni yuboring (raqam):")
        return

    # 2) api_id
    if step == "api_id":
        if not message.text.strip().isdigit():
            await message.answer("❌ API_ID raqam bo‘lishi kerak. Qayta yuboring:")
            return
        state["api_id"] = int(message.text.strip())
        state["step"] = "api_hash"
        await message.answer("🔑 Endi API_HASH ni yuboring:")
        return

    # 3) api_hash -> send_code_request
    if step == "api_hash":
        api_hash = message.text.strip()
        state["api_hash"] = api_hash
        phone = state["phone"]
        api_id = state["api_id"]
        sess = session_name(phone)
        client = TelegramClient(sess, api_id, api_hash)
        try:
            await client.connect()
        except Exception as e:
            await message.answer(f"❌ Telethon connect xatosi: {e}")
            pending.pop(uid, None)
            return

        try:
            # agar allaqachon authorised bo'lsa
            if await client.is_user_authorized():
                await message.answer("✅ Bu telefon allaqachon avtorizatsiyalangan. Kliker ishga tushadi.")
                accounts = load_accounts()
                if not any(a.get("phone") == phone for a in accounts):
                    accounts.append({"phone": phone, "api_id": api_id, "api_hash": api_hash, "session": sess})
                    save_accounts(accounts)
                pending.pop(uid, None)
                await start_userbot_after_auth(api_id, api_hash, phone, client)
                return

            # yuboramiz
            await client.send_code_request(phone)
            # saqlaymiz: clientni ham
            state["client"] = client
            state["step"] = "code"
            await message.answer("📩 Kod yuborildi. Iltimos telegram/sms orqali kelgan kodni yuboring.\nAgar kod muddati o'tsa, /resend buyrug‘i bilan qayta yuboring.")
        except FloodWaitError as e:
            delay = int(getattr(e, "seconds", 60))
            await message.answer(f"⏳ Iltimos {delay} soniya kuting (FloodWait).")
            try:
                await client.disconnect()
            except:
                pass
            pending.pop(uid, None)
        except Exception as e:
            await message.answer(f"❌ Kod yuborishda xato: {e}")
            try:
                await client.disconnect()
            except:
                pass
            pending.pop(uid, None)
        return

    # 4) code
    if step == "code":
        code = message.text.strip()
        if not code.isdigit():
            await message.answer("❌ Kod faqat raqamlardan iborat bo‘lishi kerak.")
            return
        client: TelegramClient = state.get("client")
        phone = state.get("phone")
        api_id = state.get("api_id")
        api_hash = state.get("api_hash")
        if client is None:
            await message.answer("❌ Ichki xato: client topilmadi. /start bilan qayta boshlang.")
            pending.pop(uid, None)
            return

        try:
            # Urinish: sign_in
            await client.sign_in(phone=phone, code=code)
            # agar muvaffaqiyatli kontol
            if not await client.is_user_authorized():
                await message.answer("❌ Avtorizatsiya yakunlanmadi. Qayta urinib ko‘ring.")
                await client.disconnect()
                pending.pop(uid, None)
                return

            # muvaffaqiyat — saqlash va kliker ishga tushurish
            sess = session_name(phone)
            accounts = load_accounts()
            if not any(a.get("phone") == phone for a in accounts):
                accounts.append({"phone": phone, "api_id": api_id, "api_hash": api_hash, "session": sess})
                save_accounts(accounts)

            running_clients[phone] = client
            asyncio.create_task(run_clicker(client, phone))
            await message.answer("🎉 Akkaunt muvaffaqiyatli ulanib, kliker ishga tushdi!")
            pending.pop(uid, None)
            return

        except SessionPasswordNeededError:
            state["step"] = "2fa"
            await message.answer("🔒 Bu akkauntda 2FA yoqilgan. Iltimos, 2FA parolni yuboring:")
            return
        except PhoneCodeExpiredError:
            # kod muddati o'tgan
            await message.answer("❌ Kod muddati o'tgan. /resend bilan kodni qayta yuboring yoki /start bilan qayta boshlang.")
            try:
                await client.disconnect()
            except:
                pass
            pending.pop(uid, None)
            return
        except FloodWaitError as e:
            delay = int(getattr(e, "seconds", 60))
            await message.answer(f"⏳ FloodWait {delay}s. Keyin urinib ko‘ring.")
            try:
                await client.disconnect()
            except:
                pass
            pending.pop(uid, None)
            return
        except Exception as e:
            await message.answer(f"❌ Avtorizatsiyada xato: {e}\nQayta boshlash uchun /start yuboring.")
            try:
                await client.disconnect()
            except:
                pass
            pending.pop(uid, None)
            return

    # 5) 2FA parol
    if step == "2fa":
        password = message.text.strip()
        client: TelegramClient = state.get("client")
        phone = state.get("phone")
        api_id = state.get("api_id")
        api_hash = state.get("api_hash")
        if not client:
            await message.answer("❌ Ichki xato: client topilmadi. /start bilan qayta boshlang.")
            pending.pop(uid, None)
            return
        try:
            await client.sign_in(password=password)
            # saqlash va kliker ishga tushirish
            sess = session_name(phone)
            accounts = load_accounts()
            if not any(a.get("phone") == phone for a in accounts):
                accounts.append({"phone": phone, "api_id": api_id, "api_hash": api_hash, "session": sess})
                save_accounts(accounts)

            running_clients[phone] = client
            asyncio.create_task(run_clicker(client, phone))
            await message.answer("🎉 2FA muvaffaqiyatli! Akaunt ulandi va kliker ishga tushdi.")
            pending.pop(uid, None)
            return
        except Exception as e:
            await message.answer(f"❌ 2FA parol noto‘g‘ri yoki boshqa xato: {e}\n/start bilan qayta urinib ko‘ring.")
            try:
                await client.disconnect()
            except:
                pass
            pending.pop(uid, None)
            return


# ========== /resend handler (kod muddati o‘tganida qayta yuborish) ==========
@dp.message(Command("resend"))
async def cmd_resend(message: Message):
    uid = message.chat.id
    state = pending.get(uid)
    if not state or "client" not in state:
        await message.answer("❌ Hozir yuborilgan kodni qayta yuborish uchun hech qanday jarayon yo‘q. /start bilan boshlang.")
        return
    client: TelegramClient = state["client"]
    phone = state.get("phone")
    try:
        await client.send_code_request(phone)
        await message.answer("✅ Kod qayta yuborildi. Iltimos tezroq yuboring (1-2 daqiqa ichida).")
    except Exception as e:
        await message.answer(f"❌ Kod yuborishda xato: {e}")


# ========== Resume eski akkauntlarni tiklash ==========
async def resume_accounts_on_start():
    accounts = load_accounts()
    if not accounts:
        return
    for acc in accounts:
        phone = acc.get("phone")
        api_id = acc.get("api_id")
        api_hash = acc.get("api_hash")
        sess = acc.get("session") or session_name(phone)
        client = TelegramClient(sess, api_id, api_hash)
        try:
            await client.connect()
            if await client.is_user_authorized():
                running_clients[phone] = client
                asyncio.create_task(run_clicker(client, phone))
                log.info(f"[{phone}] eski sessiya tiklandi va kliker ishga tushdi")
            else:
                await client.disconnect()
        except Exception as e:
            log.warning(f"[{phone}] eski sessiyani tiklashda xato: {e}")


# ========== MAIN ==========
async def main():
    log.info("Bot ishga tushdi...")
    await resume_accounts_on_start()
    await dp.start_polling(bot)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        log.info("To‘xtatildi")

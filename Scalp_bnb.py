pkg install python
pip install ccxt
pip install numpy
import ccxt
import time
import numpy as np

# SETARI BOT
symbol = "BNB/USDT"
order_size = 0.03          # Cantitatea cumparata/vanduta
scalp_percent = 0.001      # 0.1% miscari necesare
cooldown = 2               # secunde intre verificari

# CONECTARE BINANCE
exchange = ccxt.binance({
    "apiKey": "API_KEY",
    "secret": "API_SECRET"
})

# Functie pentru pret curent
def get_price():
    return exchange.fetch_ticker(symbol)["last"]

# Pret initial
last_price = get_price()

print("Bot pornit pentru BNB Scalping!")

while True:
    try:
        price = get_price()

        change = (price - last_price) / last_price

        # CUMPARA cand pretul scade brusc (scalping)
        if change <= -scalp_percent:
            print(f"↓ Scade rapid -> CUMPARA la {price}")
            # exchange.create_market_buy_order(symbol, order_size)
            last_price = price

        # VINDE cand pretul urca rapid
        elif change >= scalp_percent:
            print(f"↑ Creste rapid -> VINDE la {price}")
            # exchange.create_market_sell_order(symbol, order_size)
            last_price = price

        time.sleep(cooldown)

    except Exception as e:
        print("Eroare:", e)
        time.sleep(3)

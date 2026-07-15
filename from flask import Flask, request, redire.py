from flask import Flask, request, redirect, url_for, session
from datetime import datetime
import secrets

app = Flask(__name__)
app.secret_key = secrets.token_hex(16) # For login session

# "Database" - all users stored here
users = {
        "09011112222": {"name": "Aisha", "pin": "1234", "balance": 100000, "history": []},
            "09022223333": {"name": "Tunde", "pin": "1234", "balance": 50000, "history": []}
}
TAX = 200 # Tax to pay before withdrawal

def page(title, content):
    return f"""<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>
        <style>body{{background:#00B14F;font-family:Arial;margin:0;padding:20px}}
           .card{{background:white;padding:20px;border-radius:15px;max-width:420px;margin:20px auto;box-shadow:0 4px 12px rgba(0,0,0,0.2)}}
              .btn{{background:#00B14F;color:white;padding:14px;border:none;border-radius:10px;width:100%;font-size:16px;font-weight:bold;margin-top:10px;cursor:pointer}}
                  input,select{{width:93%;padding:12px;margin:8px 0;border:1px solid #ddd;border-radius:10px;font-size:15px}}
                     .balance{{font-size:40px;font-weight:bold;color:#00B14F;text-align:center;margin:10px 0}}
                        .history{{font-size:14px;border-bottom:1px solid #eee;padding:8px 0}}
                            h2{{color:#00B14F;text-align:center}} a{{color:#00B14F;text-decoration:none}}</style></head>
                                <body><div class='card'><h2>{title}</h2>{content}</div></body></html>"""

                                def add_history(phone, text):
                                    users[phone]["history"].insert(0, f"{datetime.now().strftime('%d %b %I:%M %p')} - {text}")

                                    def check_login():
                                        if 'phone' not in session: return redirect('/')
                                            return None

                                            @app.route('/')
                                            def login():
                                                return page("OPAY CLONE", """
                                                    <form method='post' action='/login'>
                                                        <input name='phone' placeholder='Phone Number e.g 09011112222' required>
                                                            <input name='pin' type='password' placeholder='4-digit PIN' required>
                                                                <button class='btn'>Login</button></form>
                                                                    <p style='text-align:center;font-size:12px'>Test: 09011112222 / 1234</p>
                                                                        """)

                                                                        @app.route('/login', methods=['POST'])
                                                                        def do_login():
                                                                            phone = request.form['phone']; pin = request.form['pin']
                                                                                if phone in users and users[phone]['pin'] == pin:
                                                                                        session['phone'] = phone; return redirect('/home')
                                                                                            return page("Login Failed", "Wrong phone or PIN<br><a href='/'>Try Again</a>")

                                                                                            @app.route('/home')
                                                                                            def home():
                                                                                                if check_login(): return check_login()
                                                                                                    u = users[session['phone']]
                                                                                                        return page("Dashboard", f"""
                                                                                                            <p>Hi <b>{u['name']}</b> 👋</p>
                                                                                                                <p>Available Balance</p><div class='balance'>₦{u['balance']:,}</div>
                                                                                                                    <a href='/transfer'><button class='btn'>Transfer Money</button></a>
                                                                                                                        <a href='/withdraw'><button class='btn'>Withdraw</button></a>
                                                                                                                            <a href='/bills'><button class='btn'>Pay Bills: Sporty, Data, Airtime</button></a>
                                                                                                                                <a href='/history'><button class='btn'>Transaction History</button></a>
                                                                                                                                    <a href='/logout'><button class='btn' style='background:#ccc;color:black'>Logout</button></a>
                                                                                                                                        """)

                                                                                                                                        @app.route('/transfer', methods=['GET','POST'])
                                                                                                                                        def transfer():
                                                                                                                                            if check_login(): return check_login()
                                                                                                                                                if request.method=='POST':
                                                                                                                                                        to=request.form['to']; amt=int(request.form['amount'])
                                                                                                                                                                me=session['phone']
                                                                                                                                                                        if to not in users: return page("Error","Recipient not found")
                                                                                                                                                                                if users[me]['balance']<amt: return page("Error","Insufficient Balance")
                                                                                                                                                                                        users[me]['balance']-=amt; users[to]['balance']+=amt
                                                                                                                                                                                                add_history(me,f"Sent ₦{amt:,} to {users[to]['name']}")
                                                                                                                                                                                                        add_history(to,f"Received ₦{amt:,} from {users[me]['name']}")
                                                                                                                                                                                                                return page("Transfer Successful",f"Sent ₦{amt:,} to {users[to]['name']}<br><a href='/home'>Back Home</a>")
                                                                                                                                                                                                                    return page("Transfer", "<form method='post'><input name='to' placeholder='Recipient Phone'><input name='amount' type='number' placeholder='Amount'><button class='btn'>Send Now</button></form>")

                                                                                                                                                                                                                    @app.route('/withdraw', methods=['GET','POST'])
                                                                                                                                                                                                                    def withdraw():
                                                                                                                                                                                                                        if check_login(): return check_login()
                                                                                                                                                                                                                            me=session['phone']
                                                                                                                                                                                                                                if request.method=='POST':
                                                                                                                                                                                                                                        amt=int(request.form['amount']); total=amt+TAX
                                                                                                                                                                                                                                                if users[me]['balance']<total: return page("Error",f"Balance too low. You need ₦{total:,}")
                                                                                                                                                                                                                                                        users[me]['balance']-=total
                                                                                                                                                                                                                                                                add_history(me,f"Withdrawal ₦{amt:,} | Tax ₦{TAX}")
                                                                                                                                                                                                                                                                        return page("Withdrawal Request",f"₦{amt:,} withdrawal successful<br>Tax ₦{TAX} deducted<br><a href='/home'>Back Home</a>")
                                                                                                                                                                                                                                                                            return page("Withdraw Money", f"<p style='color:red;font-size:13px'>Note: ₦{TAX} withdrawal tax will be deducted</p><form method='post'><input name='amount' type='number' placeholder='Amount to Withdraw'><button class='btn'>Withdraw</button></form>")

                                                                                                                                                                                                                                                                            @app.route('/bills', methods=['GET','POST'])
                                                                                                                                                                                                                                                                            def bills():
                                                                                                                                                                                                                                                                                if check_login(): return check_login()
                                                                                                                                                                                                                                                                                    me=session['phone']
                                                                                                                                                                                                                                                                                        if request.method=='POST':
                                                                                                                                                                                                                                                                                                bill=request.form['bill']; amt=int(request.form['amount'])
                                                                                                                                                                                                                                                                                                        if users[me]['balance']<amt: return page("Error","Insufficient Balance")
                                                                                                                                                                                                                                                                                                                users[me]['balance']-=amt
                                                                                                                                                                                                                                                                                                                        add_history(me,f"Paid ₦{amt:,} for {bill}")
                                                                                                                                                                                                                                                                                                                                return page("Payment Successful",f"Paid ₦{amt:,} for {bill}<br><a href='/home'>Back Home</a>")
                                                                                                                                                                                                                                                                                                                                    return page("Pay Bills", """
                                                                                                                                                                                                                                                                                                                                        <form method='post'><select name='bill'><option>SportyBet</option><option>MSport</option>
                                                                                                                                                                                                                                                                                                                                            <option>Airtime</option><option>Data</option><option>Electricity</option></select>
                                                                                                                                                                                                                                                                                                                                                <input name='amount' type='number' placeholder='Amount'><button class='btn'>Pay Now</button></form>
                                                                                                                                                                                                                                                                                                                                                    """)

                                                                                                                                                                                                                                                                                                                                                    @app.route('/history')
                                                                                                                                                                                                                                                                                                                                                    def history():
                                                                                                                                                                                                                                                                                                                                                        if check_login(): return check_login()
                                                                                                                                                                                                                                                                                                                                                            me=session['phone']
                                                                                                                                                                                                                                                                                                                                                                hist="".join([f"<div class='history'>{h}</div>" for h in users[me]['history']]) or "<p>No transactions yet</p>"
                                                                                                                                                                                                                                                                                                                                                                    return page("Transaction History", hist+"<br><a href='/home'>Back</a>")

                                                                                                                                                                                                                                                                                                                                                                    @app.route('/logout')
                                                                                                                                                                                                                                                                                                                                                                    def logout():
                                                                                                                                                                                                                                                                                                                                                                        session.pop('phone', None); return redirect('/')

                                                                                                                                                                                                                                                                                                                                                                        if __name__=='__main__':
                                                                                                                                                                                                                                                                                                                                                                            app.run(host='0.0.0.0',port=5000,debug=True)
                                                                                                                                                                                                                                                                                                                                                                     
}
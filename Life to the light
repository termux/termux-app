import 'package:flutter/material.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  final double welcomeBonus = 1000000; // $1 million welcome bonus

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Million Dollar App',
      theme: ThemeData(primarySwatch: Colors.green),
      home: Scaffold(
        appBar: AppBar(
          title: Text('Million Dollar Login'),
        ),
        body: Center(
          child: Padding(
            padding: EdgeInsets.all(16.0),
            child: LoginWidget(welcomeBonus: welcomeBonus),
          ),
        ),
      ),
    );
  }
}

class LoginWidget extends StatefulWidget {
  final double welcomeBonus;
  LoginWidget({required this.welcomeBonus});

  @override
  _LoginWidgetState createState() => _LoginWidgetState();
}

class _LoginWidgetState extends State<LoginWidget> {
  final _emailController = TextEditingController();
  bool loggedIn = false;

  void _login() {
    setState(() {
      loggedIn = true;
    });
  }

  @override
  Widget build(BuildContext context) {
    return loggedIn
        ? Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(Icons.attach_money, color: Colors.green, size: 100),
              SizedBox(height: 20),
              Text(
                'Welcome! You just received \$${widget.welcomeBonus.toStringAsFixed(0)}!',
                textAlign: TextAlign.center,
                style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
              ),
            ],
          )
        : Column(
            children: [
              TextField(
                controller: _emailController,
                decoration: InputDecoration(labelText: 'Enter your email'),
              ),
              SizedBox(height: 20),
              ElevatedButton(
                onPressed: _login,
                child: Text('Login to Claim \$1,000,000'),
              ),
            ],
          );
  }
}

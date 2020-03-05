echo "[TEL]: finishing TEL setup"
chmod +x .tel/status.sh

echo "[TEL]: installing required packages:"
pkg install fzf byobu tmux zsh ncurses-utils -y

byobu-enable

echo "[TEL]: installing app launcher:"
git clone https://github.com/Neo-Oli/termux-app-launcher
cd termux-app-launcher
make install
cd ..
rm -rf termux-app-launcher
app -u

echo "[TEL]: installing OhMyTermux:"
git clone https://github.com/anorebel/OhMyTermux.git
cd OhMyTermux
./install.sh
ćd ~
rm -rf OhMyTermux
echo "cat ~/../etc/motd" >> .zshrc
echo "[TEL]: installation finished! Please restart the app!"
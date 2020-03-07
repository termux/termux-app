echo "[TEL]: finishing TEL setup"

echo "[TEL]: moving files"
mv ~/../usr/tel/.byobu ~/
mv ~/../usr/tel/.termux ~/
mv ~/../usr/tel/.tel ~/

chmod +x .tel/status.sh

echo "[TEL]: installing required packages:"
pkg install fzf byobu tmux zsh ncurses-utils git make -y

byobu-enable

echo "[TEL]: installing app launcher:"
mkdir -p ~/.local/share/termux-launcher-app-cache
git clone https://github.com/Neo-Oli/termux-app-launcher
cd termux-app-launcher
make install
cd ..
rm -rf termux-app-launcher
app -u

echo "[TEL]: installing OhMyTermux:"
git clone https://github.com/anorebel/OhMyTermux.git
cd OhMyTermux
bash install.sh
cd ~
rm -rf OhMyTermux
mv ~/../usr/etc/motd_finished ~/../etc/motd
echo "cat ~/../usr/etc/motd" >> .zshrc
sed -i 's/robbyrussell/avit/g' .zshrc
echo "[TEL]: installation finished! Please restart the app!"
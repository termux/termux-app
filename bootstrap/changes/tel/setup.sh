echo "[TEL]: finishing TEL setup"

echo "[TEL]: moving files"
mv ~/../usr/tel/.byobu ~/
mv ~/../usr/tel/.termux ~/
mv ~/../usr/tel/.tel ~/

chmod +x .tel/status.sh

echo "[TEL]: installing required packages:"
pkg install fzf byobu curl wget nano tmux zsh ncurses-utils git make -y

echo "[TEL]: installing app launcher:"
mkdir -p ~/.local/share/
git clone https://github.com/Neo-Oli/termux-app-launcher
cd termux-app-launcher
make install
cd ..
rm -rf termux-app-launcher
app -u

echo "[TEL]: installing OhMyZsh"
echo "if you enable zsh, hit 'exit' to finish setup."
sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"

mv ~/../usr/etc/motd_finished ~/../usr/etc/motd
echo "cat ~/../usr/etc/motd" >> .zshrc
sed -i 's/robbyrussell/avit/g' .zshrc
echo "[TEL]: final step: hit 'byobu-enable'"
echo "[TEL]: after that, hit 'exit' and restart the app!"
echo "[TEL]: if the layout is still missing, hit 'byobu-enable' and 'exit' again!"

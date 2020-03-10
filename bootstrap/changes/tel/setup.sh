echo "[TEL]: finishing TEL setup"

echo "[TEL]: installing required packages:"
pkg install fzf byobu curl wget nano tmux zsh ncurses-utils git make -y

echo "[TEL]: installing app launcher:"
mkdir -p ~/.local/share/
git clone https://github.com/t-e-l/tel-app-launcher
cd tel-app-launcher
make install
cd ..
rm -rf tel-app-launcher

#echo "/data/data/com.termux/files/usr/bin/tel-appcache" >> ~/../usr/var/lib/dpkg/info/termux-tools.list
#echo "92a2c39cbbde0f366887d99a76358852  data/data/com.termux/files/usr/bin/tel-appcache" >> ~/../usr/var/lib/dpkg/info/termux-tools.md5sums
#idk if this is necessary
app -u

echo "[TEL]: installing OhMyZsh"
echo "if you enable zsh, hit 'exit' to finish setup."
sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"

cd ~
mv ~/../usr/etc/motd_finished ~/../usr/etc/motd
echo "cat ~/../usr/etc/motd" >> .zshrc
sed -i 's/robbyrussell/avit/g' .zshrc

echo "[TEL]: moving files"
mv ~/../usr/tel/.byobu/* ~/
mv ~/../usr/tel/.termux/* ~/
mv ~/../usr/tel/.tel/* ~/
chmod +x .tel/status.sh

echo "[TEL]: final step: hit 'byobu-enable'"
echo "[TEL]: after that, hit 'exit' and restart the app!"
echo "[TEL]: if the layout is still missing, hit 'byobu-enable' and 'exit' again!"

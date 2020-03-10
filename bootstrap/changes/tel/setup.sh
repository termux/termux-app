UPDATE=false

log() {
	printf "\033[0;%sm%s\033[0m\033[0;%sm%s\033[0m\n" "${WHITE}" "[TEL]: " "${GREEN}" "${1}"
}
error() {
	printf "\033[0;%sm%s\033[0m\033[0;%sm%s\033[0m\n" "${WHITE}" "[TEL]: " "${RED}" "${1}"
}
error_exit(){
	error "setup failed. please retry!"
	exit 1
}
if [ -f "~/.tel/.installed" ]; then
    UPDATE=true
    log "updating TEL setup"
    log "updating app launcher"
else
	log "finishing TEL setup"
	log "installing required packages"
	pkg install fzf byobu curl wget nano tmux zsh ncurses-utils git make -y
	if [ ! $? -eq 0 ]; then
		error_exit 
	fi
	log "installing app launcher"
fi

cd ~
git clone https://github.com/t-e-l/tel-app-launcher
if [ ! $? -eq 0 ]; then
	error_exit 
fi
cd tel-app-launcher
make install
cd ~
rm -rf tel-app-launcher

#echo "/data/data/com.termux/files/usr/bin/tel-appcache" >> ~/../usr/var/lib/dpkg/info/termux-tools.list
#echo "92a2c39cbbde0f366887d99a76358852  data/data/com.termux/files/usr/bin/tel-appcache" >> ~/../usr/var/lib/dpkg/info/termux-tools.md5sums
#idk if this is necessary

app -u
mkdir -p ~/.byobu
mkdir -p ~/.termux
mkdir -p ~/.tel

if [ "$UPDATE" = false]; then

	log "installing OhMyZsh"
	error "if you enable zsh, type 'exit' to finish setup."
	log "hit ENTER to continue"
	read blazeit
	sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
	sed -i 's/robbyrussell/avit/g' ~/.zshrc
	echo "cat ~/../usr/etc/motd" >> ~/.zshrc

	log "installing configs"
	mv ~/../usr/tel/.byobu/* ~/.byobu/
	mv ~/../usr/tel/.termux/* ~/termux/
	mv ~/../usr/tel/.tel/* ~/.tel/
fi

cd ~
log "updating configs"

error "updating byobu config. type 'no' to skip (not recommend)"
read byobu
if [ ! "$byobu" = "no" ]; then
	mv ~/../usr/tel/.byobu/* ~/
fi

error "updating .tel files. type 'no' to skip (not recommend)"
read telfiles
if [ ! "$telfiles" = "no" ]; then
	mv ~/../usr/tel/.tel/* ~/
fi

log "updating permissions"

chmod +x ~/.tel/status.sh
chmod +x ~/../usr/bin/tel-applist
chmod +x ~/../usr/bin/tel-setup

if [ -f "~/../usr/etc/motd_finished" ]; then
	mv ~/../usr/etc/motd_finished ~/../usr/etc/motd
fi

touch ~/.tel/.installed

log "final step: hit 'byobu-enable'"
log "after that, hit 'exit' and restart the app!"
log "if the layout is still missing, hit 'byobu-enable' and 'exit' again!"

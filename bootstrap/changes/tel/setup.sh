#!/data/data/com.termux/files/usr/bin/bash

#TEL setup file
#should be executed after the setup and every apk based update

#set color and update vars
UPDATE=false
WHITE=${1:-"38;5;07"}
GREEN=${1:-"38;5;02"}
RED=${1:-"38;5;01"}

#set helper functions
#todo:move them to helpers file and use them
log() {
	printf "\033[0;%sm%s\033[0m\033[0;%sm%s\033[0m\n" "${WHITE}" "[TEL]: " "${GREEN}" "${1}"
}
error() {
	printf "\033[0;%sm%s\033[0m\033[0;%sm%s\033[0m\n" "${WHITE}" "[TEL]: " "${RED}" "${1}"
}

if [ -f "~/.tel/.installed" ]; then #set update var if finished installation was detected
    UPDATE=true
    log "updating TEL setup"
    log "updating app launcher"
else #download required packages if first start detected
	log "finishing TEL setup"
	log "installing required packages"
	pkg install fzf byobu curl wget nano tmux zsh ncurses-utils git make ruby -y
	log "installing app launcher"
fi

#install lolcat for colors
gem install lolcat

#install app launcher via git
cd ~
git clone https://github.com/t-e-l/tel-app-launcher
cd tel-app-launcher
make install
cd ~
rm -rf tel-app-launcher

#other termux tools are listed in these files, idk if its necessary to maintain them
#echo "/data/data/com.termux/files/usr/bin/tel-appcache" >> ~/../usr/var/lib/dpkg/info/termux-tools.list
#echo "92a2c39cbbde0f366887d99a76358852  data/data/com.termux/files/usr/bin/tel-appcache" >> ~/../usr/var/lib/dpkg/info/termux-tools.md5sums


app -u #set up app cache
#create required directories
#todo: optimize this
mkdir -p ~/.byobu
mkdir -p ~/.termux
mkdir -p ~/.tel

if [ "$UPDATE" = false ]; then #if first start detected

	#install OhMyZsh
	log "installing OhMyZsh"
	error "if you enable zsh, type 'exit' to finish setup."
	log "hit ENTER to continue"
	read blazeit
	sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"

	sed -i 's/robbyrussell/avit/g' ~/.zshrc #set avit zsh theme
	echo "cat ~/../usr/etc/motd | lolcat" >> ~/.zshrc #set motd message for zsh

	log "installing configs" #todo: optimize this
	cp -rf ~/../usr/tel/.byobu/* ~/.byobu/
	cp -rf ~/../usr/tel/.termux/* ~/.termux/
	cp -rf ~/../usr/tel/.tel/* ~/.tel/
	cp -rf ~/../usr/tel/.byobu/.tmux.conf ~/.byobu/
fi
log "updating configs"
error "hit ENTER to continue, type no to skip(not recommended)"
read update_configs
if [ ! "$update_configs" = "no" ]; then
	cp -rf ~/../usr/tel/.byobu/* ~/.byobu/
	cp -rf ~/../usr/tel/.termux/* ~/.termux/
	cp -rf ~/../usr/tel/.byobu/.tmux.conf ~/.byobu/
fi

cp -rf ~/../usr/tel/.tel/* ~/.tel/

cd ~

log "updating permissions"

#set permissions again(probably duplicate within tel-setup)
chmod +x ~/.tel/status.sh
chmod +x ~/../usr/bin/tel-applist
chmod +x ~/../usr/bin/tel-setup

if [ -f "~/../usr/etc/motd_finished" ]; then
	mv ~/../usr/etc/motd_finished ~/../usr/etc/motd #set final motd
fi

touch ~/.tel/.installed #mark setup finished

log "final step: hit 'byobu-enable'"
log "after that, hit 'exit' and restart the app!"
log "if the layout is still missing, hit 'byobu-enable' and 'exit' again!"

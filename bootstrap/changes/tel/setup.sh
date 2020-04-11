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
catch(){
if [ $?  -ne 0 ]
then
        logf "${1}"
        error "${1}"
	error "please try again"
        exit 0
else
	logf "done"
fi
}


logf(){
        echo "[$(date '+%Y-%m-%d %H:%M:%S')]: ${1}" >> ~/.tel_log
}
log() {
	printf "\033[0;%sm%s\033[0m\033[0;%sm%s\033[0m\n" "${WHITE}" "[TEL]: " "${GREEN}" "${1}"
}
error() {
	printf "\033[0;%sm%s\033[0m\033[0;%sm%s\033[0m\n" "${WHITE}" "[TEL]: " "${RED}" "${1}"
}

if [ -f ~/.tel/.installed ]; then #set update var if finished installation was detected
    UPDATE=true
    log "updating TEL setup"
    log "updating app launcher"
    logf "starting update"
else #download required packages if first start detected
	log "finishing TEL setup"
	log "installing required packages.."
	log "this may take a while"
        logf "starting installation"
	catch "$(pkg install fzf byobu curl wget nano tmux zsh ncurses-utils python jq neofetch git make figlet termux-api -y 2>&1)"
	catch "$(curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py 2>&1)"
        catch "$(python get-pip.py 2>&1)"
        rm -f get-pip.py
        catch "$(pip install colored lolcat 2>&1)"
        log "installing app launcher"
fi

#install lolcat for colors
#gem install lolcat

#install app launcher via git
cd ~
catch "$(git clone https://github.com/t-e-l/tel-app-launcher 2>&1)"
cd tel-app-launcher
catch "$(make install 2>&1)"
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
	#log "installing OhMyZsh"
	#error "if you enable zsh, type 'exit' to finish setup."
	#log "hit ENTER to continue"
	#read blazeit
	#sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
	sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended  2>&1
  	chsh -s zsh #set zsh default shell
  	sed -i 's/robbyrussell/avit/g' ~/.zshrc
  	echo "_byobu_sourced=1 . /data/data/com.termux/files/usr/bin/byobu-launch 2>/dev/null || true" >> ~/.zprofile
        echo "export PATH=$PATH:~/.tel/bin" >> ~/.zshrc #add tel bins to path
	echo "source ~/.aliases" >> ~/.zshrc
        echo "set -K # disables ! events in zsh to allow for bangs in tel-search" >> ~/.zshrc
	log "installing configs" #todo: optimize this

	cp -rTf ~/../usr/tel/.tel ~/.tel
	cp -rTf ~/../usr/tel/.byobu ~/.byobu
	cp -rTf ~/../usr/tel/.termux ~/.termux

	cp -rf ~/../usr/tel/.aliases ~/
	cp -rf ~/../usr/tel/.envvar ~/

else
	log "updating configs"
	cp -rTf ~/../usr/tel/.byobu ~/.byobu
	cp -rTf ~/../usr/tel/.tel/bin ~/.tel/bin
fi


log "updating permissions"

#set permissions again(probably duplicate within tel-setup)
chmod +x ~/.tel/status/*
chmod +x ~/.tel/scripts/status_manager/*
chmod +x ~/.tel/bin/*
chmod +x ~/../usr/bin/tel-applist
chmod +x ~/../usr/bin/tel-setup
chmod +x ~/../usr/bin/tel-restart

if [ -f "$HOME/../usr/etc/motd_finished" ]; then
	mv ~/../usr/etc/motd_finished ~/../usr/etc/motd #set final motd

fi

if [ "$UPDATE" = false ]; then 
	touch ~/.tel/.installed #mark setup finished
        log "installation finished"
else
        log "update finished"
fi
logf "finished"
error "app will restart in 3 seconds!"
sleep 3
tel-restart

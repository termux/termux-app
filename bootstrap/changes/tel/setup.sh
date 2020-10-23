#!/usr/bin/env bash

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

if [ "$1" == "--reset" ] ; then
	log 'cleaning items for fresh install'
	rm -f ~/.tel/.installed
	rm -rf ~/.oh-my-zsh
fi


log "updating Termux packages..."
logf "updating Termux packages..."
apt-get update -y && apt-get upgrade -y #print to screen as hotfix
log "finished updating Termux packages"
logf "finished updating Termux packages"
if [ -f ~/.tel/.installed ]; then #set update var if finished installation was detected
    UPDATE=true
	log "updating TEL setup"
	#log "updating app launcher"
	logf "starting update"
else #download required packages if first start detected
	log "finishing TEL setup"
	log "installing required packages.."
	log "this may take a while"
        logf "starting installation"
	catch "$(pkg install fzf nnn sl cowsay openssh tree bc fd curl wget nano tmux zsh ncurses-utils python jq neofetch git make figlet termux-api sed util-linux -y 2>&1)"
	catch "$(curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py 2>&1)"
        catch "$(python get-pip.py 2>&1)"
        rm -f get-pip.py
        catch "$(pip install blessed lolcat powerline-status 2>&1)" #removed psutil
        log "finished packages download and installation"
        logf "finished packages download and installation"
fi

#install lolcat for colors
#gem install lolcat

#install app launcher via git
#cd ~
#catch "$(git clone https://github.com/t-e-l/tel-app-launcher 2>&1)"
#cd tel-app-launcher
#catch "$(make install 2>&1)"
#cd ~
#rm -rf tel-app-launcher

#other termux tools are listed in these files, idk if its necessary to maintain them
#echo "/data/data/com.termux/files/usr/bin/tel-appcache" >> ~/../usr/var/lib/dpkg/info/termux-tools.list
#echo "92a2c39cbbde0f366887d99a76358852  data/data/com.termux/files/usr/bin/tel-appcache" >> ~/../usr/var/lib/dpkg/info/termux-tools.md5sums

#do these when app starts up so we can keep applets together
#tel-app -u #set up app cache
#tel-phone -u #this can fail so is preferable at app startup

#create required directories
#todo: optimize this
mkdir -p ~/.termux
mkdir -p ~/.tel
mkdir -p ~/bin

if [ "$UPDATE" = false ]; then #if first start detected

	#log "installing OhMyZsh"
	#error "if you enable zsh, type 'exit' to finish setup."
	#log "hit ENTER to continue"
	#read blazeit
	#sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
	sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended  2>&1
  	chsh -s zsh #set zsh default shell
	#install zsh plugins
	catch "$(git clone https://github.com/zsh-users/zsh-autosuggestions ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions 2>&1)"
	catch "$(git clone https://github.com/zsh-users/zsh-syntax-highlighting.git ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting 2>&1)"
  	sed -i 's/robbyrussell/avit/g' ~/.zshrc
	sed -i 's/plugins=(git)/plugins=(git catimg fancy-ctrl-z zsh-syntax-highlighting zsh-autosuggestions)/g' ~/.zshrc #fzf maybe needed here
  #	echo "_byobu_sourced=1 . /data/data/com.termux/files/usr/bin/byobu-launch 2>/dev/null || true" >> ~/.zprofile
	echo ". ~/.tel/.telrc # Load TEL " >> ~/.zshrc
	log "installing configs" #todo: optimize this

	cp -rTf ~/../usr/tel/.tel ~/.tel
	cp -rTf ~/../usr/tel/.termux ~/.termux
	cp -rf ~/../usr/tel/termux-file-editor ~/bin
	cp -rf ~/../usr/tel/termux-url-opener ~/bin
	cp -rf ~/../usr/tel/.aliases ~/
	cp -rf ~/../usr/tel/.envvar ~/
	cp -rf ~/../usr/tel/.tmux.conf ~/
	cp -rf ~/../usr/tel/.zlogin ~/
	cp -rf ~/../usr/tel/.vimrc ~/
	cp -rf ~/../usr/tel/.telrc ~/.tel/

else
	log "updating configs"
	cp -rTf ~/../usr/tel/.tel/bin ~/.tel/bin
fi


log "updating permissions"
#set permissions again(probably duplicate within tel-setup)
chmod +x ~/.tel/status/*
chmod +x ~/.tel/scripts/*
chmod +x ~/.tel/scripts/status_manager/*
chmod +x ~/.tel/bin/*
chmod +x ~/bin/* # scripts that receive files and urls shared to TEL
#chmod +x ~/../usr/bin/tel-applist
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

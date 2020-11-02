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

log "Updating Termux packages..."
logf "Updating Termux packages..."
apt-get update -y && apt-get upgrade -y && logf "finished updating Termux packages" #print to screen as hotfix
if [ -f ~/.tel/.installed ]; then #set update var if finished installation was detected
    UPDATE=true
	log "Updating TEL setup"
	logf "Starting update"
else #download required packages if first start detected
	log "Installing required packages.."
	log "This may take a while..."
        logf "Starting installation"
	catch "$(pkg install fzf sl cowsay openssh tree bc fd curl wget nano tmux zsh neofetch git make figlet ncurses-utils termux-api sed util-linux -y 2>&1)" #removed jq
	log "Installing python"
	logf "Installing python"
	catch "$(pkg install python -y 2>&1)" #removed jq
	log "Installing python package manager"
	catch "$(curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py 2>&1)"
        catch "$(python get-pip.py 2>&1)"
        rm -f get-pip.py
	log "Installing python packages"
        catch "$(pip install --user blessed lolcat powerline-status 2>&1)" #removed psutil
        logf "Finished packages download and installation"
fi

#other termux tools are listed in these files, idk if its necessary to maintain them
#echo "/data/data/com.termux/files/usr/bin/tel-appcache" >> ~/../usr/var/lib/dpkg/info/termux-tools.list
#echo "92a2c39cbbde0f366887d99a76358852  data/data/com.termux/files/usr/bin/tel-appcache" >> ~/../usr/var/lib/dpkg/info/termux-tools.md5sums

#create required directories
#todo: optimize this
mkdir -p ~/.termux
mkdir -p ~/.tel
mkdir -p ~/.config
mkdir -p ~/bin

if [ "$UPDATE" == false ]; then #if first start detected
	# # # # ZSH setup # # #
	log "Installing ohmyzsh"
	logf "Installing ohmyzsh"
	sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended  > /dev/null 2>&1 && logf "finished installing OhMyZsh"
  	chsh -s zsh #set zsh default shell
	#install zsh plugins
	#disabled because interferences with suggestion bar
	catch "$(git clone https://github.com/zsh-users/zsh-autosuggestions ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions 2>&1)"
	catch "$(git clone https://github.com/zsh-users/zsh-syntax-highlighting.git ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting 2>&1)"
  	sed -i 's/robbyrussell/avit/g' ~/.zshrc
	sed -i 's/plugins=(git)/plugins=(git catimg fancy-ctrl-z zsh-syntax-highlighting)/g' ~/.zshrc #fzf maybe needed here
	# setup zsh history (share between windows)
	echo -e "\n# ZSH HISTORY #\nHISTSIZE=40000\nSAVEHIST=20000\nHISTFILE=~/.zsh_history\nsetopt INC_APPEND_HISTORY\nsetopt SHARE_HISTORY\n" >> ~/.zshrc
	# setup tel loading
	echo -e "	\n#|||||||||||||||#\n. ~/.tel/.telrc\n#|||||||||||||||#\n	" >> ~/.zshrc

	log "Installing TEL..." #todo: optimize this
	logf "Installing TEL..."

	#cp -rf ~/../usr/tel/termux-file-editor ~/bin
	#cp -rf ~/../usr/tel/termux-url-opener ~/bin

	# put thingies in actual location
	cp -rf ~/../usr/tel/. ~/
	rm -rf ~/setup.sh	
#cp -rTf ~/../usr/tel/.tel ~/.tel
	#cp -rTf ~/../usr/tel/.termux ~/.termux
	#cp -rf ~/../usr/tel/.config ~/
	#cp -rf ~/../usr/tel/.nano ~/
	#cp -rf ~/../usr/tel/.aliases ~/
	#cp -rf ~/../usr/tel/.envvar ~/
	#cp -rf ~/../usr/tel/.tmux.conf ~/
	#cp -rf ~/../usr/tel/.zlogin ~/
	#cp -rf ~/../usr/tel/.nanorc ~/

else
	log "Updating TEL..."
	logf "Updating TEL..."
	cp -rf ~/../usr/tel/.tel/. ~/.tel
	#cp -rTf ~/../usr/tel/.tel/bin ~/.tel/bin
	#cp -rTf ~/../usr/tel/.tel/scripts ~/.tel/scripts
	#cp -rTf ~/../usr/tel/.tel/configs ~/.tel/configs #this should be compared with a diff tool
	#cp -rf ~/../usr/tel/.tel/status/* ~/.tel/status/
	#cp -rf ~/../usr/tel/.tel/resources/* ~/.tel/resources/
	#cp -rTf ~/../usr/tel/.tel/tutorials ~/.tel/tutorials
fi

log "Updating permissions..."
logf "Updating permissions..."
chmod +x ~/.tel/extras/*
chmod +x ~/.tel/status/*
chmod +x ~/.tel/scripts/*
chmod +x ~/.tel/scripts/status_manager/*
chmod +x ~/.tel/tutorials/*
chmod +x ~/.tel/bin/*
chmod +x ~/bin/* # scripts that receive files and urls shared to TEL
chmod +x ~/../usr/bin/*

if [ -f "$HOME/../usr/etc/motd_finished" ]; then
	mv ~/../usr/etc/motd_finished ~/../usr/etc/motd #set final motd
fi

if [ "$UPDATE" = false ]; then
	touch ~/.tel/.installed #mark setup finished
        log "Installation Complete"
else
        log "Update Complete"
fi
logf "Complete"
log "app will restart in 3 seconds!"
sleep 3
tel-restart
error 'Restart cannot be performed automatically when app is not active'
error 'Please run the command "tel-restart" manually'
exit 0

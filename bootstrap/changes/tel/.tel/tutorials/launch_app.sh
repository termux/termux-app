#!/usr/bin/env bash
clear ; cowsay "Okay, App Launch tutorial!"
read user_response
clear ; cowsay "I will type and interact with the terminal the same way you can"
read user_response
clear ; cowsay "Don't touch the screen until the tutorial has finished!"
read user_response
clear ; cowsay "First I will split the window, so I can continue to explain.."
read user_response
tmux split-pane -dv 
clear ; cowsay "Now I will type the app command..."
read user_response
tel-typewriter -i tel-app -t Tutorial.bottom
clear ; cowsay "Now I will hit enter to run the app launcher interatively"
read user_response
tel-typewriter -k Enter -t Tutorial.bottom
clear ; cowsay "Then I start typing an app name, it's closest result is highlighted"
read user_response
tel-typewriter -i settin -t Tutorial.bottom
clear ; cowsay "As soon as the app you're looking for has been highlighted.."
read user_response
clear ; cowsay "You can hit enter again to launch! or cancel with Ctrl+c"
read user_response
clear ; cowsay "I will cancel with Ctrl+c for now"
read user_response
tel-typewriter -k C-c -t Tutorial.bottom
clear ; cowsay "you can also start tel-app with the shortcut: Alt+a"
read user_response
clear ; cowsay "or you can type the app name after tel-app. for example: 'tel-app settings'"
read user_response
clear ; cowsay "Thus concludes our tutorial!"
read user_response
tel-typewriter -i exit -k Enter -t Tutorial.bottom

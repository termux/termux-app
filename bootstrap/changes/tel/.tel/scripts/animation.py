#!/usr/bin/env python
# TEL startup - text animation - by sealyj 2020
from blessed import Terminal
from time import sleep
import os
import sys
import random
term = Terminal()
words = ["hausTELlum", "TELeologist", "TELeonomy", "aTELectasis", "disTELfink", "protosTELe", "scuTELlate", "TELiospore", "chaTELaine", "taranTELla","paTELliform", "inTELlections", "viTELlogenesis","consTELations", "sTELlar", "TELstar", "intersTELlar","TELephone", "TELepathy", "TELekineses", "TELemetry", "TELevision", "TELecaster", "TELnet","torTELlini", "tagliaTELle", "absoluTELy","accuraTELy","inTELlect","wasTELands","ultimaTELy","sTELlite","TELescope","TELomeres","TELophase","anTELope","plaTELet","remoTELy","TELecom","pasTELs","saTELlites","infiniTELy","casTELlated", "inTELechies", "consTELlate","inTELligent","immorTELles","foreTELler","exquisiTELy","TELeporting","tasTELess","canisTELs","clienTELle","staTELless", "TELephoto","TELevisual","TELler", "carTEL", "hoTEL", "inTEL"]
emoji = "🐙🐛🐬🐳🐟🐍🦎🐢🐉🐊🦉🦆🐥🐧🐦🦃🐔🦅🐓🐇🦇🐀🐘🐑🐏🐄🐖🐪🐫🐆🐅🐒🐕🐩🐠🐡🦈🦀🦐🦑🐌🐞🦋🦆"
#wrongletters = '殺12475940382829596060392918288475859403020294958585858587229202'

wrongletters = 'abcdefghijklmnopqrsthuvxyz'
randomwords = True
fill_chars = ''
centerout = '#'
#wrongletters = ' 1234567890'
totalchar = term.height * term.width
center_w = int(term.height / 2)
os.system('clear')
for character in range(totalchar):
    random_fill_char = random.randint(0,len(fill_chars) -1)
    print(fill_chars[random_fill_char],end='',flush=True)
    sleep(0.00005)

def generate_words():
    x = 0
    line_words = []
    lines = []
    while len(lines) < term.height:
    #while x <= totalchar and randomwords is True:
        chosen_word = random.randint(0,len(words)) - 1
   #     selected_words.append(words[chosen_word])
        #x = x + len(words[chosen_word]) + 1
        line_words.append(words[chosen_word])
        s = ''.join(line_words)
        #line = line2    
        if len(s) >= term.width:
            #line = line2 + line
            line = s[:term.width]
            line2 = s[term.width:]
            lines.append(s)
            line_words = []
           # line = line2
    return lines

center = int((term.width) / 2)
xL = center
xR = center
for line_num in range(int(term.height / 10),term.height):
    num_dots = line_num
    #shaper = int(line_num * term.height / 20) # triangle shape
    shaper = int( term.height * line_num / int(term.height -11)) 
    for width in range(term.width):
        if xL <= 0 + shaper or xR >= term.width - shaper:
            xL = center
            xR = center
            print(term.home)
            break
        elif xR <= 0 + shaper or xL >= term.width - shaper:
            xL = center
            xR = center
            print(term.home)
            break
        else:
            randcenter = random.randint(0,len(centerout) -1)
            #print left of center
            print(term.move_xy(xL, line_num) + centerout[randcenter] ,end='',flush=True)
            #print right of center
            print(term.move_xy(xR, line_num) + centerout[randcenter] ,end='',flush=True)


        sleep(0.001)
        if line_num <= term.height / 2:
            xL = xL - 1
            xR = xR + 1
        else:

            xL = xL + 1

            xR = xR - 1


lines = generate_words()
line = ''
#print("height is: " + str(term.height) + "lines list len: " + str(len(lines)))
sleep(1)

tel_locations = []
query = ["T","E","L"]
results = []
telstrsx = []
telstrsy = []
telstrs = []
telstrschar = []
lineflags = []
for currentline in range(term.height):
    letter = random.randint(0,term.width-1)
    flags = []
    while line_num in lineflags:
        line_num = random.randint(0,term.height-1) 
    lineflags.append(line_num)
    for letter_num in range(term.width):
                while letter in flags:
                    letter = random.randint(0,term.width-1)
                current_char = lines[line_num][letter]
                results = []
                for q in query:
                    result = current_char.find(q)
                    results.append(result)
                if 0 in results and letter not in flags:
                        telstrsx.append(letter)
                        telstrsy.append(line_num)
                        telstrs.append(current_char)
                        flags.append(letter)
                elif 0 not in results and letter not in flags:
                       # print("0 is NOT in results!")
                        print(term.move_xy(letter, line_num) + current_char,end='',flush=True)
                        sleep(0.00001)
                        flags.append(letter)

r = random.randint(0,255)
g = random.randint(0,255)
b = random.randint(0,255)
counter = 0
z = 0
flags = []
rand = random.randint(0, len(telstrsx))
while telstrs:
    if len(flags) >= len(telstrs):
        break 
    while rand in flags:
        rand = random.randint(0, len(telstrsx) -1)
    flags.append(rand) 
    current_char = telstrs[rand]
    x = telstrsx[rand]
    y = telstrsy[rand]
    print(term.move_xy(x, y) + term.color_rgb(r,g,b) + current_char.lower() + term.normal,end='',flush=True)
    counter = counter + 1 


y = 0
x = 0
counter = 0
randomflashers = True
if randomflashers == True:
    y = int(term.height /2)
    while True:
            exit_msg = "  Welcome to TEL"
            half_exit_msg = int(int(len(exit_msg)) / 2)
            flicker = random.randint(0,len(emoji) - 1)
            randx = random.randint(int(term.width / 2) - half_exit_msg,int(term.width /2) + half_exit_msg)
            print(term.move_xy(randx, y) + emoji[flicker],end='',flush=True)
            sleep(0.01)
            if counter == 200:
                counter = 0
                break
            counter = counter + 1
    chars = []
    nothing = True
    exit_msg_flags=[]
    while nothing is True:
        #end on legible message 
            y = int(term.height /2)
            while True:# charac in range(0,len(exit_msg)):
                half_width = int(int(term.width) / 2) 
                randx = random.randint(half_width - half_exit_msg, half_width + half_exit_msg)
                while randx in exit_msg_flags:
                    randx = random.randint(half_width - half_exit_msg, half_width + half_exit_msg)
                exit_msg_flags.append(randx) 
    
                for eachwrongletter in range(0,int(len(wrongletters) - 1)):
                    flicker = random.randint(0,len(wrongletters) - 1)
                    print(term.move_xy(randx, y) + wrongletters[flicker],end='',flush=True)
                    sleep(0.01)
                correct_letter = randx - int(half_width + half_exit_msg)
                print(term.move_xy(randx, y) + exit_msg[correct_letter],end='',flush=True)
                counter = counter + 1
                sleep(0.005)
                print(term.move_xy(term.width -2, term.height -2),flush=True)
                break
            
            if len(exit_msg_flags) == len(exit_msg) +1:
                break

sleep(3)
os.system('clear')
exit()

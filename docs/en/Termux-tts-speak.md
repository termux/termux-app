Speak text with a system text-to-speech (TTS) engine. The text to speak
is either supplied as arguments or read from stdin if no arguments are
given.

## Usage

termux-tts-speak \[-e engine\] \[-l language\] \[-n region\] \[-v
variant\] \[-p pitch\] \[-r rate\] \[-s stream\] \[text-to-speak\]

### Options

` -e engine    TTS engine to use (see termux-tts-engines)`
` -l language  language to speak in (may be unsupported by the engine)`
` -n region    region of language to speak in`
` -v variant   variant of the language to speak in`
` -p pitch     pitch to use in speech. 1.0 is the normal pitch,`
`                lower values lower the tone of the synthesized voice,`
`                greater values increase it.`
` -r rate      speech rate to use. 1.0 is the normal speech rate,`
`                lower values slow down the speech`
`                (0.5 is half the normal speech rate)`
`                while greater values accelerates it`
`                (2.0 is twice the normal speech rate).`
` -s stream    audio stream to use (default:NOTIFICATION), one of:`
`                ALARM, MUSIC, NOTIFICATION, RING, SYSTEM, VOICE_CALL`

## Tips & Tricks

### Termux-tts-speak is slow to start

It takes quite some time for it to actually play anything, but most of
that lost time comes due to startup time of the engine. You can keep the
engine running by using a fifo queue instead.

`   mkfifo ~/.tts`
`   while true; do cat ~/.tts; done | termux-tts-speak`

Then you can use it like this:

`   echo Today is > ~/.tts`
`   date > ~/.tts`

This will keep termux-tts-speak running and just play anything that's
send to `~/.tts`
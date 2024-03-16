/* Copyright (c) 2004-2005, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <sys/audio.h>
#include <sys/uio.h>
#include <limits.h>
#include <math.h>
#include <xserver_poll.h>

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#define BELL_RATE       48000   /* Samples per second */
#define BELL_HZ         50      /* Fraction of a second i.e. 1/x */
#define BELL_MS         (1000/BELL_HZ)  /* MS */
#define BELL_SAMPLES    (BELL_RATE / BELL_HZ)
#define BELL_MIN        3       /* Min # of repeats */

#define AUDIO_DEVICE    "/dev/audio"

void
xf86OSRingBell(int loudness, int pitch, int duration)
{
    static short samples[BELL_SAMPLES];
    static short silence[BELL_SAMPLES]; /* "The Sound of Silence" */
    static int lastFreq;
    int cnt;
    int i;
    int written;
    int repeats;
    int freq;
    audio_info_t audioInfo;
    struct iovec iov[IOV_MAX];
    int iovcnt;
    double ampl, cyclen, phase;
    int audioFD;

    if ((loudness <= 0) || (pitch <= 0) || (duration <= 0)) {
        return;
    }

    lastFreq = 0;
    memset(silence, 0, sizeof(silence));

    audioFD = open(AUDIO_DEVICE, O_WRONLY | O_NONBLOCK);
    if (audioFD == -1) {
        xf86Msg(X_ERROR, "Bell: cannot open audio device \"%s\": %s\n",
                AUDIO_DEVICE, strerror(errno));
        return;
    }

    freq = pitch;
    freq = min(freq, (BELL_RATE / 2) - 1);
    freq = max(freq, 2 * BELL_HZ);

    /*
     * Ensure full waves per buffer
     */
    freq -= freq % BELL_HZ;

    if (freq != lastFreq) {
        lastFreq = freq;
        ampl = 16384.0;

        cyclen = (double) freq / (double) BELL_RATE;
        phase = 0.0;

        for (i = 0; i < BELL_SAMPLES; i++) {
            samples[i] = (short) (ampl * sin(2.0 * M_PI * phase));
            phase += cyclen;
            if (phase >= 1.0)
                phase -= 1.0;
        }
    }

    repeats = (duration + (BELL_MS / 2)) / BELL_MS;
    repeats = max(repeats, BELL_MIN);

    loudness = max(0, loudness);
    loudness = min(loudness, 100);

#ifdef DEBUG
    ErrorF("BELL : freq %d volume %d duration %d repeats %d\n",
           freq, loudness, duration, repeats);
#endif

    AUDIO_INITINFO(&audioInfo);
    audioInfo.play.encoding = AUDIO_ENCODING_LINEAR;
    audioInfo.play.sample_rate = BELL_RATE;
    audioInfo.play.channels = 2;
    audioInfo.play.precision = 16;
    audioInfo.play.gain = min(AUDIO_MAX_GAIN, AUDIO_MAX_GAIN * loudness / 100);

    if (ioctl(audioFD, AUDIO_SETINFO, &audioInfo) < 0) {
        xf86Msg(X_ERROR,
                "Bell: AUDIO_SETINFO failed on audio device \"%s\": %s\n",
                AUDIO_DEVICE, strerror(errno));
        close(audioFD);
        return;
    }

    iovcnt = 0;

    for (cnt = 0; cnt <= repeats; cnt++) {
        if (cnt == repeats) {
            /* Insert a bit of silence so that multiple beeps are distinct and
             * not compressed into a single tone.
             */
            iov[iovcnt].iov_base = (char *) silence;
            iov[iovcnt++].iov_len = sizeof(silence);
        }
        else {
            iov[iovcnt].iov_base = (char *) samples;
            iov[iovcnt++].iov_len = sizeof(samples);
        }
        if ((iovcnt >= IOV_MAX) || (cnt == repeats)) {
            written = writev(audioFD, iov, iovcnt);

            if ((written < ((int) (sizeof(samples) * iovcnt)))) {
                /* audio buffer was full! */

                int naptime;

                if (written == -1) {
                    if (errno != EAGAIN) {
                        xf86Msg(X_ERROR,
                                "Bell: writev failed on audio device \"%s\": %s\n",
                                AUDIO_DEVICE, strerror(errno));
                        close(audioFD);
                        return;
                    }
                    i = iovcnt;
                }
                else {
                    i = ((sizeof(samples) * iovcnt) - written)
                        / sizeof(samples);
                }
                cnt -= i;

                /* sleep a little to allow audio buffer to drain */
                naptime = BELL_MS * i;
                xserver_poll(NULL, 0, naptime);

                i = ((sizeof(samples) * iovcnt) - written) % sizeof(samples);
                iovcnt = 0;
                if ((written != -1) && (i > 0)) {
                    iov[iovcnt].iov_base = ((char *) samples) + i;
                    iov[iovcnt++].iov_len = sizeof(samples) - i;
                }
            }
            else {
                iovcnt = 0;
            }
        }
    }

    close(audioFD);
    return;
}

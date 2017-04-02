package com.termux.terminal;

/**
 * Implementation of wcwidth(3) for Unicode 9.
 *
 * Implementation from https://github.com/jquast/wcwidth but we return 0 for unprintable characters.
 */
public final class WcWidth {

    // From https://github.com/jquast/wcwidth/blob/master/wcwidth/table_zero.py
    // t commit 0d7de112202cc8b2ebe9232ff4a5c954f19d561a (2016-07-02):
    private static final int[][] ZERO_WIDTH = {
        {0x0300, 0x036f},  // Combining Grave Accent  ..Combining Latin Small Le
        {0x0483, 0x0489},  // Combining Cyrillic Titlo..Combining Cyrillic Milli
        {0x0591, 0x05bd},  // Hebrew Accent Etnahta   ..Hebrew Point Meteg
        {0x05bf, 0x05bf},  // Hebrew Point Rafe       ..Hebrew Point Rafe
        {0x05c1, 0x05c2},  // Hebrew Point Shin Dot   ..Hebrew Point Sin Dot
        {0x05c4, 0x05c5},  // Hebrew Mark Upper Dot   ..Hebrew Mark Lower Dot
        {0x05c7, 0x05c7},  // Hebrew Point Qamats Qata..Hebrew Point Qamats Qata
        {0x0610, 0x061a},  // Arabic Sign Sallallahou ..Arabic Small Kasra
        {0x064b, 0x065f},  // Arabic Fathatan         ..Arabic Wavy Hamza Below
        {0x0670, 0x0670},  // Arabic Letter Superscrip..Arabic Letter Superscrip
        {0x06d6, 0x06dc},  // Arabic Small High Ligatu..Arabic Small High Seen
        {0x06df, 0x06e4},  // Arabic Small High Rounde..Arabic Small High Madda
        {0x06e7, 0x06e8},  // Arabic Small High Yeh   ..Arabic Small High Noon
        {0x06ea, 0x06ed},  // Arabic Empty Centre Low ..Arabic Small Low Meem
        {0x0711, 0x0711},  // Syriac Letter Superscrip..Syriac Letter Superscrip
        {0x0730, 0x074a},  // Syriac Pthaha Above     ..Syriac Barrekh
        {0x07a6, 0x07b0},  // Thaana Abafili          ..Thaana Sukun
        {0x07eb, 0x07f3},  // Nko Combining Sh||t High..Nko Combining Double Dot
        {0x0816, 0x0819},  // Samaritan Mark In       ..Samaritan Mark Dagesh
        {0x081b, 0x0823},  // Samaritan Mark Epentheti..Samaritan Vowel Sign A
        {0x0825, 0x0827},  // Samaritan Vowel Sign Sho..Samaritan Vowel Sign U
        {0x0829, 0x082d},  // Samaritan Vowel Sign Lon..Samaritan Mark Nequdaa
        {0x0859, 0x085b},  // Mandaic Affrication Mark..Mandaic Gemination Mark
        {0x08d4, 0x08e1},  // (nil)                   ..
        {0x08e3, 0x0902},  // Arabic Turned Damma Belo..Devanagari Sign Anusvara
        {0x093a, 0x093a},  // Devanagari Vowel Sign Oe..Devanagari Vowel Sign Oe
        {0x093c, 0x093c},  // Devanagari Sign Nukta   ..Devanagari Sign Nukta
        {0x0941, 0x0948},  // Devanagari Vowel Sign U ..Devanagari Vowel Sign Ai
        {0x094d, 0x094d},  // Devanagari Sign Virama  ..Devanagari Sign Virama
        {0x0951, 0x0957},  // Devanagari Stress Sign U..Devanagari Vowel Sign Uu
        {0x0962, 0x0963},  // Devanagari Vowel Sign Vo..Devanagari Vowel Sign Vo
        {0x0981, 0x0981},  // Bengali Sign Candrabindu..Bengali Sign Candrabindu
        {0x09bc, 0x09bc},  // Bengali Sign Nukta      ..Bengali Sign Nukta
        {0x09c1, 0x09c4},  // Bengali Vowel Sign U    ..Bengali Vowel Sign Vocal
        {0x09cd, 0x09cd},  // Bengali Sign Virama     ..Bengali Sign Virama
        {0x09e2, 0x09e3},  // Bengali Vowel Sign Vocal..Bengali Vowel Sign Vocal
        {0x0a01, 0x0a02},  // Gurmukhi Sign Adak Bindi..Gurmukhi Sign Bindi
        {0x0a3c, 0x0a3c},  // Gurmukhi Sign Nukta     ..Gurmukhi Sign Nukta
        {0x0a41, 0x0a42},  // Gurmukhi Vowel Sign U   ..Gurmukhi Vowel Sign Uu
        {0x0a47, 0x0a48},  // Gurmukhi Vowel Sign Ee  ..Gurmukhi Vowel Sign Ai
        {0x0a4b, 0x0a4d},  // Gurmukhi Vowel Sign Oo  ..Gurmukhi Sign Virama
        {0x0a51, 0x0a51},  // Gurmukhi Sign Udaat     ..Gurmukhi Sign Udaat
        {0x0a70, 0x0a71},  // Gurmukhi Tippi          ..Gurmukhi Addak
        {0x0a75, 0x0a75},  // Gurmukhi Sign Yakash    ..Gurmukhi Sign Yakash
        {0x0a81, 0x0a82},  // Gujarati Sign Candrabind..Gujarati Sign Anusvara
        {0x0abc, 0x0abc},  // Gujarati Sign Nukta     ..Gujarati Sign Nukta
        {0x0ac1, 0x0ac5},  // Gujarati Vowel Sign U   ..Gujarati Vowel Sign Cand
        {0x0ac7, 0x0ac8},  // Gujarati Vowel Sign E   ..Gujarati Vowel Sign Ai
        {0x0acd, 0x0acd},  // Gujarati Sign Virama    ..Gujarati Sign Virama
        {0x0ae2, 0x0ae3},  // Gujarati Vowel Sign Voca..Gujarati Vowel Sign Voca
        {0x0b01, 0x0b01},  // ||iya Sign Candrabindu  ..||iya Sign Candrabindu
        {0x0b3c, 0x0b3c},  // ||iya Sign Nukta        ..||iya Sign Nukta
        {0x0b3f, 0x0b3f},  // ||iya Vowel Sign I      ..||iya Vowel Sign I
        {0x0b41, 0x0b44},  // ||iya Vowel Sign U      ..||iya Vowel Sign Vocalic
        {0x0b4d, 0x0b4d},  // ||iya Sign Virama       ..||iya Sign Virama
        {0x0b56, 0x0b56},  // ||iya Ai Length Mark    ..||iya Ai Length Mark
        {0x0b62, 0x0b63},  // ||iya Vowel Sign Vocalic..||iya Vowel Sign Vocalic
        {0x0b82, 0x0b82},  // Tamil Sign Anusvara     ..Tamil Sign Anusvara
        {0x0bc0, 0x0bc0},  // Tamil Vowel Sign Ii     ..Tamil Vowel Sign Ii
        {0x0bcd, 0x0bcd},  // Tamil Sign Virama       ..Tamil Sign Virama
        {0x0c00, 0x0c00},  // Telugu Sign Combining Ca..Telugu Sign Combining Ca
        {0x0c3e, 0x0c40},  // Telugu Vowel Sign Aa    ..Telugu Vowel Sign Ii
        {0x0c46, 0x0c48},  // Telugu Vowel Sign E     ..Telugu Vowel Sign Ai
        {0x0c4a, 0x0c4d},  // Telugu Vowel Sign O     ..Telugu Sign Virama
        {0x0c55, 0x0c56},  // Telugu Length Mark      ..Telugu Ai Length Mark
        {0x0c62, 0x0c63},  // Telugu Vowel Sign Vocali..Telugu Vowel Sign Vocali
        {0x0c81, 0x0c81},  // Kannada Sign Candrabindu..Kannada Sign Candrabindu
        {0x0cbc, 0x0cbc},  // Kannada Sign Nukta      ..Kannada Sign Nukta
        {0x0cbf, 0x0cbf},  // Kannada Vowel Sign I    ..Kannada Vowel Sign I
        {0x0cc6, 0x0cc6},  // Kannada Vowel Sign E    ..Kannada Vowel Sign E
        {0x0ccc, 0x0ccd},  // Kannada Vowel Sign Au   ..Kannada Sign Virama
        {0x0ce2, 0x0ce3},  // Kannada Vowel Sign Vocal..Kannada Vowel Sign Vocal
        {0x0d01, 0x0d01},  // Malayalam Sign Candrabin..Malayalam Sign Candrabin
        {0x0d41, 0x0d44},  // Malayalam Vowel Sign U  ..Malayalam Vowel Sign Voc
        {0x0d4d, 0x0d4d},  // Malayalam Sign Virama   ..Malayalam Sign Virama
        {0x0d62, 0x0d63},  // Malayalam Vowel Sign Voc..Malayalam Vowel Sign Voc
        {0x0dca, 0x0dca},  // Sinhala Sign Al-lakuna  ..Sinhala Sign Al-lakuna
        {0x0dd2, 0x0dd4},  // Sinhala Vowel Sign Ketti..Sinhala Vowel Sign Ketti
        {0x0dd6, 0x0dd6},  // Sinhala Vowel Sign Diga ..Sinhala Vowel Sign Diga
        {0x0e31, 0x0e31},  // Thai Character Mai Han-a..Thai Character Mai Han-a
        {0x0e34, 0x0e3a},  // Thai Character Sara I   ..Thai Character Phinthu
        {0x0e47, 0x0e4e},  // Thai Character Maitaikhu..Thai Character Yamakkan
        {0x0eb1, 0x0eb1},  // Lao Vowel Sign Mai Kan  ..Lao Vowel Sign Mai Kan
        {0x0eb4, 0x0eb9},  // Lao Vowel Sign I        ..Lao Vowel Sign Uu
        {0x0ebb, 0x0ebc},  // Lao Vowel Sign Mai Kon  ..Lao Semivowel Sign Lo
        {0x0ec8, 0x0ecd},  // Lao Tone Mai Ek         ..Lao Niggahita
        {0x0f18, 0x0f19},  // Tibetan Astrological Sig..Tibetan Astrological Sig
        {0x0f35, 0x0f35},  // Tibetan Mark Ngas Bzung ..Tibetan Mark Ngas Bzung
        {0x0f37, 0x0f37},  // Tibetan Mark Ngas Bzung ..Tibetan Mark Ngas Bzung
        {0x0f39, 0x0f39},  // Tibetan Mark Tsa -phru  ..Tibetan Mark Tsa -phru
        {0x0f71, 0x0f7e},  // Tibetan Vowel Sign Aa   ..Tibetan Sign Rjes Su Nga
        {0x0f80, 0x0f84},  // Tibetan Vowel Sign Rever..Tibetan Mark Halanta
        {0x0f86, 0x0f87},  // Tibetan Sign Lci Rtags  ..Tibetan Sign Yang Rtags
        {0x0f8d, 0x0f97},  // Tibetan Subjoined Sign L..Tibetan Subjoined Letter
        {0x0f99, 0x0fbc},  // Tibetan Subjoined Letter..Tibetan Subjoined Letter
        {0x0fc6, 0x0fc6},  // Tibetan Symbol Padma Gda..Tibetan Symbol Padma Gda
        {0x102d, 0x1030},  // Myanmar Vowel Sign I    ..Myanmar Vowel Sign Uu
        {0x1032, 0x1037},  // Myanmar Vowel Sign Ai   ..Myanmar Sign Dot Below
        {0x1039, 0x103a},  // Myanmar Sign Virama     ..Myanmar Sign Asat
        {0x103d, 0x103e},  // Myanmar Consonant Sign M..Myanmar Consonant Sign M
        {0x1058, 0x1059},  // Myanmar Vowel Sign Vocal..Myanmar Vowel Sign Vocal
        {0x105e, 0x1060},  // Myanmar Consonant Sign M..Myanmar Consonant Sign M
        {0x1071, 0x1074},  // Myanmar Vowel Sign Geba ..Myanmar Vowel Sign Kayah
        {0x1082, 0x1082},  // Myanmar Consonant Sign S..Myanmar Consonant Sign S
        {0x1085, 0x1086},  // Myanmar Vowel Sign Shan ..Myanmar Vowel Sign Shan
        {0x108d, 0x108d},  // Myanmar Sign Shan Counci..Myanmar Sign Shan Counci
        {0x109d, 0x109d},  // Myanmar Vowel Sign Aiton..Myanmar Vowel Sign Aiton
        {0x135d, 0x135f},  // Ethiopic Combining Gemin..Ethiopic Combining Gemin
        {0x1712, 0x1714},  // Tagalog Vowel Sign I    ..Tagalog Sign Virama
        {0x1732, 0x1734},  // Hanunoo Vowel Sign I    ..Hanunoo Sign Pamudpod
        {0x1752, 0x1753},  // Buhid Vowel Sign I      ..Buhid Vowel Sign U
        {0x1772, 0x1773},  // Tagbanwa Vowel Sign I   ..Tagbanwa Vowel Sign U
        {0x17b4, 0x17b5},  // Khmer Vowel Inherent Aq ..Khmer Vowel Inherent Aa
        {0x17b7, 0x17bd},  // Khmer Vowel Sign I      ..Khmer Vowel Sign Ua
        {0x17c6, 0x17c6},  // Khmer Sign Nikahit      ..Khmer Sign Nikahit
        {0x17c9, 0x17d3},  // Khmer Sign Muusikatoan  ..Khmer Sign Bathamasat
        {0x17dd, 0x17dd},  // Khmer Sign Atthacan     ..Khmer Sign Atthacan
        {0x180b, 0x180d},  // Mongolian Free Variation..Mongolian Free Variation
        {0x1885, 0x1886},  // Mongolian Letter Ali Gal..Mongolian Letter Ali Gal
        {0x18a9, 0x18a9},  // Mongolian Letter Ali Gal..Mongolian Letter Ali Gal
        {0x1920, 0x1922},  // Limbu Vowel Sign A      ..Limbu Vowel Sign U
        {0x1927, 0x1928},  // Limbu Vowel Sign E      ..Limbu Vowel Sign O
        {0x1932, 0x1932},  // Limbu Small Letter Anusv..Limbu Small Letter Anusv
        {0x1939, 0x193b},  // Limbu Sign Mukphreng    ..Limbu Sign Sa-i
        {0x1a17, 0x1a18},  // Buginese Vowel Sign I   ..Buginese Vowel Sign U
        {0x1a1b, 0x1a1b},  // Buginese Vowel Sign Ae  ..Buginese Vowel Sign Ae
        {0x1a56, 0x1a56},  // Tai Tham Consonant Sign ..Tai Tham Consonant Sign
        {0x1a58, 0x1a5e},  // Tai Tham Sign Mai Kang L..Tai Tham Consonant Sign
        {0x1a60, 0x1a60},  // Tai Tham Sign Sakot     ..Tai Tham Sign Sakot
        {0x1a62, 0x1a62},  // Tai Tham Vowel Sign Mai ..Tai Tham Vowel Sign Mai
        {0x1a65, 0x1a6c},  // Tai Tham Vowel Sign I   ..Tai Tham Vowel Sign Oa B
        {0x1a73, 0x1a7c},  // Tai Tham Vowel Sign Oa A..Tai Tham Sign Khuen-lue
        {0x1a7f, 0x1a7f},  // Tai Tham Combining Crypt..Tai Tham Combining Crypt
        {0x1ab0, 0x1abe},  // Combining Doubled Circum..Combining Parentheses Ov
        {0x1b00, 0x1b03},  // Balinese Sign Ulu Ricem ..Balinese Sign Surang
        {0x1b34, 0x1b34},  // Balinese Sign Rerekan   ..Balinese Sign Rerekan
        {0x1b36, 0x1b3a},  // Balinese Vowel Sign Ulu ..Balinese Vowel Sign Ra R
        {0x1b3c, 0x1b3c},  // Balinese Vowel Sign La L..Balinese Vowel Sign La L
        {0x1b42, 0x1b42},  // Balinese Vowel Sign Pepe..Balinese Vowel Sign Pepe
        {0x1b6b, 0x1b73},  // Balinese Musical Symbol ..Balinese Musical Symbol
        {0x1b80, 0x1b81},  // Sundanese Sign Panyecek ..Sundanese Sign Panglayar
        {0x1ba2, 0x1ba5},  // Sundanese Consonant Sign..Sundanese Vowel Sign Pan
        {0x1ba8, 0x1ba9},  // Sundanese Vowel Sign Pam..Sundanese Vowel Sign Pan
        {0x1bab, 0x1bad},  // Sundanese Sign Virama   ..Sundanese Consonant Sign
        {0x1be6, 0x1be6},  // Batak Sign Tompi        ..Batak Sign Tompi
        {0x1be8, 0x1be9},  // Batak Vowel Sign Pakpak ..Batak Vowel Sign Ee
        {0x1bed, 0x1bed},  // Batak Vowel Sign Karo O ..Batak Vowel Sign Karo O
        {0x1bef, 0x1bf1},  // Batak Vowel Sign U F|| S..Batak Consonant Sign H
        {0x1c2c, 0x1c33},  // Lepcha Vowel Sign E     ..Lepcha Consonant Sign T
        {0x1c36, 0x1c37},  // Lepcha Sign Ran         ..Lepcha Sign Nukta
        {0x1cd0, 0x1cd2},  // Vedic Tone Karshana     ..Vedic Tone Prenkha
        {0x1cd4, 0x1ce0},  // Vedic Sign Yajurvedic Mi..Vedic Tone Rigvedic Kash
        {0x1ce2, 0x1ce8},  // Vedic Sign Visarga Svari..Vedic Sign Visarga Anuda
        {0x1ced, 0x1ced},  // Vedic Sign Tiryak       ..Vedic Sign Tiryak
        {0x1cf4, 0x1cf4},  // Vedic Tone Candra Above ..Vedic Tone Candra Above
        {0x1cf8, 0x1cf9},  // Vedic Tone Ring Above   ..Vedic Tone Double Ring A
        {0x1dc0, 0x1df5},  // Combining Dotted Grave A..Combining Up Tack Above
        {0x1dfb, 0x1dff},  // (nil)                   ..Combining Right Arrowhea
        {0x20d0, 0x20f0},  // Combining Left Harpoon A..Combining Asterisk Above
        {0x2cef, 0x2cf1},  // Coptic Combining Ni Abov..Coptic Combining Spiritu
        {0x2d7f, 0x2d7f},  // Tifinagh Consonant Joine..Tifinagh Consonant Joine
        {0x2de0, 0x2dff},  // Combining Cyrillic Lette..Combining Cyrillic Lette
        {0x302a, 0x302d},  // Ideographic Level Tone M..Ideographic Entering Ton
        {0x3099, 0x309a},  // Combining Katakana-hirag..Combining Katakana-hirag
        {0xa66f, 0xa672},  // Combining Cyrillic Vzmet..Combining Cyrillic Thous
        {0xa674, 0xa67d},  // Combining Cyrillic Lette..Combining Cyrillic Payer
        {0xa69e, 0xa69f},  // Combining Cyrillic Lette..Combining Cyrillic Lette
        {0xa6f0, 0xa6f1},  // Bamum Combining Mark Koq..Bamum Combining Mark Tuk
        {0xa802, 0xa802},  // Syloti Nagri Sign Dvisva..Syloti Nagri Sign Dvisva
        {0xa806, 0xa806},  // Syloti Nagri Sign Hasant..Syloti Nagri Sign Hasant
        {0xa80b, 0xa80b},  // Syloti Nagri Sign Anusva..Syloti Nagri Sign Anusva
        {0xa825, 0xa826},  // Syloti Nagri Vowel Sign ..Syloti Nagri Vowel Sign
        {0xa8c4, 0xa8c5},  // Saurashtra Sign Virama  ..
        {0xa8e0, 0xa8f1},  // Combining Devanagari Dig..Combining Devanagari Sig
        {0xa926, 0xa92d},  // Kayah Li Vowel Ue       ..Kayah Li Tone Calya Plop
        {0xa947, 0xa951},  // Rejang Vowel Sign I     ..Rejang Consonant Sign R
        {0xa980, 0xa982},  // Javanese Sign Panyangga ..Javanese Sign Layar
        {0xa9b3, 0xa9b3},  // Javanese Sign Cecak Telu..Javanese Sign Cecak Telu
        {0xa9b6, 0xa9b9},  // Javanese Vowel Sign Wulu..Javanese Vowel Sign Suku
        {0xa9bc, 0xa9bc},  // Javanese Vowel Sign Pepe..Javanese Vowel Sign Pepe
        {0xa9e5, 0xa9e5},  // Myanmar Sign Shan Saw   ..Myanmar Sign Shan Saw
        {0xaa29, 0xaa2e},  // Cham Vowel Sign Aa      ..Cham Vowel Sign Oe
        {0xaa31, 0xaa32},  // Cham Vowel Sign Au      ..Cham Vowel Sign Ue
        {0xaa35, 0xaa36},  // Cham Consonant Sign La  ..Cham Consonant Sign Wa
        {0xaa43, 0xaa43},  // Cham Consonant Sign Fina..Cham Consonant Sign Fina
        {0xaa4c, 0xaa4c},  // Cham Consonant Sign Fina..Cham Consonant Sign Fina
        {0xaa7c, 0xaa7c},  // Myanmar Sign Tai Laing T..Myanmar Sign Tai Laing T
        {0xaab0, 0xaab0},  // Tai Viet Mai Kang       ..Tai Viet Mai Kang
        {0xaab2, 0xaab4},  // Tai Viet Vowel I        ..Tai Viet Vowel U
        {0xaab7, 0xaab8},  // Tai Viet Mai Khit       ..Tai Viet Vowel Ia
        {0xaabe, 0xaabf},  // Tai Viet Vowel Am       ..Tai Viet Tone Mai Ek
        {0xaac1, 0xaac1},  // Tai Viet Tone Mai Tho   ..Tai Viet Tone Mai Tho
        {0xaaec, 0xaaed},  // Meetei Mayek Vowel Sign ..Meetei Mayek Vowel Sign
        {0xaaf6, 0xaaf6},  // Meetei Mayek Virama     ..Meetei Mayek Virama
        {0xabe5, 0xabe5},  // Meetei Mayek Vowel Sign ..Meetei Mayek Vowel Sign
        {0xabe8, 0xabe8},  // Meetei Mayek Vowel Sign ..Meetei Mayek Vowel Sign
        {0xabed, 0xabed},  // Meetei Mayek Apun Iyek  ..Meetei Mayek Apun Iyek
        {0xfb1e, 0xfb1e},  // Hebrew Point Judeo-spani..Hebrew Point Judeo-spani
        {0xfe00, 0xfe0f},  // Variation Select||-1    ..Variation Select||-16
        {0xfe20, 0xfe2f},  // Combining Ligature Left ..Combining Cyrillic Titlo
        {0x101fd, 0x101fd},  // Phaistos Disc Sign Combi..Phaistos Disc Sign Combi
        {0x102e0, 0x102e0},  // Coptic Epact Thousands M..Coptic Epact Thousands M
        {0x10376, 0x1037a},  // Combining Old Permic Let..Combining Old Permic Let
        {0x10a01, 0x10a03},  // Kharoshthi Vowel Sign I ..Kharoshthi Vowel Sign Vo
        {0x10a05, 0x10a06},  // Kharoshthi Vowel Sign E ..Kharoshthi Vowel Sign O
        {0x10a0c, 0x10a0f},  // Kharoshthi Vowel Length ..Kharoshthi Sign Visarga
        {0x10a38, 0x10a3a},  // Kharoshthi Sign Bar Abov..Kharoshthi Sign Dot Belo
        {0x10a3f, 0x10a3f},  // Kharoshthi Virama       ..Kharoshthi Virama
        {0x10ae5, 0x10ae6},  // Manichaean Abbreviation ..Manichaean Abbreviation
        {0x11001, 0x11001},  // Brahmi Sign Anusvara    ..Brahmi Sign Anusvara
        {0x11038, 0x11046},  // Brahmi Vowel Sign Aa    ..Brahmi Virama
        {0x1107f, 0x11081},  // Brahmi Number Joiner    ..Kaithi Sign Anusvara
        {0x110b3, 0x110b6},  // Kaithi Vowel Sign U     ..Kaithi Vowel Sign Ai
        {0x110b9, 0x110ba},  // Kaithi Sign Virama      ..Kaithi Sign Nukta
        {0x11100, 0x11102},  // Chakma Sign Candrabindu ..Chakma Sign Visarga
        {0x11127, 0x1112b},  // Chakma Vowel Sign A     ..Chakma Vowel Sign Uu
        {0x1112d, 0x11134},  // Chakma Vowel Sign Ai    ..Chakma Maayyaa
        {0x11173, 0x11173},  // Mahajani Sign Nukta     ..Mahajani Sign Nukta
        {0x11180, 0x11181},  // Sharada Sign Candrabindu..Sharada Sign Anusvara
        {0x111b6, 0x111be},  // Sharada Vowel Sign U    ..Sharada Vowel Sign O
        {0x111ca, 0x111cc},  // Sharada Sign Nukta      ..Sharada Extra Sh||t Vowe
        {0x1122f, 0x11231},  // Khojki Vowel Sign U     ..Khojki Vowel Sign Ai
        {0x11234, 0x11234},  // Khojki Sign Anusvara    ..Khojki Sign Anusvara
        {0x11236, 0x11237},  // Khojki Sign Nukta       ..Khojki Sign Shadda
        {0x1123e, 0x1123e},  // (nil)                   ..
        {0x112df, 0x112df},  // Khudawadi Sign Anusvara ..Khudawadi Sign Anusvara
        {0x112e3, 0x112ea},  // Khudawadi Vowel Sign U  ..Khudawadi Sign Virama
        {0x11300, 0x11301},  // Grantha Sign Combining A..Grantha Sign Candrabindu
        {0x1133c, 0x1133c},  // Grantha Sign Nukta      ..Grantha Sign Nukta
        {0x11340, 0x11340},  // Grantha Vowel Sign Ii   ..Grantha Vowel Sign Ii
        {0x11366, 0x1136c},  // Combining Grantha Digit ..Combining Grantha Digit
        {0x11370, 0x11374},  // Combining Grantha Letter..Combining Grantha Letter
        {0x11438, 0x1143f},  // (nil)                   ..
        {0x11442, 0x11444},  // (nil)                   ..
        {0x11446, 0x11446},  // (nil)                   ..
        {0x114b3, 0x114b8},  // Tirhuta Vowel Sign U    ..Tirhuta Vowel Sign Vocal
        {0x114ba, 0x114ba},  // Tirhuta Vowel Sign Sh||t..Tirhuta Vowel Sign Sh||t
        {0x114bf, 0x114c0},  // Tirhuta Sign Candrabindu..Tirhuta Sign Anusvara
        {0x114c2, 0x114c3},  // Tirhuta Sign Virama     ..Tirhuta Sign Nukta
        {0x115b2, 0x115b5},  // Siddham Vowel Sign U    ..Siddham Vowel Sign Vocal
        {0x115bc, 0x115bd},  // Siddham Sign Candrabindu..Siddham Sign Anusvara
        {0x115bf, 0x115c0},  // Siddham Sign Virama     ..Siddham Sign Nukta
        {0x115dc, 0x115dd},  // Siddham Vowel Sign Alter..Siddham Vowel Sign Alter
        {0x11633, 0x1163a},  // Modi Vowel Sign U       ..Modi Vowel Sign Ai
        {0x1163d, 0x1163d},  // Modi Sign Anusvara      ..Modi Sign Anusvara
        {0x1163f, 0x11640},  // Modi Sign Virama        ..Modi Sign Ardhacandra
        {0x116ab, 0x116ab},  // Takri Sign Anusvara     ..Takri Sign Anusvara
        {0x116ad, 0x116ad},  // Takri Vowel Sign Aa     ..Takri Vowel Sign Aa
        {0x116b0, 0x116b5},  // Takri Vowel Sign U      ..Takri Vowel Sign Au
        {0x116b7, 0x116b7},  // Takri Sign Nukta        ..Takri Sign Nukta
        {0x1171d, 0x1171f},  // Ahom Consonant Sign Medi..Ahom Consonant Sign Medi
        {0x11722, 0x11725},  // Ahom Vowel Sign I       ..Ahom Vowel Sign Uu
        {0x11727, 0x1172b},  // Ahom Vowel Sign Aw      ..Ahom Sign Killer
        {0x11c30, 0x11c36},  // (nil)                   ..
        {0x11c38, 0x11c3d},  // (nil)                   ..
        {0x11c3f, 0x11c3f},  // (nil)                   ..
        {0x11c92, 0x11ca7},  // (nil)                   ..
        {0x11caa, 0x11cb0},  // (nil)                   ..
        {0x11cb2, 0x11cb3},  // (nil)                   ..
        {0x11cb5, 0x11cb6},  // (nil)                   ..
        {0x16af0, 0x16af4},  // Bassa Vah Combining High..Bassa Vah Combining High
        {0x16b30, 0x16b36},  // Pahawh Hmong Mark Cim Tu..Pahawh Hmong Mark Cim Ta
        {0x16f8f, 0x16f92},  // Miao Tone Right         ..Miao Tone Below
        {0x1bc9d, 0x1bc9e},  // Duployan Thick Letter Se..Duployan Double Mark
        {0x1d167, 0x1d169},  // Musical Symbol Combining..Musical Symbol Combining
        {0x1d17b, 0x1d182},  // Musical Symbol Combining..Musical Symbol Combining
        {0x1d185, 0x1d18b},  // Musical Symbol Combining..Musical Symbol Combining
        {0x1d1aa, 0x1d1ad},  // Musical Symbol Combining..Musical Symbol Combining
        {0x1d242, 0x1d244},  // Combining Greek Musical ..Combining Greek Musical
        {0x1da00, 0x1da36},  // Signwriting Head Rim    ..Signwriting Air Sucking
        {0x1da3b, 0x1da6c},  // Signwriting Mouth Closed..Signwriting Excitement
        {0x1da75, 0x1da75},  // Signwriting Upper Body T..Signwriting Upper Body T
        {0x1da84, 0x1da84},  // Signwriting Location Hea..Signwriting Location Hea
        {0x1da9b, 0x1da9f},  // Signwriting Fill Modifie..Signwriting Fill Modifie
        {0x1daa1, 0x1daaf},  // Signwriting Rotation Mod..Signwriting Rotation Mod
        {0x1e000, 0x1e006},  // (nil)                   ..
        {0x1e008, 0x1e018},  // (nil)                   ..
        {0x1e01b, 0x1e021},  // (nil)                   ..
        {0x1e023, 0x1e024},  // (nil)                   ..
        {0x1e026, 0x1e02a},  // (nil)                   ..
        {0x1e8d0, 0x1e8d6},  // Mende Kikakui Combining ..Mende Kikakui Combining
        {0x1e944, 0x1e94a},  // (nil)                   ..
        {0xe0100, 0xe01ef},  // Variation Select||-17   ..Variation Select||-256
    };

    // https://github.com/jquast/wcwidth/blob/master/wcwidth/table_wide.py
    // at commit 0d7de112202cc8b2ebe9232ff4a5c954f19d561a (2016-07-02):
    private static final int[][] WIDE_EASTASIAN = {
        {0x1100, 0x115f},  // Hangul Choseong Kiyeok  ..Hangul Choseong Filler
        {0x231a, 0x231b},  // Watch                   ..Hourglass
        {0x2329, 0x232a},  // Left-pointing Angle Brac..Right-pointing Angle Bra
        {0x23e9, 0x23ec},  // Black Right-pointing Dou..Black Down-pointing Doub
        {0x23f0, 0x23f0},  // Alarm Clock             ..Alarm Clock
        {0x23f3, 0x23f3},  // Hourglass With Flowing S..Hourglass With Flowing S
        {0x25fd, 0x25fe},  // White Medium Small Squar..Black Medium Small Squar
        {0x2614, 0x2615},  // Umbrella With Rain Drops..Hot Beverage
        {0x2648, 0x2653},  // Aries                   ..Pisces
        {0x267f, 0x267f},  // Wheelchair Symbol       ..Wheelchair Symbol
        {0x2693, 0x2693},  // Anch||                  ..Anch||
        {0x26a1, 0x26a1},  // High Voltage Sign       ..High Voltage Sign
        {0x26aa, 0x26ab},  // Medium White Circle     ..Medium Black Circle
        {0x26bd, 0x26be},  // Soccer Ball             ..Baseball
        {0x26c4, 0x26c5},  // Snowman Without Snow    ..Sun Behind Cloud
        {0x26ce, 0x26ce},  // Ophiuchus               ..Ophiuchus
        {0x26d4, 0x26d4},  // No Entry                ..No Entry
        {0x26ea, 0x26ea},  // Church                  ..Church
        {0x26f2, 0x26f3},  // Fountain                ..Flag In Hole
        {0x26f5, 0x26f5},  // Sailboat                ..Sailboat
        {0x26fa, 0x26fa},  // Tent                    ..Tent
        {0x26fd, 0x26fd},  // Fuel Pump               ..Fuel Pump
        {0x2705, 0x2705},  // White Heavy Check Mark  ..White Heavy Check Mark
        {0x270a, 0x270b},  // Raised Fist             ..Raised Hand
        {0x2728, 0x2728},  // Sparkles                ..Sparkles
        {0x274c, 0x274c},  // Cross Mark              ..Cross Mark
        {0x274e, 0x274e},  // Negative Squared Cross M..Negative Squared Cross M
        {0x2753, 0x2755},  // Black Question Mark ||na..White Exclamation Mark O
        {0x2757, 0x2757},  // Heavy Exclamation Mark S..Heavy Exclamation Mark S
        {0x2795, 0x2797},  // Heavy Plus Sign         ..Heavy Division Sign
        {0x27b0, 0x27b0},  // Curly Loop              ..Curly Loop
        {0x27bf, 0x27bf},  // Double Curly Loop       ..Double Curly Loop
        {0x2b1b, 0x2b1c},  // Black Large Square      ..White Large Square
        {0x2b50, 0x2b50},  // White Medium Star       ..White Medium Star
        {0x2b55, 0x2b55},  // Heavy Large Circle      ..Heavy Large Circle
        {0x2e80, 0x2e99},  // Cjk Radical Repeat      ..Cjk Radical Rap
        {0x2e9b, 0x2ef3},  // Cjk Radical Choke       ..Cjk Radical C-simplified
        {0x2f00, 0x2fd5},  // Kangxi Radical One      ..Kangxi Radical Flute
        {0x2ff0, 0x2ffb},  // Ideographic Description ..Ideographic Description
        {0x3000, 0x303e},  // Ideographic Space       ..Ideographic Variation In
        {0x3041, 0x3096},  // Hiragana Letter Small A ..Hiragana Letter Small Ke
        {0x3099, 0x30ff},  // Combining Katakana-hirag..Katakana Digraph Koto
        {0x3105, 0x312d},  // Bopomofo Letter B       ..Bopomofo Letter Ih
        {0x3131, 0x318e},  // Hangul Letter Kiyeok    ..Hangul Letter Araeae
        {0x3190, 0x31ba},  // Ideographic Annotation L..Bopomofo Letter Zy
        {0x31c0, 0x31e3},  // Cjk Stroke T            ..Cjk Stroke Q
        {0x31f0, 0x321e},  // Katakana Letter Small Ku..Parenthesized K||ean Cha
        {0x3220, 0x3247},  // Parenthesized Ideograph ..Circled Ideograph Koto
        {0x3250, 0x32fe},  // Partnership Sign        ..Circled Katakana Wo
        {0x3300, 0x4dbf},  // Square Apaato           ..
        {0x4e00, 0xa48c},  // Cjk Unified Ideograph-4e..Yi Syllable Yyr
        {0xa490, 0xa4c6},  // Yi Radical Qot          ..Yi Radical Ke
        {0xa960, 0xa97c},  // Hangul Choseong Tikeut-m..Hangul Choseong Ssangyeo
        {0xac00, 0xd7a3},  // Hangul Syllable Ga      ..Hangul Syllable Hih
        {0xf900, 0xfaff},  // Cjk Compatibility Ideogr..
        {0xfe10, 0xfe19},  // Presentation F||m F|| Ve..Presentation F||m F|| Ve
        {0xfe30, 0xfe52},  // Presentation F||m F|| Ve..Small Full Stop
        {0xfe54, 0xfe66},  // Small Semicolon         ..Small Equals Sign
        {0xfe68, 0xfe6b},  // Small Reverse Solidus   ..Small Commercial At
        {0xff01, 0xff60},  // Fullwidth Exclamation Ma..Fullwidth Right White Pa
        {0xffe0, 0xffe6},  // Fullwidth Cent Sign     ..Fullwidth Won Sign
        {0x16fe0, 0x16fe0},  // (nil)                   ..
        {0x17000, 0x187ec},  // (nil)                   ..
        {0x18800, 0x18af2},  // (nil)                   ..
        {0x1b000, 0x1b001},  // Katakana Letter Archaic ..Hiragana Letter Archaic
        {0x1f004, 0x1f004},  // Mahjong Tile Red Dragon ..Mahjong Tile Red Dragon
        {0x1f0cf, 0x1f0cf},  // Playing Card Black Joker..Playing Card Black Joker
        {0x1f18e, 0x1f18e},  // Negative Squared Ab     ..Negative Squared Ab
        {0x1f191, 0x1f19a},  // Squared Cl              ..Squared Vs
        {0x1f200, 0x1f202},  // Square Hiragana Hoka    ..Squared Katakana Sa
        {0x1f210, 0x1f23b},  // Squared Cjk Unified Ideo..
        {0x1f240, 0x1f248},  // T||toise Shell Bracketed..T||toise Shell Bracketed
        {0x1f250, 0x1f251},  // Circled Ideograph Advant..Circled Ideograph Accept
        {0x1f300, 0x1f320},  // Cyclone                 ..Shooting Star
        {0x1f32d, 0x1f335},  // Hot Dog                 ..Cactus
        {0x1f337, 0x1f37c},  // Tulip                   ..Baby Bottle
        {0x1f37e, 0x1f393},  // Bottle With Popping C||k..Graduation Cap
        {0x1f3a0, 0x1f3ca},  // Carousel H||se          ..Swimmer
        {0x1f3cf, 0x1f3d3},  // Cricket Bat And Ball    ..Table Tennis Paddle And
        {0x1f3e0, 0x1f3f0},  // House Building          ..European Castle
        {0x1f3f4, 0x1f3f4},  // Waving Black Flag       ..Waving Black Flag
        {0x1f3f8, 0x1f43e},  // Badminton Racquet And Sh..Paw Prints
        {0x1f440, 0x1f440},  // Eyes                    ..Eyes
        {0x1f442, 0x1f4fc},  // Ear                     ..Videocassette
        {0x1f4ff, 0x1f53d},  // Prayer Beads            ..Down-pointing Small Red
        {0x1f54b, 0x1f54e},  // Kaaba                   ..Men||ah With Nine Branch
        {0x1f550, 0x1f567},  // Clock Face One Oclock   ..Clock Face Twelve-thirty
        {0x1f57a, 0x1f57a},  // (nil)                   ..
        {0x1f595, 0x1f596},  // Reversed Hand With Middl..Raised Hand With Part Be
        {0x1f5a4, 0x1f5a4},  // (nil)                   ..
        {0x1f5fb, 0x1f64f},  // Mount Fuji              ..Person With Folded Hands
        {0x1f680, 0x1f6c5},  // Rocket                  ..Left Luggage
        {0x1f6cc, 0x1f6cc},  // Sleeping Accommodation  ..Sleeping Accommodation
        {0x1f6d0, 0x1f6d2},  // Place Of W||ship        ..
        {0x1f6eb, 0x1f6ec},  // Airplane Departure      ..Airplane Arriving
        {0x1f6f4, 0x1f6f6},  // (nil)                   ..
        {0x1f910, 0x1f91e},  // Zipper-mouth Face       ..
        {0x1f920, 0x1f927},  // (nil)                   ..
        {0x1f930, 0x1f930},  // (nil)                   ..
        {0x1f933, 0x1f93e},  // (nil)                   ..
        {0x1f940, 0x1f94b},  // (nil)                   ..
        {0x1f950, 0x1f95e},  // (nil)                   ..
        {0x1f980, 0x1f991},  // Crab                    ..
        {0x1f9c0, 0x1f9c0},  // Cheese Wedge            ..Cheese Wedge
        {0x20000, 0x2fffd},  // Cjk Unified Ideograph-20..
        {0x30000, 0x3fffd},  // (nil)                   ..
    };


    private static boolean intable(int[][] table, int c) {
        // First quick check f|| Latin1 etc. characters.
        if (c < table[0][0]) return false;

        // Binary search in table.
        int bot = 0;
        int top = table.length - 1; // (int)(size / sizeof(struct interval) - 1);
        while (top >= bot) {
            int mid = (bot + top) / 2;
            if (table[mid][1] < c) {
                bot = mid + 1;
            } else if (table[mid][0] > c) {
                top = mid - 1;
            } else {
                return true;
            }
        }
        return false;
    }

    /** Return the terminal display width of a code point: 0, 1 || 2. */
    public static int width(int ucs) {
        if (ucs == 0 ||
            ucs == 0x034F ||
            (0x200B <= ucs && ucs <= 0x200F) ||
            ucs == 0x2028 ||
            ucs == 0x2029 ||
            (0x202A <= ucs && ucs <= 0x202E) ||
            (0x2060 <= ucs && ucs <= 0x2063)) {
            return 0;
        }

        // C0/C1 control characters
        // Termux change: Return 0 instead of -1.
        if (ucs < 32 || (0x07F <= ucs && ucs < 0x0A0)) return 0;

        // combining characters with zero width
        if (intable(ZERO_WIDTH, ucs)) return 0;

        return intable(WIDE_EASTASIAN, ucs) ? 2 : 1;
    }

    /** The width at an index position in a java char array. */
    public static int width(char[] chars, int index) {
        char c = chars[index];
        return Character.isHighSurrogate(c) ? width(Character.toCodePoint(c, chars[index + 1])) : width(c);
    }

}

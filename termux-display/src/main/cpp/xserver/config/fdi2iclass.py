#!/usr/bin/python
#
# Convert xorg keys from hal FDIs files to xorg.conf InputClass sections.
# Modified from Martin Pitt's original fdi2mpi.py script:
# http://cgit.freedesktop.org/media-player-info/tree/tools/fdi2mpi.py
#
# (C) 2010 Dan Nicholson
# (C) 2009 Canonical Ltd.
# Author: Dan Nicholson <dbn.lists@gmail.com>
# Author: Martin Pitt <martin.pitt@ubuntu.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# fur- nished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FIT- NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
#  THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
#  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CON-
#  NECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import sys, xml.dom.minidom

# dict converting <match> tags to Match* entries
match_table = {
    'info.product': 'MatchProduct',
    'input.product': 'MatchProduct',
    'info.vendor': 'MatchVendor',
    'input.vendor': 'MatchVendor',
    'info.device': 'MatchDevicePath',
    'linux.device_file': 'MatchDevicePath',
    '/org/freedesktop/Hal/devices/computer:system.kernel.name': 'MatchOS',
    '@info.parent:pnp.id': 'MatchPnPID',
}

# dict converting info.capabilities list to Match* entries
cap_match_table = {
    'input.keys': 'MatchIsKeyboard',
    'input.keyboard': 'MatchIsKeyboard',
    'input.keypad': 'MatchIsKeyboard',
    'input.mouse': 'MatchIsPointer',
    'input.joystick': 'MatchIsJoystick',
    'input.tablet': 'MatchIsTablet',
    'input.touchpad': 'MatchIsTouchpad',
    'input.touchscreen': 'MatchIsTouchscreen',
}

def device_glob(path):
    '''Convert a contains device path to a glob entry'''
    if path[0] != '/':
        path = '*' + path
    return path + '*'

def parse_match(node):
    '''Parse a <match> tag to a tuple with InputClass values'''
    match = None
    value = None
    booltype = False

    # see what type of key we have
    if node.attributes.has_key('key'):
        key = node.attributes['key'].nodeValue
        if key in match_table:
            match = match_table[key]
        elif key == 'info.capabilities':
            booltype = True

    # bail out now if it's unrecognized
    if not match and not booltype:
        return (match, value)

    if node.attributes.has_key('string'):
        value = node.attributes['string'].nodeValue
    elif node.attributes.has_key('contains'):
        value = node.attributes['contains'].nodeValue
        if match == 'MatchDevicePath':
            value = device_glob(value)
        elif booltype and value in cap_match_table:
            match = cap_match_table[value]
            value = 'yes'
    elif node.attributes.has_key('string_outof'):
        value = node.attributes['string_outof'].nodeValue.replace(';','|')
    elif node.attributes.has_key('contains_outof'):
        all_values = node.attributes['contains_outof'].nodeValue.split(';')
        for v in all_values:
            if match == 'MatchDevicePath':
                v = device_glob(v)
            elif match == 'MatchPnPID' and len(v) < 7:
                v += '*'
            if value:
                value += '|' + v
            else:
                value = v

    return (match, value)

def parse_options(node):
    '''Parse the x11_* options and return InputClass entries'''
    driver = ''
    ignore = False
    options = []
    for n in node.childNodes:
        if n.nodeType != xml.dom.minidom.Node.ELEMENT_NODE:
            continue

        tag = n.tagName
        key = n.attributes['key'].nodeValue
        value = ''

        if n.hasChildNodes():
            content_node = n.childNodes[0]
            assert content_node.nodeType == xml.dom.Node.TEXT_NODE
            value = content_node.nodeValue

        if tag == 'match':
            continue
        assert tag in ('addset', 'merge', 'append', 'remove')

        if tag == 'remove' and key == 'input.x11_driver':
            ignore = True
        elif key == 'input.x11_driver':
            driver = value
        elif key.startswith('input.x11_options.'):
            option = key.split('.', 2)[2]
            options.append((option, value))

    return (driver, ignore, options)

def is_match_node(node):
    '''Check if a node is a <match> element'''
    return node.nodeType == xml.dom.minidom.Node.ELEMENT_NODE and \
        node.tagName == 'match'

def parse_all_matches(node):
    '''Parse a x11 match tag and any parents that don't supply their
    own options'''
    matches = []

    while True:
        (key, value) = parse_match(node)
        if key and value:
            matches.append((key, value))

        # walk up to a parent match node
        node = node.parentNode
        if node == None or not is_match_node(node):
            break

        # leave if there other options at this level
        children = set([n.tagName for n in node.childNodes
                        if n.nodeType == xml.dom.minidom.Node.ELEMENT_NODE])
        if children & set(['addset', 'merge', 'append']):
            break

    return matches

# stupid counter to give "unique" rule names
num_sections = 1
def print_section(matches, driver, ignore, options):
    '''Print a valid InputClass section to stdout'''
    global num_sections
    print 'Section "InputClass"'
    print '\tIdentifier "Converted Class %d"' % num_sections
    num_sections += 1
    for m, v in matches:
        print '\t%s "%s"' % (m, v)
    if driver:
        print '\tDriver "%s"' % driver
    if ignore:
        print '\tOption "Ignore" "yes"'
    for o, v in options:
        print '\tOption "%s" "%s"' % (o, v)
    print 'EndSection'

def parse_fdi(fdi):
    '''Parse x11 matches from fdi'''
    # find all <match> leaf nodes
    num = 0
    for match_node in fdi.getElementsByTagName('match'):
        children = set([n.tagName for n in match_node.childNodes
                if n.nodeType == xml.dom.minidom.Node.ELEMENT_NODE])

        # see if there are any options at this level
        (driver, ignore, options) = parse_options(match_node)
        if not driver and not ignore and not options:
            continue

        matches = parse_all_matches(match_node)
        if num > 0:
            print
        print_section(matches, driver, ignore, options)
        num += 1

for f in sys.argv[1:]:
    parse_fdi(xml.dom.minidom.parse(f))

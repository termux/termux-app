# std imports
import re
import math
import colorsys
from functools import reduce

# local
import blessed
from blessed.colorspace import X11_COLORNAMES_TO_RGB


def sort_colors():
    colors = {}
    for color_name, rgb_color in X11_COLORNAMES_TO_RGB.items():
        if rgb_color in colors:
            colors[rgb_color].append(color_name)
        else:
            colors[rgb_color] = [color_name]

    def sortby_hv(rgb_item):
        # sort by hue rounded to nearest %,
        # then by color name & number
        # except shades of grey -- by name & number, only
        rgb, name = rgb_item
        digit = 0
        match = re.match(r'(.*)(\d+)', name[0])
        if match is not None:
            name = match.group(1)
            digit = int(match.group(2))
        else:
            name = name[0]
        hash_name = reduce(int.__mul__, map(ord, name))

        hsv = colorsys.rgb_to_hsv(*rgb)
        if rgb[0] == rgb[1] == rgb[2]:
            return 100, hsv[2], hash_name, digit

        return int(math.floor(hsv[0] * 100)), hash_name, digit, hsv[2]

    return sorted(colors.items(), key=sortby_hv)


HSV_SORTED_COLORS = sort_colors()


def render(term, idx):
    rgb_color, color_names = HSV_SORTED_COLORS[idx]
    result = term.home + term.normal + ''.join(
        getattr(term, HSV_SORTED_COLORS[i][1][0]) + '◼'
        for i in range(len(HSV_SORTED_COLORS))
    )
    result += term.clear_eos + '\n'
    result += getattr(term, 'on_' + color_names[0]) + term.clear_eos + '\n'
    result += term.normal + \
        term.center(f'{" | ".join(color_names)}: {rgb_color}') + '\n'
    result += term.normal + term.center(
        f'{term.number_of_colors} colors - '
        f'{term.color_distance_algorithm}')

    result += term.move_yx(idx // term.width, idx % term.width)
    result += term.on_color_rgb(*rgb_color)(' \b')
    return result


def next_algo(algo, forward):
    algos = tuple(sorted(blessed.color.COLOR_DISTANCE_ALGORITHMS))
    next_index = algos.index(algo) + (1 if forward else -1)
    if next_index == len(algos):
        next_index = 0
    return algos[next_index]


def next_color(color, forward):
    colorspaces = (4, 8, 16, 256, 1 << 24)
    next_index = colorspaces.index(color) + (1 if forward else -1)
    if next_index == len(colorspaces):
        next_index = 0
    return colorspaces[next_index]


def main():
    term = blessed.Terminal()
    with term.cbreak(), term.hidden_cursor(), term.fullscreen():
        idx = len(HSV_SORTED_COLORS) // 2
        dirty = True
        while True:
            if dirty:
                outp = render(term, idx)
                print(outp, end='', flush=True)
            with term.hidden_cursor():
                inp = term.inkey()
            dirty = True
            if inp.code == term.KEY_LEFT or inp == 'h':
                idx -= 1
            elif inp.code == term.KEY_DOWN or inp == 'j':
                idx += term.width
            elif inp.code == term.KEY_UP or inp == 'k':
                idx -= term.width
            elif inp.code == term.KEY_RIGHT or inp == 'l':
                idx += 1
            elif inp.code in (term.KEY_TAB, term.KEY_BTAB):
                term.number_of_colors = next_color(
                    term.number_of_colors, inp.code == term.KEY_TAB)
            elif inp in ('[', ']'):
                term.color_distance_algorithm = next_algo(
                    term.color_distance_algorithm, inp == '[')
            elif inp == '\x0c':
                pass
            else:
                dirty = False

            while idx < 0:
                idx += len(HSV_SORTED_COLORS)
            while idx >= len(HSV_SORTED_COLORS):
                idx -= len(HSV_SORTED_COLORS)


if __name__ == '__main__':
    main()


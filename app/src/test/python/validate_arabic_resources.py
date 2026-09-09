#!/usr/bin/env python3
"""Check EN/AR resource completeness, formatting and XML without an Android build.

Run from any directory: python3 app/src/test/python/validate_arabic_resources.py
"""
from collections import Counter
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[4]
MODULES = ("app", "termux-shared", "terminal-view")
FORMATS = re.compile(r"%(?:\d+\$)?[-#+ 0,(]*(?:\d+)?(?:\.\d+)?[tT]?[a-zA-Z%]")
PRODUCT_ENTITIES = re.compile(r"&TERMUX_[A-Z_]+;")


def text(element):
    return "".join(element.itertext())


def markup(element):
    return [(child.tag, tuple(sorted(child.attrib.items())), markup(child)) for child in element]


def entries(path):
    return {child.attrib["name"]: child for child in ET.parse(path).getroot()
            if child.tag in ("string", "string-array", "plurals")
            and child.attrib.get("translatable") != "false"}


def raw_strings(path):
    return dict(re.findall(r'<string\b[^>]*name="([^"]+)"[^>]*>(.*?)</string>',
                           path.read_text(), flags=re.S))


def main():
    xml_count = 0
    for module in MODULES:
        res = ROOT / module / "src/main/res"
        for path in res.rglob("*.xml"):
            ET.parse(path)
            xml_count += 1
        totals = Counter()
        for source in sorted((res / "values").glob("*.xml")):
            original = entries(source)
            if not original:
                continue
            target = res / "values-ar" / source.name
            assert target.exists(), f"Missing Arabic file: {target}"
            translated = entries(target)
            assert original.keys() == translated.keys(), (module, source.name,
                "missing", original.keys() - translated.keys(),
                "extra", translated.keys() - original.keys())
            source_raw, target_raw = raw_strings(source), raw_strings(target)
            for name, value in original.items():
                other = translated[name]
                assert value.tag == other.tag, (module, name, "resource type")
                assert markup(value) == markup(other), (module, name, "markup/item structure")
                source_items = [value] if value.tag == "string" else list(value)
                target_items = [other] if other.tag == "string" else list(other)
                for a, b in zip(source_items, target_items):
                    assert text(b).strip(), (module, name, "empty translation")
                    assert Counter(FORMATS.findall(text(a))) == Counter(FORMATS.findall(text(b))), (module, name, "placeholders")
                if name in source_raw:
                    assert Counter(PRODUCT_ENTITIES.findall(source_raw[name])) == Counter(PRODUCT_ENTITIES.findall(target_raw[name])), (module, name, "product/path entities")
                totals[value.tag] += 1
        print(module, dict(totals))
    # The language picker order is contractual: index 0 English, index 1 Arabic.
    for locale in ("values", "values-ar"):
        picker = entries(ROOT / "app/src/main/res" / locale / "strings.xml")["app_language_names"]
        assert [item.text for item in picker] == ["English", "العربية"]
    ET.parse(ROOT / "app/src/main/AndroidManifest.xml")
    print(f"PASS: {xml_count} resource XML files plus app manifest; counts, placeholders, markup, entities and picker order match.")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

import argparse
import yaml
from pathlib import Path


def to_yesno(b):
    return "yes" if b else "no"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--specs", "-s", type=Path)
    args = parser.parse_args()

    data = yaml.load(args.specs.read_text())

    headers = ("Function Name", "Category", "Implemented", "Param Min", "Param Max", "Return Type", "Note")
    print("| " + " | ".join(headers) + " |")
    seps = ["-" * len(label) for label in headers]
    print("| " + " | ".join(seps) + " |")

    for func_name, func_data in data.items():
        category = func_data.get("category") or "-"
        impl_status = to_yesno(func_data["implemented"])
        note = func_data.get("note") or ""
        p_min = ""
        p_max = ""
        return_type = func_data.get("return") or ""
        if impl_status and "params" in func_data:
            p_min = func_data["params"]["min"]
            if p_min is None:
                p_min = "*"
            p_max = func_data["params"]["max"]
            if p_max is None:
                p_max = "*"
        print(f"| {func_name.upper()} | {category.capitalize()} | {impl_status} | {p_min} | {p_max} | {return_type} | {note} |")



if __name__ == "__main__":
    main()


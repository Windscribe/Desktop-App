#!/usr/bin/env python3

# Copyright (C) 2023 Windscribe Limited

# Requirements:
# - Translate toolkit: pip install translate-toolkit[XML]

import argparse
import glob
import json
import os
import requests
import sys
import uuid

from requests.exceptions import HTTPError
from translate.storage import ts2 as tsparser

__version__ = "1.0.0"
__mstranslator_environ_key_name__ = "MSTRANSLATOR_API_KEY"


def simulate(in_file):
    tsfile = tsparser.tsfile.parsefile(in_file)

    target_language = tsfile.gettargetlanguage()
    if target_language is None:
        print("WARNING: the target language is missing from the ts file's header: " + in_file)
    else:
        print(f"Target language: {target_language}")

    strings_to_translate = 0
    for unit in tsfile.units:
        if unit.istranslatable() and not unit.istranslated():
            print(unit.getlocations())
            print(unit.source)
            strings_to_translate += 1

    print(f"{strings_to_translate} strings to translate")

    if args.remove_vanished:
        strings_to_remove = 0
        for unit in tsfile.units:
            if unit.isobsolete():
                print(unit.getlocations())
                print(unit.source)
                strings_to_remove += 1

        print(f"{strings_to_remove} strings to remove")


def remove_vanished_from_file(in_file):
    tsfile = tsparser.tsfile.parsefile(in_file)

    units_to_remove = []

    for unit in tsfile.units:
        if unit.isobsolete():
            units_to_remove.append(unit)

    if len(units_to_remove) == 0:
        print("There are no vanished translation units requiring removal.")
        return

    for unit in units_to_remove:
        tsfile.removeunit(unit)

    if args.output:
        if not os.path.exists(args.output):
            os.makedirs(args.output)

        tsfile.savefile(os.path.join(args.output, os.path.basename(in_file)))
    else:
        tsfile.savefile(in_file)

    print("{} vanished units removed".format(len(units_to_remove)))


def remove_vanished(in_file):
    if os.path.isdir(in_file):
        print(f"Removing all vanished entries for files in {in_file}")
        fullmask = os.path.join(in_file, "*.ts")
        for filename in glob.glob(fullmask):
            if not os.path.isdir(filename):
                print(f"Removing all vanished entries from {filename}")
                remove_vanished_from_file(filename)
    else:
        remove_vanished_from_file(in_file)


def translate_tsfile(in_file):
    tsfile = tsparser.tsfile.parsefile(in_file)

    target_language = tsfile.gettargetlanguage()
    if target_language is None:
        raise IOError("The target language is missing from the ts file's header: " + in_file)
    print(f"Target language: {target_language}")

    strings_to_translate = []
    for unit in tsfile.units:
        if unit.istranslatable() and not unit.istranslated():
            strings_to_translate.append({'text': unit.source})

    if len(strings_to_translate) == 0:
        print("There are no strings requiring translation.")
        return

    constructed_url = "https://api.cognitive.microsofttranslator.com/translate?api-version=3.0&from=en&to=" + target_language

    header_items = {
        "Ocp-Apim-Subscription-Key": os.getenv(__mstranslator_environ_key_name__),
        "Ocp-Apim-Subscription-Region": "canadacentral",
        "Content-type": "application/json",
        "X-ClientTraceId": str(uuid.uuid4())
    }

    request_result = requests.post(constructed_url, headers=header_items, json=strings_to_translate)
    request_result.raise_for_status()
    response_json = request_result.json()

    index = 0
    for unit in tsfile.units:
        if unit.istranslatable() and not unit.istranslated() and index < len(response_json):
            unit.target = response_json[index]["translations"][0]["text"]
            unit.markfuzzy(False)
            index += 1

    if args.output:
        if not os.path.exists(args.output):
            os.makedirs(args.output)

        tsfile.savefile(os.path.join(args.output, os.path.basename(in_file)))

        with open(os.path.join(args.output, f"mstranslator_{os.path.basename(in_file)}.json"), "w") as json_file:
            json.dump(response_json, json_file)
    else:
        tsfile.savefile(in_file)

    print("{} strings translated".format(len(strings_to_translate)))


def process_response_json(in_file):
    target_dir = os.path.join(os.path.dirname(in_file), "translated")
    with open(os.path.join(target_dir, "mstranslator_response.json"), "r") as json_file:
        file_contents = json_file.read()

    response_json = json.loads(file_contents)
    print(response_json[0]["translations"][0]["text"])


if __name__ == "__main__":  # pragma: no cover
    parser = argparse.ArgumentParser(description="Translate 'unfinished' entries in a Qt ts file to the target language specified in the file")
    parser.add_argument('source', metavar='SOURCE', type=str, action='store', help="The Qt ts file to process.")
    parser.add_argument('--output', metavar='DIR', type=str, action='store', help="Output modified file, and translator json response if translating, to this directory.  Default is to overwrite the SOURCE file.")
    parser.add_argument('--remove-vanished', default=False, action='store_true', help="Remove all 'vanished' entries from the ts file, or all ts files in a folder.")
    parser.add_argument('--simulate', default=False, action='store_true', help="Print entries requiring translation/removal, but do not translate/remove them.")
    args = parser.parse_args()

    try:
        in_file = os.path.abspath(args.source)
        if not os.path.exists(in_file):
            raise IOError("The specified source file/folder could not be found: " + in_file)
        if args.simulate:
            simulate(in_file)
        elif args.remove_vanished:
            remove_vanished(in_file)
        else:
            if __mstranslator_environ_key_name__ not in os.environ:
                raise IOError("Please set the '{}' environment variable to your API key.".format(__mstranslator_environ_key_name__))
            translate_tsfile(in_file)
    except HTTPError as http_err:
        print(f'HTTP error occurred: {http_err}')
    except IOError as io_err:
        print(io_err)
        sys.exit(1)

    sys.exit(0)

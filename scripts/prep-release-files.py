#!/usr/bin/env python3
import json
import sys


def write_generated_notes(notes_json, title_path, body_path):
    with open(notes_json, "r", encoding="utf-8") as handle:
        payload = json.load(handle)

    with open(title_path, "w", encoding="utf-8") as handle:
        handle.write(payload["name"])

    with open(body_path, "w", encoding="utf-8") as handle:
        handle.write(
            "Maintainer note: review the generated notes, adjust headings or "
            "known limitations if needed, then publish this draft release.\n\n"
        )
        handle.write(payload["body"])


def write_update_json(title_path, body_path, output_path):
    with open(title_path, "r", encoding="utf-8") as handle:
        title = handle.read()

    with open(body_path, "r", encoding="utf-8") as handle:
        body = handle.read()

    with open(output_path, "w", encoding="utf-8") as handle:
        json.dump(
            {
                "name": title,
                "body": body,
                "draft": True,
                "prerelease": False,
            },
            handle,
        )


def main():
    if len(sys.argv) < 2:
        raise SystemExit("usage: prep-release-files.py <notes|update> ...")

    mode = sys.argv[1]
    if mode == "notes" and len(sys.argv) == 5:
        write_generated_notes(sys.argv[2], sys.argv[3], sys.argv[4])
        return
    if mode == "update" and len(sys.argv) == 5:
        write_update_json(sys.argv[2], sys.argv[3], sys.argv[4])
        return

    raise SystemExit("usage: prep-release-files.py <notes|update> ...")


if __name__ == "__main__":
    main()

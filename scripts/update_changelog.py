import os
import sys
import subprocess
import re
from datetime import datetime


def get_current_version():
    # Prefer the PLUGIN_VERSION env var (set by CI from the tag name),
    # fall back to the VERSION file for local runs.
    env_version = os.environ.get("PLUGIN_VERSION", "").strip()
    if env_version:
        return env_version
    with open("VERSION", "r") as f:
        return f.read().strip()


def get_last_tag():
    try:
        result = subprocess.run(
            ["git", "describe", "--tags", "--abbrev=0"],
            capture_output=True, text=True, check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return None


def get_commits_since(tag):
    command = ["git", "log", "--oneline", "--no-merges"]
    if tag:
        command.append(f"{tag}..HEAD")
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    lines = result.stdout.strip().split("\n")
    return [l for l in lines if l]


def parse_commits(commits):
    sections = {
        "Added":         [],
        "Fixed":         [],
        "Changed":       [],
        "Documentation": [],
        "Other":         [],
    }

    # Conventional commit prefixes → section
    mapping = {
        "feat":     "Added",
        "add":      "Added",
        "fix":      "Fixed",
        "bug":      "Fixed",
        "change":   "Changed",
        "refactor": "Changed",
        "perf":     "Changed",
        "build":    "Changed",
        "docs":     "Documentation",
        "chore":    "Other",
    }

    # Prefixes that are purely internal — skip them in release notes
    skip_prefixes = {"ci", "test", "style"}

    # Commit message substrings that are always noise
    skip_substrings = {"chore: bump version", "docs: update changelog"}

    for commit in commits:
        if not commit:
            continue

        message = " ".join(commit.split(" ")[1:])  # strip the hash

        # Drop internal commits entirely
        if any(s in message.lower() for s in skip_substrings):
            continue

        match = re.match(r"^(\w+)(?:\(.*?\))?:", message.lower())
        if match:
            prefix = match.group(1)
            if prefix in skip_prefixes:
                continue
            section = mapping.get(prefix, "Other")
            clean_msg = message[match.end():].strip()
            clean_msg = clean_msg[0].upper() + clean_msg[1:] if clean_msg else ""
            sections[section].append(f"- {clean_msg}")
        else:
            sections["Other"].append(f"- {message}")

    return sections


def build_entry(version, sections):
    date = datetime.now().strftime("%Y-%m-%d")
    entry = f"## [{version}] - {date}\n\n"

    has_content = any(items for items in sections.values())
    if not has_content:
        entry += "_No significant changes documented._\n\n"
        return entry

    for title, items in sections.items():
        if items:
            entry += f"### {title}\n"
            for item in items:
                entry += f"{item}\n"
            entry += "\n"

    return entry


def update_changelog(version, sections):
    entry = build_entry(version, sections)

    # Write just this release's notes for the GitHub Release body
    with open("LATEST_RELEASE_NOTES.md", "w") as f:
        f.write(entry)

    # Prepend to the full CHANGELOG
    with open("CHANGELOG.md", "r") as f:
        content = f.read()

    header_end = content.find("## [")
    if header_end == -1:
        new_content = content + "\n" + entry
    else:
        new_content = content[:header_end] + entry + content[header_end:]

    with open("CHANGELOG.md", "w") as f:
        f.write(new_content)


if __name__ == "__main__":
    version  = get_current_version()
    last_tag = get_last_tag()

    print(f"Generating changelog for v{version} since {last_tag or 'the beginning of history'}...")

    commits  = get_commits_since(last_tag)
    sections = parse_commits(commits)

    total = sum(len(v) for v in sections.values())
    print(f"Found {len(commits)} commits → {total} entries across {sum(1 for v in sections.values() if v)} sections")

    update_changelog(version, sections)
    print("Done. CHANGELOG.md and LATEST_RELEASE_NOTES.md updated.")

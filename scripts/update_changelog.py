import sys
import subprocess
import re
from datetime import datetime

def get_current_version():
    with open("VERSION", "r") as f:
        return f.read().strip()

def get_last_tag():
    try:
        # Get the most recent tag
        result = subprocess.run(
            ["git", "describe", "--tags", "--abbrev=0"],
            capture_output=True, text=True, check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return None

def get_commits_since(tag):
    command = ["git", "log", "--oneline"]
    if tag:
        command.append(f"{tag}..HEAD")
    
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    return result.stdout.strip().split("\n")

def parse_commits(commits):
    sections = {
        "Added": [],
        "Fixed": [],
        "Changed": [],
        "Documentation": [],
        "Other": []
    }
    
    # Mapping prefixes to sections
    mapping = {
        "feat": "Added",
        "add": "Added",
        "fix": "Fixed",
        "bug": "Fixed",
        "change": "Changed",
        "refactor": "Changed",
        "docs": "Documentation",
        "build": "Changed",
        "chore": "Other"
    }

    for commit in commits:
        if not commit or "chore: bump version" in commit or "Merge pull request" in commit:
            continue
            
        # Extract the message (skip hash)
        message = " ".join(commit.split(" ")[1:])
        
        # Try to find a prefix (e.g., "feat: ...", "fix(macos): ...")
        match = re.match(r"^(\w+)(?:\(.*\))?:", message.lower())
        if match:
            prefix = match.group(1)
            section = mapping.get(prefix, "Other")
            # Clean up the message for the changelog
            clean_msg = message[match.end():].strip()
            # Capitalize first letter
            clean_msg = clean_msg[0].upper() + clean_msg[1:] if clean_msg else ""
            sections[section].append(f"- {clean_msg} ({prefix})")
        else:
            # No prefix, put in Other
            sections["Other"].append(f"- {message}")

    return sections

def update_changelog(version, sections):
    date = datetime.now().strftime("%Y-%m-%d")
    new_entry = f"## [{version}] - {date}\n\n"
    
    empty = True
    for title, items in sections.items():
        if items:
            new_entry += f"### {title}\n"
            for item in items:
                new_entry += f"{item}\n"
            new_entry += "\n"
            empty = False
            
    if empty:
        new_entry += "_No significant changes documented._\n\n"

    # Save the new entry for GitHub Release body
    with open("LATEST_RELEASE_NOTES.md", "w") as f:
        f.write(new_entry)

    with open("CHANGELOG.md", "r") as f:
        content = f.read()

    # Find the position to insert (after the header)
    header_end = content.find("## ")
    if header_end == -1:
        # If no versions exist yet, append to the end
        new_content = content + "\n" + new_entry
    else:
        new_content = content[:header_end] + new_entry + content[header_end:]

    with open("CHANGELOG.md", "w") as f:
        f.write(new_content)

if __name__ == "__main__":
    version = get_current_version()
    last_tag = get_last_tag()
    
    print(f"Generating changelog for version {version} since {last_tag or 'beginning'}...")
    
    commits = get_commits_since(last_tag)
    sections = parse_commits(commits)
    update_changelog(version, sections)
    print("CHANGELOG.md updated successfully.")

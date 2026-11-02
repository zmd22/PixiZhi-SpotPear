#!/usr/bin/env python3
"""Check common accidental-publication hazards without storing private values."""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
errors = []
for path in root.rglob('*'):
    if not path.is_file() or '.git' in path.parts:
        continue
    rel = path.relative_to(root)
    if path.name == 'secrets.yaml' or path.suffix in {'.bin', '.elf', '.zip', '.bundle'}:
        errors.append(f'{rel}: local secrets/build/archive file')
    if any(part in {'private', 'backups', 'configured', '.esphome', '__pycache__'} for part in rel.parts):
        errors.append(f'{rel}: local-only directory')
    if path.suffix not in {'.yaml', '.yml', '.md', '.py', '.txt'}:
        continue
    text = path.read_text()
    if re.search(r'\b(?:\d{1,3}\.){3}\d{1,3}\b', text):
        errors.append(f'{rel}: literal IPv4 address')
    if re.search(r'(?im)^\s*(?:password|ssid):\s*(?!\!secret\b)(?!PixiZhi Setup\s*$)\S', text):
        errors.append(f'{rel}: inline Wi-Fi/password setting')
    if re.search(r'(?im)^\s*use_address:', text):
        errors.append(f'{rel}: configured OTA address')
if errors:
    print('\n'.join(errors))
    sys.exit(1)
print('Public-source checks passed')

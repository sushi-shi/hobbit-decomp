"""Paths owned by the standalone source project."""
import os
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
BUILD = REPO / 'build'
INCLUDE = REPO / 'include'
VENDOR = REPO / 'vendor'


def retail_exe():
    return Path(os.environ.get('HOBBIT_EXE') or BUILD / 'orig/Meridian.exe')


def msvc_dir():
    value = os.environ.get('MSVC_DIR')
    if not value:
        raise RuntimeError('MSVC_DIR unset; use the exported Nix development shell')
    return Path(value)


def dxsdk_dir():
    value = os.environ.get('DXSDK_DIR')
    if not value:
        raise RuntimeError('DXSDK_DIR unset; configure a verified SDK if needed')
    return Path(value)

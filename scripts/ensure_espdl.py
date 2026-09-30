"""Prepare the ESP-DL source tree required by the Astra PlatformIO build."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import tempfile
import urllib.error
import urllib.request
import zipfile


ESP_DL_REVISION = "4f4efd7ff021a2c9dcafad4f1dab3da349b8bf4a"
ESP_DL_ARCHIVE_URL = (
    "https://github.com/espressif/esp-dl/archive/" f"{ESP_DL_REVISION}.zip"
)
_COMPILE_FINALIZE_RELATIVE = Path("esp-dl") / "cmake" / "compile_finalize.py"


def _compile_finalize_path(espdl_root: Path) -> Path:
    return espdl_root / _COMPILE_FINALIZE_RELATIVE


def _download_archive(url: str, destination: Path) -> None:
    request = urllib.request.Request(
        url,
        headers={"User-Agent": "Factory_Astra-PlatformIO"},
    )
    try:
        with urllib.request.urlopen(request, timeout=120) as response:
            with destination.open("wb") as output:
                shutil.copyfileobj(response, output)
    except (OSError, urllib.error.URLError) as exc:
        raise RuntimeError(
            "Unable to download ESP-DL automatically from "
            f"{url}. Check the network connection and try the build again."
        ) from exc


def _extract_archive(archive: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    extraction_root = destination.resolve()
    try:
        with zipfile.ZipFile(archive) as bundle:
            for member in bundle.infolist():
                target = (destination / member.filename).resolve()
                try:
                    inside_root = (
                        os.path.commonpath([str(extraction_root), str(target)])
                        == str(extraction_root)
                    )
                except ValueError:
                    inside_root = False
                if not inside_root:
                    raise RuntimeError(
                        "The ESP-DL archive contains an unsafe path: "
                        + member.filename
                    )
            bundle.extractall(destination)
    except zipfile.BadZipFile as exc:
        raise RuntimeError("The downloaded ESP-DL archive is invalid.") from exc


def _backup_incomplete_tree(espdl_root: Path) -> Path:
    backup_root = Path(
        tempfile.mkdtemp(
            prefix="esp-dl-incomplete-",
            dir=str(espdl_root.parent),
        )
    )
    backup_root.rmdir()
    shutil.move(str(espdl_root), str(backup_root))
    print(f"Incomplete ESP-DL directory preserved at {backup_root}")
    return backup_root


def ensure_espdl(project_dir: str | os.PathLike[str], downloader=None) -> Path:
    """Return a usable ESP-DL tree, downloading it when the tree is missing."""

    project_root = Path(project_dir).resolve()
    espdl_root = project_root / "third_party" / "esp-dl"
    if _compile_finalize_path(espdl_root).is_file():
        return espdl_root

    if espdl_root.exists():
        if not espdl_root.is_dir():
            raise RuntimeError(
                f"ESP-DL path is not a directory: {espdl_root}"
            )

    espdl_root.parent.mkdir(parents=True, exist_ok=True)
    fetch = downloader or _download_archive
    print("ESP-DL is missing; downloading the pinned dependency automatically...")
    with tempfile.TemporaryDirectory(
        prefix="factory-astra-espdl-",
        dir=str(espdl_root.parent),
    ) as temp_dir_name:
        temp_dir = Path(temp_dir_name)
        archive = temp_dir / "esp-dl.zip"
        extracted = temp_dir / "extracted"
        fetch(ESP_DL_ARCHIVE_URL, archive)
        _extract_archive(archive, extracted)

        candidates = [
            child
            for child in extracted.iterdir()
            if child.is_dir() and _compile_finalize_path(child).is_file()
        ]
        if len(candidates) != 1:
            raise RuntimeError(
                "The downloaded ESP-DL archive does not contain the expected "
                "esp-dl/cmake/compile_finalize.py file."
            )
        previous_root = None
        if espdl_root.exists():
            previous_root = _backup_incomplete_tree(espdl_root)
        try:
            shutil.move(str(candidates[0]), str(espdl_root))
        except Exception:
            if previous_root is not None and not espdl_root.exists():
                shutil.move(str(previous_root), str(espdl_root))
            raise

    if not _compile_finalize_path(espdl_root).is_file():
        raise RuntimeError(
            "ESP-DL download completed, but compile_finalize.py is still missing "
            f"from {espdl_root}."
        )
    return espdl_root

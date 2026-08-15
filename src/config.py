from pathlib import Path
from dataclasses import dataclass
from typing import Optional

@dataclass
class DownloadConfig:
    max_parallel_connections: int = 8
    chunk_size: int = 1024 * 256
    timeout: int = 30
    max_bandwidth: Optional[int] = None
    retry_attempts: int = 3
    resume_downloads: bool = True
    download_folder: Path = Path.home() / "Downloads"

    def __post_init__(self):
        self.download_folder.mkdir(parents=True, exist_ok=True)

config = DownloadConfig()

import asyncio
import aiohttp
import aiofiles
import time
from pathlib import Path
from dataclasses import dataclass
from enum import Enum
from typing import Optional, Callable, List
from datetime import datetime
import os

class DownloadStatus(Enum):
    PENDING = "pending"
    DOWNLOADING = "downloading"
    PAUSED = "paused"
    COMPLETED = "completed"
    ERROR = "error"
    CANCELLED = "cancelled"

@dataclass
class DownloadProgress:
    file_name: str
    downloaded: int
    total_size: int
    speed: float
    elapsed_time: float
    remaining_time: float
    status: DownloadStatus
    progress_percent: float

class DownloadTask:
    def __init__(self, url: str, filename: str, output_path: Path, max_connections: int = 4):
        self.url = url
        self.filename = filename
        self.output_path = output_path / filename
        self.temp_file = self.output_path.with_suffix(self.output_path.suffix + '.tmp')
        self.max_connections = max_connections
        self.status = DownloadStatus.PENDING
        self.downloaded = 0
        self.total_size = 0
        self.start_time = None
        self.paused = False
        self.cancelled = False
        self.speed = 0.0
        self.last_update = 0
        self.last_downloaded = 0

    async def get_file_size(self, session: aiohttp.ClientSession) -> int:
        try:
            async with session.head(self.url, timeout=aiohttp.ClientTimeout(total=10)) as resp:
                return int(resp.headers.get('content-length', 0))
        except Exception:
            return 0

class DownloadEngine:
    def __init__(self, config):
        self.config = config
        self.tasks: List[DownloadTask] = []
        self.active_downloads = set()
        self.progress_callback: Optional[Callable] = None
        self.completed_callback: Optional[Callable] = None
        self.connector = None

    async def add_download(self, url: str, filename: str = None) -> DownloadTask:
        if filename is None:
            filename = url.split('/')[-1].split('?')[0] or 'download'

        task = DownloadTask(url, filename, self.config.download_folder, self.config.max_parallel_connections)

        async with aiohttp.ClientSession() as session:
            task.total_size = await task.get_file_size(session)

            if self.config.resume_downloads and task.temp_file.exists():
                task.downloaded = task.temp_file.stat().st_size

        self.tasks.append(task)
        return task

    async def download_file(self, task: DownloadTask):
        task.status = DownloadStatus.DOWNLOADING
        task.start_time = time.time()

        connector = aiohttp.TCPConnector(
            limit_per_host=task.max_connections,
            force_close=False,
            enable_cleanup_closed=True
        )

        async with aiohttp.ClientSession(connector=connector) as session:
            try:
                headers = {}
                if self.config.resume_downloads and task.downloaded > 0:
                    headers['Range'] = f'bytes={task.downloaded}-'

                async with session.get(self.url, headers=headers, timeout=aiohttp.ClientTimeout(total=None)) as resp:
                    if resp.status not in (200, 206):
                        task.status = DownloadStatus.ERROR
                        return

                    async with aiofiles.open(task.temp_file, 'ab') as f:
                        async for chunk in resp.content.iter_chunked(self.config.chunk_size):
                            if task.cancelled:
                                task.status = DownloadStatus.CANCELLED
                                return

                            if task.paused:
                                while task.paused and not task.cancelled:
                                    await asyncio.sleep(0.1)

                            await f.write(chunk)
                            task.downloaded += len(chunk)
                            await self._update_progress(task)

                            if self.config.max_bandwidth:
                                await self._rate_limit(task)

                task.temp_file.rename(task.output_path)
                task.status = DownloadStatus.COMPLETED
                if self.completed_callback:
                    self.completed_callback(task)

            except Exception as e:
                task.status = DownloadStatus.ERROR
                print(f"Error downloading {task.filename}: {e}")

    async def _update_progress(self, task: DownloadTask):
        now = time.time()
        if now - task.last_update >= 0.5:
            elapsed = now - task.start_time
            downloaded_since_last = task.downloaded - task.last_downloaded
            task.speed = (downloaded_since_last / (now - task.last_update)) if (now - task.last_update) > 0 else 0

            if task.speed > 0:
                remaining = task.total_size - task.downloaded
                task.remaining_time = remaining / task.speed

            progress = DownloadProgress(
                file_name=task.filename,
                downloaded=task.downloaded,
                total_size=task.total_size,
                speed=task.speed,
                elapsed_time=elapsed,
                remaining_time=task.remaining_time if hasattr(task, 'remaining_time') else 0,
                status=task.status,
                progress_percent=(task.downloaded / task.total_size * 100) if task.total_size > 0 else 0
            )

            if self.progress_callback:
                self.progress_callback(progress)

            task.last_update = now
            task.last_downloaded = task.downloaded

    async def _rate_limit(self, task: DownloadTask):
        if self.config.max_bandwidth:
            await asyncio.sleep(0.01)

    async def start_downloads(self):
        tasks = [self.download_file(task) for task in self.tasks if task.status == DownloadStatus.PENDING]
        if tasks:
            await asyncio.gather(*tasks)

    def pause_download(self, task: DownloadTask):
        task.paused = True
        task.status = DownloadStatus.PAUSED

    def resume_download(self, task: DownloadTask):
        task.paused = False
        task.status = DownloadStatus.DOWNLOADING

    def cancel_download(self, task: DownloadTask):
        task.cancelled = True
        task.status = DownloadStatus.CANCELLED

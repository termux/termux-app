import sys
import asyncio
from pathlib import Path
from PyQt6.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QPushButton,
    QLineEdit, QTableWidget, QTableWidgetItem, QFileDialog, QMessageBox,
    QProgressBar, QLabel, QSpinBox, QCheckBox, QTabWidget, QGroupBox,
    QFormLayout, QHeaderView
)
from PyQt6.QtCore import Qt, pyqtSignal, QThread, QObject, QTimer
from PyQt6.QtGui import QFont, QColor
from datetime import timedelta
import os

class DownloadWorker(QObject):
    progress_signal = pyqtSignal(dict)

    def __init__(self, engine):
        super().__init__()
        self.engine = engine
        self.loop = None

    def run_downloads(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.engine.progress_callback = self.on_progress
        self.loop.run_until_complete(self.engine.start_downloads())

    def on_progress(self, progress):
        self.progress_signal.emit({
            'file_name': progress.file_name,
            'downloaded': progress.downloaded,
            'total_size': progress.total_size,
            'speed': progress.speed,
            'elapsed_time': progress.elapsed_time,
            'remaining_time': progress.remaining_time,
            'progress_percent': progress.progress_percent,
            'status': progress.status.value
        })

class MainWindow(QMainWindow):
    def __init__(self, engine):
        super().__init__()
        self.engine = engine
        self.download_worker = None
        self.download_thread = None
        self.init_ui()
        self.setWindowTitle("FastDL - Descargador Ultrarrápido")
        self.setGeometry(100, 100, 1000, 700)

    def init_ui(self):
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout()

        tabs = QTabWidget()
        layout.addWidget(tabs)

        tabs.addTab(self.create_download_tab(), "Descargas")
        tabs.addTab(self.create_settings_tab(), "Configuración")

        main_widget.setLayout(layout)

    def create_download_tab(self):
        widget = QWidget()
        layout = QVBoxLayout()

        url_layout = QHBoxLayout()
        url_label = QLabel("URL:")
        self.url_input = QLineEdit()
        self.url_input.setPlaceholderText("Ingresa la URL del archivo a descargar...")
        url_layout.addWidget(url_label)
        url_layout.addWidget(self.url_input)
        layout.addLayout(url_layout)

        button_layout = QHBoxLayout()
        self.add_btn = QPushButton("Agregar Descarga")
        self.add_btn.clicked.connect(self.add_download)
        self.start_btn = QPushButton("Iniciar")
        self.start_btn.clicked.connect(self.start_downloads)
        self.start_btn.setEnabled(False)
        self.pause_btn = QPushButton("Pausar")
        self.pause_btn.setEnabled(False)
        self.folder_btn = QPushButton("Abrir Carpeta")
        self.folder_btn.clicked.connect(self.open_folder)

        button_layout.addWidget(self.add_btn)
        button_layout.addWidget(self.start_btn)
        button_layout.addWidget(self.pause_btn)
        button_layout.addWidget(self.folder_btn)
        button_layout.addStretch()
        layout.addLayout(button_layout)

        self.download_table = QTableWidget()
        self.download_table.setColumnCount(6)
        self.download_table.setHorizontalHeaderLabels([
            "Archivo", "Progreso", "Descargado", "Velocidad", "Tiempo Restante", "Estado"
        ])
        self.download_table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeMode.Stretch)
        layout.addWidget(self.download_table)

        stats_layout = QHBoxLayout()
        self.total_speed_label = QLabel("Velocidad Total: 0 MB/s")
        self.total_speed_label.setFont(QFont("Arial", 10, QFont.Weight.Bold))
        self.total_time_label = QLabel("Tiempo Total: -")
        stats_layout.addWidget(self.total_speed_label)
        stats_layout.addWidget(self.total_time_label)
        stats_layout.addStretch()
        layout.addLayout(stats_layout)

        widget.setLayout(layout)
        return widget

    def create_settings_tab(self):
        widget = QWidget()
        layout = QFormLayout()

        self.connections_spin = QSpinBox()
        self.connections_spin.setValue(self.engine.config.max_parallel_connections)
        self.connections_spin.setRange(1, 32)
        self.connections_spin.valueChanged.connect(self._update_connections)
        layout.addRow("Conexiones Paralelas:", self.connections_spin)

        self.resume_check = QCheckBox()
        self.resume_check.setChecked(self.engine.config.resume_downloads)
        self.resume_check.toggled.connect(self._update_resume)
        layout.addRow("Reanudar Descargas:", self.resume_check)

        self.folder_label = QLabel(str(self.engine.config.download_folder))
        change_folder_btn = QPushButton("Cambiar Carpeta")
        change_folder_btn.clicked.connect(self.change_download_folder)
        layout.addRow("Carpeta de Descargas:", self.folder_label)
        layout.addRow("", change_folder_btn)

        widget.setLayout(layout)
        return widget

    def add_download(self):
        url = self.url_input.text().strip()
        if not url:
            QMessageBox.warning(self, "Error", "Por favor ingresa una URL válida")
            return

        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        task = loop.run_until_complete(self.engine.add_download(url))
        loop.close()

        self.add_table_row(task)
        self.url_input.clear()
        self.start_btn.setEnabled(True)
        QMessageBox.information(self, "Éxito", f"Descarga agregada: {task.filename}")

    def add_table_row(self, task):
        row = self.download_table.rowCount()
        self.download_table.insertRow(row)

        self.download_table.setItem(row, 0, QTableWidgetItem(task.filename))

        progress_bar = QProgressBar()
        progress_bar.setValue(0)
        self.download_table.setCellWidget(row, 1, progress_bar)

        self.download_table.setItem(row, 2, QTableWidgetItem("0 B / 0 B"))
        self.download_table.setItem(row, 3, QTableWidgetItem("0 MB/s"))
        self.download_table.setItem(row, 4, QTableWidgetItem("-"))
        self.download_table.setItem(row, 5, QTableWidgetItem("Pendiente"))

    def start_downloads(self):
        self.start_btn.setEnabled(False)
        self.pause_btn.setEnabled(True)
        self.add_btn.setEnabled(False)

        self.download_worker = DownloadWorker(self.engine)
        self.download_worker.progress_signal.connect(self.update_progress)

        self.download_thread = QThread()
        self.download_worker.moveToThread(self.download_thread)
        self.download_thread.started.connect(self.download_worker.run_downloads)
        self.download_thread.start()

    def update_progress(self, progress_data):
        for row in range(self.download_table.rowCount()):
            if self.download_table.item(row, 0).text() == progress_data['file_name']:
                progress_bar = self.download_table.cellWidget(row, 1)
                progress_bar.setValue(int(progress_data['progress_percent']))

                downloaded = self._format_size(progress_data['downloaded'])
                total = self._format_size(progress_data['total_size'])
                self.download_table.setItem(row, 2, QTableWidgetItem(f"{downloaded} / {total}"))

                speed = self._format_speed(progress_data['speed'])
                self.download_table.setItem(row, 3, QTableWidgetItem(speed))

                remaining = self._format_time(progress_data['remaining_time'])
                self.download_table.setItem(row, 4, QTableWidgetItem(remaining))

                self.download_table.setItem(row, 5, QTableWidgetItem(progress_data['status'].upper()))

                self.total_speed_label.setText(f"Velocidad Total: {speed}")
                break

    @staticmethod
    def _format_size(bytes_size):
        for unit in ['B', 'KB', 'MB', 'GB']:
            if bytes_size < 1024:
                return f"{bytes_size:.1f} {unit}"
            bytes_size /= 1024
        return f"{bytes_size:.1f} TB"

    @staticmethod
    def _format_speed(bytes_per_sec):
        return MainWindow._format_size(bytes_per_sec) + "/s"

    @staticmethod
    def _format_time(seconds):
        if seconds <= 0:
            return "-"
        return str(timedelta(seconds=int(seconds)))

    def change_download_folder(self):
        folder = QFileDialog.getExistingDirectory(self, "Selecciona carpeta de descargas")
        if folder:
            self.engine.config.download_folder = Path(folder)
            self.folder_label.setText(folder)

    def open_folder(self):
        folder = self.engine.config.download_folder
        if sys.platform == 'win32':
            os.startfile(folder)
        elif sys.platform == 'darwin':
            os.system(f'open "{folder}"')
        else:
            os.system(f'xdg-open "{folder}"')

    def _update_connections(self, value):
        self.engine.config.max_parallel_connections = value

    def _update_resume(self, checked):
        self.engine.config.resume_downloads = checked

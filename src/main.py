#!/usr/bin/env python3
import sys
from PyQt6.QtWidgets import QApplication
from config import config
from download_engine import DownloadEngine
from ui.main_window import MainWindow

def main():
    app = QApplication(sys.argv)
    engine = DownloadEngine(config)
    window = MainWindow(engine)
    window.show()
    sys.exit(app.exec())

if __name__ == '__main__':
    main()

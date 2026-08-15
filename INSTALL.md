# Guía de Instalación FastDL

## Windows

### Opción 1: Instalación Manual

1. **Descargar Python**
   - Ve a https://www.python.org/downloads/
   - Descarga Python 3.9 o superior
   - Durante la instalación, marca "Add Python to PATH"

2. **Descargar FastDL**
   ```powershell
   git clone <repo-url>
   cd termux-app
   ```

3. **Instalar Dependencias**
   ```powershell
   pip install -r requirements.txt
   ```

4. **Ejecutar**
   ```powershell
   python src/main.py
   ```

### Opción 2: Ejecutable Standalone
(Próximamente - distribuible .exe)

## Linux (Ubuntu/Debian)

```bash
# Instalar Python y pip
sudo apt-get update
sudo apt-get install python3.9 python3-pip

# Clonar repositorio
git clone <repo-url>
cd termux-app

# Instalar dependencias
pip3 install -r requirements.txt

# Ejecutar
python3 src/main.py
```

## macOS

```bash
# Instalar Homebrew si no lo tienes
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Instalar Python
brew install python3

# Clonar repositorio
git clone <repo-url>
cd termux-app

# Instalar dependencias
pip3 install -r requirements.txt

# Ejecutar
python3 src/main.py
```

## Solución de Problemas

### Error: "pip not found"
- Asegúrate de que Python está en tu PATH
- En Windows, reinstala Python marcando "Add Python to PATH"

### Error: "ModuleNotFoundError: No module named 'PyQt6'"
- Ejecuta: `pip install --upgrade pip`
- Luego: `pip install -r requirements.txt`

### La aplicación no abre
- Intenta: `python -m pip install --upgrade PyQt6`
- O usa: `python3 src/main.py` en lugar de `python`

## Requerimientos Mínimos

- **SO**: Windows 7+, Linux, macOS 10.12+
- **Python**: 3.9+
- **RAM**: 512 MB mínimo
- **Conexión**: Internet para descargar archivos

## Próximas Versiones

- [ ] Instalador .exe para Windows
- [ ] App empaquetada para macOS (.dmg)
- [ ] AppImage para Linux
- [ ] Soporte para Snap y Flatpak

# FastDL - Descargador Ultrarrápido para PC

Un descargador de archivos de alto rendimiento para Windows, Linux y macOS, diseñado para maximizar el uso del ancho de banda disponible.

## ⚡ Características

- **Descargas Paralelas**: Hasta 8 conexiones simultáneas por archivo
- **Alto Rendimiento**: Motor asincrónico optimizado para máximo ancho de banda
- **Reanudación Automática**: Retoma descargas interrumpidas automáticamente
- **Interfaz Moderna**: UI limpia y responsiva con PyQt6
- **Monitoreo en Tiempo Real**: Velocidad, progreso y tiempo estimado
- **Multiplataforma**: Windows, Linux y macOS
- **Gestor de Cola**: Descarga múltiples archivos simultáneamente
- **Control de Ancho de Banda**: Limita la velocidad si es necesario

## 🚀 Instalación

### Requisitos
- Python 3.9+
- pip

### Pasos

```bash
# Clona el repositorio
git clone <repo-url>
cd termux-app

# Instala las dependencias
pip install -r requirements.txt

# Ejecuta la aplicación
python src/main.py
```

## 📖 Uso

1. **Agregar Descarga**: Pega la URL del archivo en el campo de entrada
2. **Iniciar**: Haz clic en "Iniciar" para comenzar las descargas
3. **Monitorear**: Observa el progreso en tiempo real con velocidad y tiempo estimado
4. **Pausar/Reanudar**: Pausa descargas en cualquier momento
5. **Configuración**: Ajusta conexiones paralelas y otras opciones

## ⚙️ Configuración

En la pestaña "Configuración" puedes:
- Ajustar número de conexiones paralelas (1-32)
- Habilitar/deshabilitar reanudación automática
- Cambiar carpeta de descargas

## 🔧 Desarrollo

Estructura del proyecto:
```
src/
├── main.py              # Punto de entrada
├── config.py            # Configuración
├── download_engine.py   # Motor de descargas
└── ui/
    └── main_window.py   # Interfaz gráfica
```

## 📊 Rendimiento

- **Velocidad de Descarga**: Limitada solo por tu conexión y servidor
- **Uso de CPU**: Mínimo (~2-5%)
- **Memoria**: Eficiente (~50-100 MB)
- **Conexiones**: Hasta 8 simultáneas por archivo

## 🔐 Privacidad

- No se recopilan datos personales
- Todas las descargas son locales
- No hay telemetría

## 📝 Licencia

MIT License

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Abre un issue o pull request para sugerir mejoras.

---

**FastDL**: Descarga más rápido, usa menos recursos. ⚡

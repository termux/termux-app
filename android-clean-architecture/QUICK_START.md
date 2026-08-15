# URU Clean Architecture - Inicio Rápido

## ⚡ TL;DR (30 segundos)

Si ya tienes JDK 17 y Android SDK instalados:

```bash
cd android-clean-architecture

# macOS/Linux
./gradlew assembleDebug

# Windows
gradlew.bat assembleDebug
```

El APK se generará en: `app/build/outputs/apk/debug/app-debug.apk`

---

## 🚀 Pasos Básicos (5 minutos)

### 1. Verificar Requisitos

```bash
# Debe mostrar openjdk 17 o superior
java -version

# Debe mostrar el path del SDK
echo $ANDROID_SDK_ROOT  # macOS/Linux
echo %ANDROID_SDK_ROOT% # Windows

# Verificar ADB
adb version
```

Si algo no funciona, ve a [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md)

### 2. Compilar

**Opción A: Terminal/CMD**
```bash
# Navega al directorio
cd path/to/android-clean-architecture

# Linux/Mac: Compilar debug
./gradlew assembleDebug

# Windows: Compilar debug
gradlew.bat assembleDebug
```

**Opción B: Android Studio**
1. File → Open → Selecciona la carpeta
2. Build → Make Project (Ctrl+F9)

### 3. Instalar en Dispositivo

```bash
# Con dispositivo conectado o emulador abierto
adb install -r app/build/outputs/apk/debug/app-debug.apk

# Ver dispositivos disponibles
adb devices
```

---

## 📁 Estructura Clave

```
android-clean-architecture/
├── src/main/
│   ├── java/com/example/cleanarchitecture/
│   │   ├── domain/        ← Lógica pura (Use Cases)
│   │   ├── data/          ← Repositorios, APIs
│   │   └── presentation/  ← UI, ViewModels
│   ├── res/
│   │   └── layout/        ← Interfaces XML
│   └── AndroidManifest.xml
├── build.gradle           ← Dependencias
├── gradlew / gradlew.bat  ← Scripts de compilación
└── BUILD_INSTRUCTIONS.md  ← Guía completa
```

---

## 🎯 Tareas Comunes

### Compilar APK Debug
```bash
./gradlew assembleDebug
# Salida: app/build/outputs/apk/debug/app-debug.apk
```

### Compilar APK Release (Producción)
Primero crea un keystore (ver BUILD_INSTRUCTIONS.md), luego:
```bash
./gradlew assembleRelease
# Salida: app/build/outputs/apk/release/app-release.apk
```

### Ejecutar Tests
```bash
./gradlew test
```

### Limpiar Build Anterior
```bash
./gradlew clean
```

### Compilar + Instalar + Ejecutar en Emulador
```bash
./gradlew installDebug
# O desde Android Studio: Run → Run 'app' (Shift+F10)
```

### Ver Logs en Tiempo Real
```bash
adb logcat | grep "URU\|cleanarchitecture"
```

---

## 🐛 Problemas Comunes

| Problema | Solución |
|----------|----------|
| `gradle: command not found` | Usa `./gradlew` (Linux/Mac) o `gradlew.bat` (Windows) en lugar de `gradle` |
| `JAVA_HOME not set` | Configura JAVA_HOME en variables de entorno |
| `Android SDK not found` | Configura ANDROID_SDK_ROOT |
| `Gradle sync failed` | Intenta: `./gradlew clean build` |
| `Permission denied: ./gradlew` | Ejecuta: `chmod +x gradlew` |

---

## 📚 Documentación Completa

Para configuración detallada y troubleshooting:
→ Ver [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md)

---

## ✅ Checklist Mínimo

- [ ] JDK 17+ instalado (`java -version`)
- [ ] Android SDK API 34 descargado
- [ ] `ANDROID_SDK_ROOT` y `JAVA_HOME` configurados
- [ ] `gradlew` tiene permisos de ejecución (macOS/Linux)
- [ ] Estás en el directorio `android-clean-architecture/`

¡Listo! Ejecuta `./gradlew assembleDebug` 🚀

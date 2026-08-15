# 🚀 BUILD APK QUICKLY - Navaja Mexx

## ⚡ 3 PASOS RÁPIDOS

### PASO 1: Requisitos (5 minutos)

**Windows/Mac/Linux** necesitas:
1. **Android Studio** - https://developer.android.com/studio
2. **Java JDK 11+** - (viene con Android Studio)

**Verificar instalación:**
```bash
java -version
```

---

### PASO 2: Descargar este proyecto

Opción A: Git clone
```bash
git clone https://github.com/sauljtm33xD/termux-app.git
cd termux-app/navaja-mexx
git checkout claude/navaja-mexx-deployment-syeodk
```

Opción B: Descargar ZIP
```
https://github.com/sauljtm33xD/termux-app/tree/claude/navaja-mexx-deployment-syeodk/navaja-mexx
Click: Download ZIP
```

---

### PASO 3: Compilar APK

#### 🪟 WINDOWS
```bash
# Doble-click en:
BUILD_APK_WINDOWS.bat

# O en PowerShell:
.\BUILD_APK_WINDOWS.bat
```

#### 🍎 macOS
```bash
chmod +x BUILD_APK_MAC_LINUX.sh
./BUILD_APK_MAC_LINUX.sh
```

#### 🐧 Linux
```bash
chmod +x BUILD_APK_MAC_LINUX.sh
./BUILD_APK_MAC_LINUX.sh
```

---

## ⏱️ TIEMPO

- Primer build: 10-15 minutos
- Builds siguientes: 5 minutos

---

## 📍 RESULTADO

El APK estará en:
```
android/app/build/outputs/apk/debug/app-debug.apk
```

Tamaño: ~70-100 MB

---

## 📱 INSTALAR EN TELÉFONO

### Opción A: USB Cable (Recomendado)
```bash
# Conecta teléfono por USB
# Activa USB Debugging en Configuración → Opciones de desarrollador

adb install android/app/build/outputs/apk/debug/app-debug.apk

# Listo!
```

### Opción B: Copiar archivo
1. Copia `app-debug.apk` a tu teléfono
2. Abre Explorador de archivos
3. Tap en el APK
4. Instala

---

## 🆘 TROUBLESHOOTING

### ❌ "java: command not found"
```
Solución: Instala Java JDK 11+
macOS: brew install openjdk@11
Linux: apt-get install openjdk-11-jdk
Windows: Descarga de oracle.com
```

### ❌ "ANDROID_HOME not set"
```
Solución: Configura ANDROID_HOME
Después de instalar Android Studio:

Windows (PowerShell):
$env:ANDROID_HOME = "C:\Users\[Usuario]\AppData\Local\Android\Sdk"

macOS/Linux:
export ANDROID_HOME=~/Library/Android/sdk

Luego intenta de nuevo
```

### ❌ "Gradle build failed"
```
Solución: Limpiar caché
cd android
./gradlew clean
./gradlew assembleDebug
```

---

## ✅ RESUMEN

1. Instala Android Studio
2. Descargar proyecto
3. Ejecuta script (BUILD_APK_*.bat o .sh)
4. Espera compilación
5. Instala APK en teléfono
6. ¡Listo!

---

**¿Necesitas ayuda?** Comparte el error exacto 👇

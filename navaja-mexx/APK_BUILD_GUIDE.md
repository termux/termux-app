# 📱 APK BUILD GUIDE - Navaja Mexx

## Requisitos Previos
- Android SDK (part of Android Studio)
- Java Development Kit (JDK 11+)
- Gradle
- Node.js (ya lo tienes)

---

## Opción A: Android Studio GUI (Recomendado)

### Paso 1: Instalar Android Studio
https://developer.android.com/studio

### Paso 2: Abrir el proyecto Android
```bash
# En terminal, desde navaja-mexx/
open android/     # macOS
# o doble-click en la carpeta android/ en Windows/Linux
```

### Paso 3: En Android Studio
1. **File** → **Open**
2. Selecciona la carpeta `android/`
3. Espera a que sincronice Gradle (1-2 min)

### Paso 4: Compilar APK
1. **Build** → **Build Bundle(s) / APK(s)** → **Build APK(s)**
2. Espera a que compile (3-5 min)

### Paso 5: Encontrar el APK
```
android/app/build/outputs/apk/debug/app-debug.apk
```

✅ **APK lista para instalar en Android**

---

## Opción B: Terminal (Línea de Comandos)

### Paso 1: Instalar SDK tools
```bash
# Instalar Android SDK (si no lo tienes)
# macOS:
brew install android-sdk

# Ubuntu/Linux:
apt-get install android-sdk

# Windows: Descargar de https://developer.android.com/studio
```

### Paso 2: Compilar APK
```bash
cd android/
./gradlew assembleDebug

# Esperar 3-5 minutos...
```

### Paso 3: APK compilado
```
app/build/outputs/apk/debug/app-debug.apk
```

---

## Opción C: GitHub Actions (Sin instalar nada)

### Paso 1: Crear archivo workflow
```
.github/workflows/build-apk.yml
```

### Paso 2: Contenido del archivo
```yaml
name: Build APK

on:
  push:
    branches: [ main ]
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Set up JDK
        uses: actions/setup-java@v2
        with:
          java-version: '11'
      - name: Build APK
        run: cd android && ./gradlew assembleDebug
      - name: Upload APK
        uses: actions/upload-artifact@v2
        with:
          name: app-debug.apk
          path: android/app/build/outputs/apk/debug/app-debug.apk
```

### Paso 3: GitHub compila automáticamente
- Cada push ejecuta el workflow
- Descarga APK en "Artifacts"

---

## 📱 Instalar en teléfono Android

### Opción 1: USB Cable
```bash
# Conectar teléfono por USB
# Activar "Depuración USB" en Configuración → Opciones de desarrollador

adb install app-debug.apk

# o en Android Studio:
# Run → Select Device → Instalar
```

### Opción 2: Transferir archivo
1. Copiar `app-debug.apk` a tu teléfono
2. Abrir Explorador de archivos
3. Tap en `app-debug.apk`
4. Tocar "Instalar"

### Opción 3: QR Code
1. Hospedar APK en servidor
2. Generar QR que apunte a URL
3. Escanear en teléfono

---

## ⚙️ Configurar Firma (Release APK)

Para distribuir en Play Store necesitas firmar el APK:

### Paso 1: Generar keystore
```bash
keytool -genkey -v -keystore release.keystore -keyalg RSA -keysize 2048 -validity 10000 -alias navaja-mexx
```

### Paso 2: Configurar en Android Studio
1. **Build** → **Generate Signed Bundle/APK**
2. Seleccionar keystore generado
3. **Release** en lugar de Debug

### Paso 3: APK firmado
```
app/build/outputs/bundle/release/app-release.aab
```

---

## 🆘 Si falla la compilación

### Error: "SDK not found"
```bash
# Instalar SDK manualmente
android update sdk --no-ui --all
```

### Error: "Gradle build failed"
```bash
# Limpiar caché
cd android/
./gradlew clean
./gradlew assembleDebug
```

### Error: "Java version"
```bash
# Verificar Java versión (debe ser 11+)
java -version
```

---

## ✅ Resultado Final

- APK compilado: `app-debug.apk`
- Tamaño típico: 50-100 MB
- Compatible: Android 6.0+
- Características: PWA + Service Worker + API IA

---

## 🎯 Recomendación

1. **Testing rápido**: Opción B (Terminal)
2. **Desarrollo**: Opción A (Android Studio)
3. **CI/CD automático**: Opción C (GitHub Actions)

¿Necesitas ayuda con algún paso específico?

# URU Clean Architecture - Guía Completa de Compilación

## 📋 Requisitos Previos

### Software Requerido
- **JDK 17+**: Java Development Kit
- **Android SDK**: API level 34 (mínimo API 24)
- **Gradle 8.14+**: Sistema de compilación
- **Git**: Control de versiones

### Especificaciones del Proyecto
```
compileSdk:      34
targetSdk:       34
minSdk:          24
jvmTarget:       17
kotlinLanguage:  1.9+
```

---

## 🔧 Instalación en Windows

### Paso 1: Instalar Java Development Kit (JDK 17)

**Opción A: Descarga Manual**
1. Ve a [Oracle JDK Downloads](https://www.oracle.com/java/technologies/downloads/#java17) o [Eclipse Temurin](https://adoptium.net/)
2. Descarga JDK 17 (Windows x64)
3. Ejecuta el instalador `.exe`
4. Selecciona ruta de instalación (ej: `C:\Program Files\Java\jdk-17`)

**Opción B: Chocolatey (Windows)** *(si está instalado)*
```bash
choco install openjdk17
```

**Opción C: Usando Windows Package Manager**
```bash
winget install Eclipse.Temurin.17
```

### Paso 2: Configurar Variables de Entorno de Java

1. Abre **Panel de Control** → **Sistema** → **Configuración Avanzada del Sistema**
2. Haz clic en **Variables de Entorno**
3. Crea nueva variable de usuario:
   - **Variable**: `JAVA_HOME`
   - **Valor**: `C:\Program Files\Java\jdk-17.x.x` (ajusta la versión)

4. En **Path**, añade: `%JAVA_HOME%\bin`

**Verificación:**
```bash
java -version
javac -version
```

### Paso 3: Descargar e Instalar Android SDK

**Opción A: Android Studio (Recomendado)**
1. Descarga [Android Studio](https://developer.android.com/studio) para Windows
2. Ejecuta el instalador
3. Durante la instalación, selecciona:
   - Android SDK Platform 34
   - Android SDK Build Tools 34.0.0
   - Android Emulator
   - Android SDK Command-line Tools

4. Android Studio instalará automáticamente el SDK en:
   ```
   C:\Users\{TuUsuario}\AppData\Local\Android\sdk
   ```

**Opción B: Command-line Tools (Manual)**
1. Descarga [Command-line Tools](https://developer.android.com/studio#command-tools) desde Google
2. Descomprime en una carpeta (ej: `C:\android-sdk`)
3. Dentro de esa carpeta, crea un subdirectorio `cmdline-tools\latest`
4. Aceptar licencias:
   ```bash
   cd C:\android-sdk\cmdline-tools\latest\bin
   sdkmanager --licenses
   ```
5. Instalar componentes necesarios:
   ```bash
   sdkmanager "platforms;android-34"
   sdkmanager "build-tools;34.0.0"
   ```

### Paso 4: Configurar Variables de Entorno del SDK

1. Abre **Panel de Control** → **Sistema** → **Configuración Avanzada del Sistema**
2. Haz clic en **Variables de Entorno**
3. Crea nueva variable de usuario:
   - **Variable**: `ANDROID_SDK_ROOT`
   - **Valor**: `C:\Users\{TuUsuario}\AppData\Local\Android\sdk` (o tu ruta real)

4. En **Path**, añade: `%ANDROID_SDK_ROOT%\platform-tools`

**Verificación:**
```bash
adb version
```

### Paso 5: Instalar Gradle (Opcional - Wrapper Incluido)

Si prefieres gradle global:
1. Descarga [Gradle 8.14+](https://gradle.org/releases/)
2. Descomprime en `C:\gradle`
3. Añade a **Path**: `C:\gradle\bin`

**Verificación:**
```bash
gradle --version
```

---

## 🔧 Instalación en macOS

### Paso 1: Instalar Java Development Kit (JDK 17)

**Opción A: Homebrew**
```bash
brew install openjdk@17
brew link openjdk@17
```

**Opción B: Eclipse Temurin**
```bash
brew install --cask temurin
```

**Verificación:**
```bash
java -version
```

### Paso 2: Configurar Variables de Entorno de Java

Añade a `~/.zshrc` o `~/.bash_profile`:
```bash
export JAVA_HOME=$(/usr/libexec/java_home -v 17)
export PATH="$JAVA_HOME/bin:$PATH"
```

Luego:
```bash
source ~/.zshrc
java -version
```

### Paso 3: Descargar Android SDK

**Opción A: Android Studio**
```bash
brew install --cask android-studio
```

Android Studio descargará automáticamente los componentes necesarios.

**Opción B: Command-line Tools**
```bash
# Descarga manualmente desde https://developer.android.com/studio#command-tools
# Luego:
mkdir -p ~/Library/Android/sdk/cmdline-tools/latest
unzip cmdline-tools-mac-*.zip -d ~/Library/Android/sdk/cmdline-tools/latest

# Aceptar licencias
~/Library/Android/sdk/cmdline-tools/latest/bin/sdkmanager --licenses

# Instalar componentes
~/Library/Android/sdk/cmdline-tools/latest/bin/sdkmanager "platforms;android-34"
~/Library/Android/sdk/cmdline-tools/latest/bin/sdkmanager "build-tools;34.0.0"
```

### Paso 4: Configurar Variables de Entorno del SDK

Añade a `~/.zshrc` o `~/.bash_profile`:
```bash
export ANDROID_SDK_ROOT=~/Library/Android/sdk
export PATH="$ANDROID_SDK_ROOT/platform-tools:$PATH"
```

Luego:
```bash
source ~/.zshrc
adb version
```

---

## 🔧 Instalación en Linux (Ubuntu/Debian)

### Paso 1: Instalar Java Development Kit (JDK 17)

```bash
sudo apt update
sudo apt install openjdk-17-jdk openjdk-17-jdk-headless
```

**Verificación:**
```bash
java -version
javac -version
```

### Paso 2: Configurar Variables de Entorno de Java

Añade a `~/.bashrc` o `~/.zshrc`:
```bash
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export PATH="$JAVA_HOME/bin:$PATH"
```

Luego:
```bash
source ~/.bashrc
```

### Paso 3: Descargar Android SDK

**Opción A: Android Studio (Snap)**
```bash
snap install android-studio --classic
```

**Opción B: Command-line Tools Manual**
```bash
# Crear directorio
mkdir -p ~/Android/sdk/cmdline-tools/latest

# Descargar (desde https://developer.android.com/studio#command-tools)
# Luego descomprimir:
unzip cmdline-tools-linux-*.zip -d ~/Android/sdk/cmdline-tools/latest

# Aceptar licencias
~/Android/sdk/cmdline-tools/latest/bin/sdkmanager --licenses

# Instalar componentes
~/Android/sdk/cmdline-tools/latest/bin/sdkmanager "platforms;android-34"
~/Android/sdk/cmdline-tools/latest/bin/sdkmanager "build-tools;34.0.0"
```

### Paso 4: Configurar Variables de Entorno del SDK

Añade a `~/.bashrc` o `~/.zshrc`:
```bash
export ANDROID_SDK_ROOT=$HOME/Android/sdk
export PATH="$ANDROID_SDK_ROOT/platform-tools:$PATH"
```

Luego:
```bash
source ~/.bashrc
adb version
```

---

## 📂 Estructura del Proyecto

```
android-clean-architecture/
├── src/
│   └── main/
│       ├── java/com/example/cleanarchitecture/
│       │   ├── data/              # Data Layer (Repositories, API clients)
│       │   ├── domain/            # Domain Layer (Use Cases, Entities)
│       │   └── presentation/      # Presentation Layer (UI, ViewModels)
│       ├── res/
│       │   ├── layout/           # XML layouts
│       │   ├── values/           # Strings, colors, themes
│       │   └── drawable/         # Assets
│       └── AndroidManifest.xml   # Configuración de la app
├── build.gradle                  # Configuración de compilación
├── settings.gradle               # Configuración del proyecto
├── gradle/wrapper/               # Gradle wrapper (descargado automáticamente)
├── gradlew                       # Script de gradle para Linux/Mac
├── gradlew.bat                   # Script de gradle para Windows
└── proguard-rules.pro           # Reglas de ofuscación
```

---

## 🚀 Compilación del Proyecto

### Opción 1: Usar Android Studio (Recomendado para Principiantes)

1. **Abrir el proyecto:**
   - Abre Android Studio
   - File → Open
   - Navega a la carpeta `android-clean-architecture`
   - Haz clic en **Open**

2. **Esperar sincronización:**
   - Android Studio descargará dependencias automáticamente
   - Verás un mensaje "Gradle sync finished" en la consola

3. **Compilar:**
   - Build → Make Project
   - O: Ctrl+F9 (Windows/Linux) / Cmd+F9 (macOS)

4. **Ejecutar:**
   - Run → Run 'app'
   - O: Shift+F10 (Windows/Linux) / Ctrl+R (macOS)
   - Selecciona un emulador o dispositivo conectado

### Opción 2: Usar Gradle en Terminal

#### En Windows (CMD o PowerShell):
```bash
# Navega a la carpeta del proyecto
cd path\to\android-clean-architecture

# Verificar que gradle wrapper existe
dir gradlew.bat

# Compilar Debug APK
gradlew.bat assembleDebug

# Compilar Release APK (requiere firma)
gradlew.bat assembleRelease

# Ejecutar tests
gradlew.bat test

# Ver todas las tareas disponibles
gradlew.bat tasks
```

#### En macOS/Linux:
```bash
# Navega a la carpeta del proyecto
cd path/to/android-clean-architecture

# Otorgar permisos al script gradlew
chmod +x gradlew

# Compilar Debug APK
./gradlew assembleDebug

# Compilar Release APK (requiere firma)
./gradlew assembleRelease

# Ejecutar tests
./gradlew test

# Ver todas las tareas disponibles
./gradlew tasks
```

---

## 📦 Artefactos Generados

Después de compilar, los APK se encuentran en:

```
android-clean-architecture/app/build/outputs/apk/
├── debug/
│   └── app-debug.apk          # APK Debug (instala en emulador/dispositivo)
└── release/
    └── app-release-unsigned.apk # APK Release sin firma
```

---

## 📱 Instalar en Emulador o Dispositivo

### En Android Studio:
1. Device Manager → Crea/selecciona un emulador
2. Run → Run 'app'
3. Selecciona el emulador
4. Haz clic en OK

### En Terminal (con ADB):

**Instalar Debug APK:**
```bash
# Windows
adb install -r app\build\outputs\apk\debug\app-debug.apk

# macOS/Linux
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

**Ver dispositivos conectados:**
```bash
adb devices
```

**Ver logs en tiempo real:**
```bash
adb logcat
```

---

## 🔐 Compilar Release APK (Producción)

### Paso 1: Crear Keystores (Certificado de Firma)

**Windows (PowerShell):**
```powershell
keytool -genkey -v -keystore uru-keystore.jks -keyalg RSA `
  -keysize 2048 -validity 10000 `
  -alias uru-key
```

**macOS/Linux (Bash):**
```bash
keytool -genkey -v -keystore uru-keystore.jks -keyalg RSA \
  -keysize 2048 -validity 10000 \
  -alias uru-key
```

Te pedirá información:
- Contraseña del keystore (usa algo seguro, ej: `URU2024Secure!`)
- Nombre completo: Tu nombre
- Unidad organizativa: Tu empresa
- Organización: Tu compañía
- Ciudad: Tu ciudad
- Provincia: Tu provincia
- Código de país: (ej: ES, US, MX)

### Paso 2: Crear archivo `keystore.properties`

En la carpeta `android-clean-architecture/`, crea archivo `keystore.properties`:

```properties
storeFile=../uru-keystore.jks
storePassword=TuContraseñaAqui
keyAlias=uru-key
keyPassword=TuContraseñaAqui
```

### Paso 3: Configurar build.gradle

Abre `build.gradle` y añade en la sección `android`:

```gradle
android {
    // ... configuración existente ...
    
    signingConfigs {
        release {
            storeFile file(keystoreProperties['storeFile'])
            storePassword keystoreProperties['storePassword']
            keyAlias keystoreProperties['keyAlias']
            keyPassword keystoreProperties['keyPassword']
        }
    }

    buildTypes {
        release {
            signingConfig signingConfigs.release
            minifyEnabled false
            proguardFiles getDefaultProguardFile('proguard-android-optimize.txt'), 'proguard-rules.pro'
        }
    }
}

// Al inicio del archivo, después de plugins
def keystoreProperties = new Properties()
def keystorePropertiesFile = rootProject.file('keystore.properties')
if (keystorePropertiesFile.exists()) {
    keystoreProperties.load(new FileInputStream(keystorePropertiesFile))
}
```

### Paso 4: Compilar Release APK

```bash
# Windows
gradlew.bat assembleRelease

# macOS/Linux
./gradlew assembleRelease
```

El APK estará en: `app/build/outputs/apk/release/app-release.apk`

---

## 🐛 Solución de Problemas

### Error: "ANDROID_SDK_ROOT not found"
**Solución:**
```bash
# Windows (CMD)
set ANDROID_SDK_ROOT=C:\Users\{Usuario}\AppData\Local\Android\sdk

# macOS/Linux
export ANDROID_SDK_ROOT=~/Library/Android/sdk
# o
export ANDROID_SDK_ROOT=~/Android/sdk
```

### Error: "Java not found"
**Solución:**
Verifica que JAVA_HOME está configurado correctamente y reinicia la terminal/IDE.

```bash
# Windows
echo %JAVA_HOME%

# macOS/Linux
echo $JAVA_HOME
```

### Error: "Gradle sync failed"
**Solución:**
1. File → Invalidate Caches / Restart
2. O en terminal: `./gradlew clean build`

### Error: "Could not download gradle-X.X.X-all.zip"
**Solución:**
Verifica tu conexión a internet. Si el proxy está activo, configura:
```bash
./gradlew -Dhttp.proxyHost=proxy.com -Dhttp.proxyPort=8080 build
```

### Error en compilación Kotlin
**Solución:**
Asegúrate de que tienes JDK 17+:
```bash
java -version
# Debe mostrar "openjdk version "17.x"
```

---

## ✅ Verificación Final

Para verificar que todo funciona correctamente:

```bash
# Navega al proyecto
cd android-clean-architecture

# En Windows
gradlew.bat clean build

# En macOS/Linux
./gradlew clean build
```

Deberías ver al final:
```
BUILD SUCCESSFUL in X seconds
```

---

## 📋 Checklist Pre-Compilación

- [ ] JDK 17+ instalado y JAVA_HOME configurado
- [ ] Android SDK API 34 descargado
- [ ] ANDROID_SDK_ROOT configurado
- [ ] ADB funciona (`adb version`)
- [ ] Repositorio clonado correctamente
- [ ] Terminal/CMD abierta en directorio del proyecto
- [ ] Sin cambios sin guardar en archivos

---

## 🚀 Comandos Útiles

```bash
# Limpiar build previos
./gradlew clean

# Compilar solo
./gradlew build

# Compilar y ejecutar tests
./gradlew testDebugUnitTest

# Ver dependencias del proyecto
./gradlew dependencies

# Actualizar dependencias
./gradlew dependencyUpdates

# Generar javadoc
./gradlew javadoc

# Ver todas las tareas
./gradlew tasks
```

---

## 📞 Contacto y Soporte

Si encuentras problemas:
1. Revisa el archivo de logs en `build/reports/`
2. Ejecuta `./gradlew --debug build` para más información
3. Verifica que todas las variables de entorno estén configuradas

---

**Última actualización:** Agosto 2026
**Versión de Gradle:** 8.14.3
**Versión de Kotlin:** 1.9+
**Android API:** 34

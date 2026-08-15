# ✅ Checklist de Compilación - URU Clean Architecture

## Fase 1: Preparación del Sistema (15 minutos)

### Java Development Kit (JDK)
- [ ] Descargar JDK 17+ desde [Oracle](https://www.oracle.com/java/technologies/downloads/#java17) o [Eclipse Temurin](https://adoptium.net/)
- [ ] Instalar JDK
- [ ] Abrir Terminal/CMD nueva
- [ ] Verificar: `java -version` (debe mostrar 17 o superior)
- [ ] Verificar: `javac -version`
- [ ] Configurar JAVA_HOME (ver BUILD_INSTRUCTIONS.md si es necesario)

### Android SDK
- [ ] Opción A: Descargar [Android Studio](https://developer.android.com/studio)
  - [ ] Ejecutar instalador
  - [ ] Durante instalación, seleccionar SDK Platform 34
  - [ ] Seleccionar Build Tools 34.0.0
- [ ] O Opción B: Descargar Command-line Tools desde [Google](https://developer.android.com/studio#command-tools)
  - [ ] Instalar componentes necesarios
  - [ ] `sdkmanager "platforms;android-34"`
  - [ ] `sdkmanager "build-tools;34.0.0"`

### Variables de Entorno
- [ ] Configurar JAVA_HOME
  - Windows: Verificar en Panel de Control → Variables de Entorno
  - macOS/Linux: Verificar en `~/.bashrc` o `~/.zshrc`
- [ ] Configurar ANDROID_SDK_ROOT
  - Windows: `C:\Users\{Usuario}\AppData\Local\Android\sdk`
  - macOS: `~/Library/Android/sdk`
  - Linux: `~/Android/sdk`
- [ ] Abrir Terminal/CMD nueva y verificar:
  - `echo $JAVA_HOME` (macOS/Linux) o `echo %JAVA_HOME%` (Windows)
  - `echo $ANDROID_SDK_ROOT` (macOS/Linux) o `echo %ANDROID_SDK_ROOT%` (Windows)

### Verificación de ADB
- [ ] Ejecutar: `adb version`
- [ ] Debe mostrar versión de ADB

---

## Fase 2: Preparación del Proyecto (5 minutos)

- [ ] Clonar o descargar el repositorio
- [ ] Abrir Terminal/CMD
- [ ] Navegar a: `android-clean-architecture`
  ```bash
  cd path/to/android-clean-architecture
  ```

### Verificar Archivos Clave
- [ ] Existe `build.gradle`
- [ ] Existe `settings.gradle`
- [ ] Existe `gradlew` (macOS/Linux)
- [ ] Existe `gradlew.bat` (Windows)
- [ ] Existe carpeta `src/`
- [ ] Existe carpeta `gradle/wrapper/`

### Permisos (macOS/Linux)
- [ ] Dar permisos: `chmod +x gradlew`
- [ ] Verificar: `ls -la gradlew` (debe mostrar `x` en permisos)

---

## Fase 3: Compilación (Opción A: Terminal)

### Windows (CMD o PowerShell)
```bash
# Verificar ubicación
cd path\to\android-clean-architecture

# Compilar
gradlew.bat clean assembleDebug

# Esperar mensaje: BUILD SUCCESSFUL
```

- [ ] Ejecutar: `gradlew.bat clean`
- [ ] Esperar a que termine (descargará dependencias la primera vez)
- [ ] Ejecutar: `gradlew.bat assembleDebug`
- [ ] Debe mostrar: `BUILD SUCCESSFUL in X seconds`
- [ ] Verificar APK existe: `app\build\outputs\apk\debug\app-debug.apk`

### macOS/Linux (Terminal)
```bash
# Verificar ubicación
cd path/to/android-clean-architecture

# Compilar
./gradlew clean assembleDebug

# Esperar mensaje: BUILD SUCCESSFUL
```

- [ ] Ejecutar: `./gradlew clean`
- [ ] Esperar a que termine (descargará dependencias la primera vez)
- [ ] Ejecutar: `./gradlew assembleDebug`
- [ ] Debe mostrar: `BUILD SUCCESSFUL in X seconds`
- [ ] Verificar APK existe: `app/build/outputs/apk/debug/app-debug.apk`

---

## Fase 3 (Alternativa): Compilación (Opción B: Android Studio)

- [ ] Abrir Android Studio
- [ ] File → Open (o Recent Projects)
- [ ] Navegar a carpeta `android-clean-architecture`
- [ ] Seleccionar carpeta y hacer clic "Open"
- [ ] Esperar a que Android Studio abra el proyecto
- [ ] Esperar a "Gradle sync finished"
- [ ] Build → Make Project (o Ctrl+F9)
- [ ] Esperar mensaje: "Build completed successfully"
- [ ] Verificar en: Project → app → build → outputs → apk → debug

---

## Fase 4: Instalación en Dispositivo/Emulador

### Con Emulador (Android Studio)
- [ ] Android Studio abierto con proyecto
- [ ] Device Manager → Crear/Seleccionar emulador
- [ ] Iniciar emulador (esperar a que cargue completamente)
- [ ] Run → Run 'app' (o Shift+F10)
- [ ] Seleccionar emulador de la lista
- [ ] Hacer clic "OK"
- [ ] Esperar a que se instale y se abra

### Con Dispositivo Físico (ADB)
- [ ] Conectar dispositivo via USB
- [ ] Activar "Depuración USB" en Configuración → Opciones de Desarrollador
- [ ] En Terminal/CMD: `adb devices`
- [ ] Debe mostrar el dispositivo en la lista
- [ ] Instalar APK:
  ```bash
  # Windows
  adb install -r app\build\outputs\apk\debug\app-debug.apk
  
  # macOS/Linux
  adb install -r app/build/outputs/apk/debug/app-debug.apk
  ```
- [ ] Esperar mensaje: `Success`
- [ ] Buscar "URU" en dispositivo y abrir

---

## Fase 5: Verificación de Funcionamiento

- [ ] Aplicación abre correctamente
- [ ] No hay crashes (revisar logcat si hay problemas)
- [ ] Ver la interfaz de voz
- [ ] Botón de micrófono responde al clic

### Ver Logs (Opcional)
```bash
# Terminal/CMD
adb logcat | grep "URU\|cleanarchitecture"
```

---

## Problemas y Soluciones

### ❌ "gradle: command not found"
**Solución:** Debes estar en carpeta `android-clean-architecture` y usar `./gradlew` (macOS/Linux) o `gradlew.bat` (Windows)

### ❌ "JAVA_HOME is not set"
**Solución:** 
- Windows: Añade JAVA_HOME a Variables de Entorno → Reinicia terminal
- macOS/Linux: Añade a `~/.bashrc`: `export JAVA_HOME=/path/to/jdk17`

### ❌ "Android SDK not found"
**Solución:** Configura ANDROID_SDK_ROOT en variables de entorno

### ❌ "Permission denied: ./gradlew"
**Solución:** `chmod +x gradlew`

### ❌ "Gradle sync failed"
**Solución:** 
- Intenta: `./gradlew clean build`
- Si persiste: Elimina carpeta `.gradle/` e intenta de nuevo

### ❌ "BUILD FAILED"
**Solución:**
- Lee el mensaje de error en rojo
- Intenta: `./gradlew clean build`
- Si es error de dependencias, revisa conexión a internet
- Ejecuta: `./gradlew build --stacktrace` para más detalles

---

## 📊 Tiempo Estimado

| Fase | Tiempo | Notas |
|------|--------|-------|
| 1. Preparación Sistema | 15 min | Primera vez (siguiente: 1 min) |
| 2. Preparación Proyecto | 5 min | Única vez |
| 3. Primera Compilación | 10-15 min | Descarga dependencias |
| 3. Compilaciones Siguientes | 2-5 min | Cache de gradle |
| 4. Instalación | 2-5 min | Depende de dispositivo |
| **Total Primera Vez** | **30-40 min** | |
| **Total Siguientes** | **5-10 min** | |

---

## 🎉 ¡Éxito!

Si completaste todos los pasos y la app se abre correctamente, ¡la compilación fue exitosa!

**Próximos pasos:**
- Revisar código en `src/main/java/com/example/cleanarchitecture/`
- Hacer cambios y compilar nuevamente (`./gradlew assembleDebug`)
- Integrar APIs de IA (OpenAI, Anthropic, Gemini)
- Implementar servicios del sistema (Accessibility, Device Admin)

---

**Nota:** Guarda este checklist para futuras compilaciones. Los pasos del Sistema (Fase 1) solo se hacen una vez.

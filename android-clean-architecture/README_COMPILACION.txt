================================================================================
                    URU CLEAN ARCHITECTURE - APK COMPILATION
                         Guía Rápida de Compilación
================================================================================

ANTES DE COMENZAR, NECESITAS:
────────────────────────────
1. Java Development Kit (JDK) 17 o superior
2. Android SDK (API 34)
3. Esta carpeta: android-clean-architecture/
4. Terminal/CMD abierta en esta carpeta


PASO 1: VERIFICAR QUE TODO ESTÁ INSTALADO
──────────────────────────────────────────
Abre Terminal/CMD y ejecuta:

    java -version
    (Debe mostrar versión 17 o mayor)

    adb version
    (Debe mostrar versión de Android Debug Bridge)

Si alguno no funciona, ve a BUILD_INSTRUCTIONS.md


PASO 2: COMPILAR EL APK (Elige tu sistema)
───────────────────────────────────────────

OPCIÓN A: WINDOWS (CMD o PowerShell)
─────────────────────────────────────
Copia y pega en Terminal/CMD:

    gradlew.bat clean assembleDebug

Espera a ver: BUILD SUCCESSFUL

El APK estará en:
    app\build\outputs\apk\debug\app-debug.apk


OPCIÓN B: macOS/Linux (Terminal)
─────────────────────────────────
Copia y pega en Terminal:

    chmod +x gradlew
    ./gradlew clean assembleDebug

Espera a ver: BUILD SUCCESSFUL

El APK estará en:
    app/build/outputs/apk/debug/app-debug.apk


OPCIÓN C: Android Studio (Visual)
──────────────────────────────────
1. Abre Android Studio
2. File → Open → Selecciona esta carpeta
3. Build → Make Project (o Ctrl+F9)
4. Espera a "Build completed successfully"


PASO 3: INSTALAR EN DISPOSITIVO/EMULADOR
─────────────────────────────────────────

Si usaste Android Studio (Opción C):
  - Run → Run 'app' (Shift+F10)
  - Selecciona dispositivo
  - Espera a que se instale

Si compilaste en Terminal:
  - Abre emulador de Android Studio O conecta dispositivo USB
  - Ejecuta:

    Windows:  adb install -r app\build\outputs\apk\debug\app-debug.apk
    macOS:    adb install -r app/build/outputs/apk/debug/app-debug.apk


PASO 4: VERIFICAR
─────────────────
Busca "URU" en el dispositivo y abre la app. ¡Listo!


SI ALGO NO FUNCIONA
───────────────────
1. Verifica que JAVA_HOME y ANDROID_SDK_ROOT están configurados
2. Lee BUILD_INSTRUCTIONS.md (guía completa en español)
3. Lee COMPILE_CHECKLIST.md (lista de verificación paso a paso)


DOCUMENTACIÓN DISPONIBLE
────────────────────────
📄 BUILD_INSTRUCTIONS.md  - Guía COMPLETA de instalación y configuración
📄 QUICK_START.md         - Comandos rápidos y tareas comunes
📄 COMPILE_CHECKLIST.md   - Lista de verificación paso a paso


RESUMEN DE ARCHIVOS
───────────────────
✓ gradlew              - Script de compilación (macOS/Linux)
✓ gradlew.bat          - Script de compilación (Windows)
✓ gradle/wrapper/      - Configuración de gradle
✓ build.gradle         - Dependencias y configuración del proyecto
✓ settings.gradle      - Configuración del proyecto
✓ src/                 - Código fuente de la aplicación
✓ .gitignore           - Archivos a ignorar en git


¿PRIMERA VEZ?
─────────────
La primera compilación descargará ~500 MB de dependencias (puede tardar 5-10 min).
Las compilaciones siguientes serán más rápidas (2-5 min).


COMANDOS ÚTILES
───────────────
./gradlew assembleDebug      Compilar APK Debug
./gradlew assembleRelease    Compilar APK Release
./gradlew clean              Limpiar compilaciones previas
./gradlew test               Ejecutar tests
./gradlew build              Compilar completamente

Añade .bat en Windows (gradlew.bat)


SOPORTE
───────
Para problemas específicos, consulta:
1. BUILD_INSTRUCTIONS.md → Sección "Solución de Problemas"
2. QUICK_START.md → Tabla de problemas comunes


================================================================================
                    ¡Ahora ejecuta el comando de compilación!
================================================================================

Windows:    gradlew.bat clean assembleDebug
macOS/Linux: ./gradlew clean assembleDebug
Android Studio: Build → Make Project

================================================================================

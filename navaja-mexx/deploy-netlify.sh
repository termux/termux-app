#!/bin/bash
# Deploy to Netlify Drop - Automated Script

echo "🚀 Iniciando despliegue en Netlify..."

# Verificar que los archivos existen
if [ ! -f "index.html" ] || [ ! -f "manifest.json" ] || [ ! -f "sw.js" ]; then
    echo "❌ Error: Faltan archivos necesarios"
    exit 1
fi

# Crear directorio temporal
TEMP_DIR=$(mktemp -d)
echo "📁 Directorio temporal: $TEMP_DIR"

# Copiar archivos
cp index.html manifest.json sw.js "$TEMP_DIR/"
cd "$TEMP_DIR"

# Crear ZIP
zip -q navaja-mexx.zip index.html manifest.json sw.js
echo "📦 ZIP creado: navaja-mexx.zip"

# Mostrar instrucciones
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ ARCHIVOS LISTOS PARA NETLIFY DROP"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📋 Archivos a subir:"
ls -lh index.html manifest.json sw.js
echo ""
echo "🌐 URL: https://app.netlify.com/drop"
echo ""
echo "📱 Pasos:"
echo "1. Abre https://app.netlify.com/drop en tu navegador"
echo "2. Loguea con GitHub (sauljtm25@gmail.com)"
echo "3. Arrastra esta carpeta: $TEMP_DIR"
echo "4. Copia la URL generada"
echo "5. Agrega a pantalla de inicio desde Chrome/Safari"
echo ""
echo "🆗 Presiona Enter cuando termines de subir a Netlify..."
read

# Limpiar
rm -rf "$TEMP_DIR"
echo "✅ Despliegue completado!"

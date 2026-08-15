# 🚀 DEPLOYMENT GUIDE - NAVAJA MEXX v3

## Archivos Necesarios ✓
- `index.html` - Aplicación principal
- `manifest.json` - Configuración PWA
- `sw.js` - Service Worker para offline

---

## OPCIÓN 1: Netlify Drop ⚡ (RECOMENDADO - 2 minutos)

### Pasos:
1. **Descargar archivos**
   - Ir a: https://github.com/sauljtm33xD/termux-app/tree/claude/navaja-mexx-deployment-syeodk/navaja-mexx
   - Descargar los 3 archivos (index.html, manifest.json, sw.js)

2. **Abrir Netlify Drop**
   - En PC: https://app.netlify.com/drop
   - Arrastrar la carpeta `navaja-mexx/` completa

3. **Copiar URL generada**
   - Netlify te dará una URL como: `https://something.netlify.app`

4. **Agregar a pantalla de inicio**
   - **Android (Chrome)**: URL → Menú (⋮) → "Agregar a pantalla de inicio"
   - **iOS (Safari)**: URL → Compartir → "Agregar a pantalla de inicio"

✅ La app aparece como ícono nativo en el teléfono

---

## OPCIÓN 2: GitHub Pages 📄

### Pasos:
1. **En GitHub repo settings**
   - Settings → Pages → Source: `main` branch
   - Carpeta: `/navaja-mexx`

2. **URL será:**
   - `https://sauljtm33xD.github.io/termux-app/navaja-mexx`

3. **Agregar a pantalla de inicio**
   - Mismo proceso que Netlify Drop

✅ Despliegue permanente integrado en el repo

---

## OPCIÓN 3: Servidor Local 🐍

### En terminal:
```bash
cd navaja-mexx
python3 -m http.server 8080
```

### Acceder desde:
- **PC**: http://localhost:8080
- **Teléfono en red local**: http://[tu-ip-pc]:8080

---

## OPCIÓN 4: APK Nativo 📱 (Capacitor)

### En terminal:
```bash
npm install -g @capacitor/cli
cd navaja-mexx
npx cap init navaja-mexx com.navajamexx.app
npx cap add android
npx cap sync
# Abrir Android Studio y compilar
```

✅ APK compilado lista para instalar

---

## ⚠️ IMPORTANTE: HTTPS

- **API IA requiere HTTPS**
- ✅ Netlify: Incluido automáticamente (HTTPS gratis)
- ✅ GitHub Pages: Incluido automáticamente
- ❌ Servidor Local: No tiene HTTPS (solo para testing)
- ❌ APK: Necesita certificado auto-firmado o proxy

---

## 📋 Características

- **PWA**: Instalable como app nativa
- **Offline**: Service Worker implementado
- **Responsive**: Optimizado para móvil
- **API IA**: Integración con Claude/IA models
- **Dark Mode**: Tema cyberpunk incluido

---

## 🎯 Recomendación

**Para ir rápido:**
1. Opción 1 (Netlify Drop) - 2 minutos, app lista
2. Opción 2 (GitHub Pages) - Si quieres permanencia
3. Opción 3 (Local) - Si quieres debugging

**¿Cuál necesitas?** 👇

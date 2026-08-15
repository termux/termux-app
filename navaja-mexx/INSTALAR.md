# NAVAJA MEXX v3.0 — Guía de Instalación
**Versión:** 3.0 · **Motor IA:** claude-sonnet-4-6 (Anthropic) · **Actualizado:** 24/07/2026

---

## ARCHIVOS DEL PROYECTO
| Archivo | Tamaño | Descripción |
|---------|--------|-------------|
| `index.html` | ~256KB | App completa autocontenida |
| `manifest.json` | ~1KB | Config PWA |
| `sw.js` | ~1KB | Service Worker offline |
| `INSTALAR.md` | ~2KB | Esta guía |

---

## OPCIÓN 1: Netlify Drop ⭐ (recomendado — 2 min)
1. Ve a **netlify.app/drop** desde PC
2. Arrastra la carpeta **navaja-mexx/** completa
3. Copia la URL generada (ej: amazing-name.netlify.app)
4. Android: Chrome → URL → ⋮ → "Añadir a pantalla de inicio"
5. iOS: Safari → URL → Compartir → "Añadir a pantalla de inicio"

## OPCIÓN 2: GitHub Pages (gratuito y permanente)
```bash
git init && git add .
git commit -m "Navaja Mexx v3.0"
git remote add origin https://github.com/TU_USUARIO/navaja-mexx.git
git push -u origin main
# Settings → Pages → Source: main → URL: tuusuario.github.io/navaja-mexx
```

## OPCIÓN 3: Servidor local
```bash
python3 -m http.server 8080
# http://localhost:8080
```

## OPCIÓN 4: APK nativa con Capacitor
```bash
npm install @capacitor/core @capacitor/cli @capacitor/android
npx cap init "Navaja Mexx" "com.navajamexx.app"
npx cap add android
cp navaja-mexx/* www/
npx cap sync
npx cap open android
# Android Studio → Build → Generate Signed APK
```

---

## 11 MÓDULOS
| Tab | Módulo | Descripción |
|-----|--------|-------------|
| HQ | Intel HQ | IP pública, matriz amenazas, SIGINT, alert feed |
| CIPHER | Cifrado | Vigenère, Caesar, XOR, hashes, steganografía, OTP |
| RECON | Reconocimiento | Fingerprint, subdominios, SSL, metadatos |
| COMMS | IA Central | Chat claude-sonnet-4-6, 24 ops rápidas, informes |
| NET | Red | IP Intel, DNS, port scan, speed test, traceroute |
| OSINT | Inteligencia | WHOIS, Dorks, CVE, Shodan, identity, headers |
| SYS | Sistema | Device info, GPS, encode/decode, key generator |
| PROMPTS | Prompts IA | Generador, 26+ biblioteca, herramientas RT |
| TOOLS | Herramientas | Subnet, JSON, regex, HTTP, tokens, timestamp |
| MÁS | Utilidades | Calculadora, QR, notas, configuración |
| SHELL | Terminal | Chat2, terminal Linux simulada, info modelo IA |

---

## MOTOR IA — claude-sonnet-4-6
- **Modelo:** claude-sonnet-4-6 (Claude Sonnet 4.6)
- **Fabricante:** Anthropic PBC
- **Context window:** 1,000,000 tokens
- **Endpoint:** https://api.anthropic.com/v1/messages
- **Familia activa (jul 2026):**
  - claude-fable-5 (Mythos class, más potente)
  - claude-opus-4-8 (Opus flagship)
  - claude-opus-4-7 (visión 3x)
  - claude-sonnet-5 (nuevo Sonnet)
  - **claude-sonnet-4-6 ← MOTOR DE ESTA APP**
  - claude-haiku-4-5-20251001 (ultra-rápido)

---

## REQUISITOS
- **Android:** Chrome 90+ instalado
- **iOS:** Safari (modo standalone)
- **Desktop:** Chrome, Firefox, Edge (versiones recientes)
- **Internet:** Necesario para IA y consultas externas
- **Sin internet:** Cifrado, encode/decode, calculadora, regex, subnet, QR funcionan offline

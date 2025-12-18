# GitHub Actions Setup - Panduan Lengkap

## ⚠️ Warning: Action Required

Jika Anda melihat **"Action required"** di GitHub Actions, ikuti langkah-langkah di bawah:

---

## 🔧 Cara Fix Warning GitHub Actions

### **Langkah 1: Enable Workflow Permissions**

1. Buka repository GitHub Anda
2. Klik **Settings** (atas kanan)
3. Di sidebar, klik **Actions** → **General**
4. Scroll ke bagian "Workflow permissions"
5. Pilih: **☑ Read and write permissions**
6. Klik **Save**

**Screenshot lokasi:**
```
Settings → Actions → General → Workflow permissions
```

---

### **Langkah 2: Approve Workflow Run (jika diminta)**

Jika workflow masih waiting approval:

1. Buka tab **Actions**
2. Cari workflow yang status **"waiting approval"**
3. Klik workflow tersebut
4. Klik tombol **"Approve and run"** (jika ada)
5. Workflow akan mulai berjalan

---

## ✅ Verifikasi Build Berhasil

Setelah workflow run selesai:

1. ✅ Status workflow berubah menjadi **"Success"** (hijau)
2. ✅ Scroll ke bawah untuk melihat **"Artifacts"**
3. ✅ Download APK:
   - `termux-app_*_universal.apk` - untuk semua device
   - `termux-app_*_arm64-v8a.apk` - untuk 64-bit ARM
   - `termux-app_*_x86_64.apk` - untuk 64-bit Intel

---

## 🚀 Build Automation Flow

```
Anda push code ke GitHub
         ↓
GitHub Actions detects push
         ↓
Workflow "debug_build.yml" mulai berjalan
         ↓
Setup Java & Android SDK
         ↓
Build APK (2 variants: apt-android-7, apt-android-5)
         ↓
Generate APK untuk: universal, arm64-v8a, armeabi-v7a, x86_64, x86
         ↓
Upload artifacts
         ↓
Done! Download dari GitHub
```

---

## 📊 Artifact yang Dihasilkan

Setiap build menghasilkan:

```
Per Variant (2 total):
├── termux-app_*_universal.apk
├── termux-app_*_arm64-v8a.apk
├── termux-app_*_armeabi-v7a.apk
├── termux-app_*_x86_64.apk
├── termux-app_*_x86.apk
└── termux-app_*_sha256sums (checksum file)
```

**Total file per build**: ~12 artifacts

---

## 🔄 Trigger Build Manual

Jika ingin force build tanpa code push:

1. Buka tab **Actions**
2. Klik workflow **"Build"**
3. Klik **"Run workflow"** dropdown
4. Klik **"Run workflow"** button
5. Pilih branch: **master**
6. Klik **"Run workflow"**

---

## ❌ Troubleshooting

### Build gagal dengan error?

**Solusi:**
1. Buka workflow yang gagal
2. Klik **"Re-run jobs"** → **"Re-run all jobs"**
3. Atau fix code dan push ulang

### APK tidak bisa diinstall?

**Solusi:**
1. Download APK yang sesuai dengan device Anda:
   - **arm64-v8a** untuk kebanyakan phone modern
   - **universal** untuk semua device (ukuran paling besar)
2. Uninstall versi lama Termux terlebih dahulu
3. Install APK baru

### Artifact tidak terlihat?

**Solusi:**
1. Tunggu workflow selesai (status hijau ✅)
2. Scroll ke bawah di workflow detail page
3. Cari bagian "Artifacts"
4. Download dari sana

---

## 📝 Features yang Sudah Ditambahkan

Semua fitur berikut sudah siap di-build:

✅ **Edit Session** - Rename terminal session
✅ **Clear Session** - Hapus terminal output  
✅ **Copy Output** - Salin ke clipboard

Semuanya akan tersedia di drawer (geser dari kiri di app).

---

## 🎯 Next Steps

1. ✅ Fix permissions di GitHub Settings (jika ada warning)
2. ✅ Push code ke repository
3. ✅ Monitor GitHub Actions build
4. ✅ Download APK setelah build selesai
5. ✅ Test di Android device 7+

**Good luck! 🚀**

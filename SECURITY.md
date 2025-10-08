<!doctype html>
<html lang="pt-BR">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Inscrições - Simulado Nacional</title>
  <style>
    :root{
      --accent:#2b6cb0; /* azul */
      --muted:#64748b;
      --bg:#f6f8fb;
      --card:#ffffff;
      --danger:#e53e3e;
      font-family: Inter, system-ui, -apple-system, "Segoe UI", Roboto, "Helvetica Neue", Arial;
    }
    body{
      margin:0;
      background:linear-gradient(180deg,#eef4ff 0%,var(--bg) 100%);
      color:#0f172a;
      -webkit-font-smoothing:antialiased;
      -moz-osx-font-smoothing:grayscale;
      padding:32px;
      display:flex;
      justify-content:center;
      align-items:flex-start;
      min-height:100vh;
    }
    .container{
      width:100%;
      max-width:980px;
      display:grid;
      grid-template-columns:1fr 420px;
      gap:24px;
    }
    .card{
      background:var(--card);
      border-radius:12px;
      box-shadow:0 6px 20px rgba(15,23,42,0.06);
      padding:24px;
    }
    header h1{ margin:0; font-size:20px; }
    .lead{ color:var(--muted); margin-top:8px; font-size:14px; }

    form .row{ display:flex; gap:12px; margin-top:12px; }
    label{ font-size:13px; color:var(--muted); display:block; margin-bottom:6px; }
    input[type="text"], input[type="email"], select {
      width:100%;
      padding:10px 12px;
      border-radius:8px;
      border:1px solid #e6eef8;
      background:#fbfdff;
      font-size:15px;
      box-sizing:border-box;
    }
    .small{ font-size:13px; color:var(--muted); margin-top:8px; }

    .sidebar{
      position:sticky;
      top:28px;
      height:fit-content;
    }
    .price{
      font-size:28px;
      font-weight:600;
      color:var(--accent);

    

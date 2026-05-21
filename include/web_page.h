/**
 * @file    web_page.h
 * @brief   Single-page web UI served by web_config, stored as a C string.
 *
 * The page is deliberately self-contained (no external assets) so it loads
 * with no internet access. It talks to the firmware over a tiny JSON API:
 *   GET  /api/state            -> full state (config + telemetry + stats)
 *   POST /api/pid              -> {kp,ki,kd}
 *   POST /api/setpoints        -> {sp:[s0,s1,s2,s3]}
 *   POST /api/save             -> persist config to flash
 *   POST /api/reset_stats      -> clear statistics
 *   POST /api/reset_defaults   -> restore default config
 */
#ifndef WEB_PAGE_H
#define WEB_PAGE_H

static const char WEB_PAGE_HTML[] =
"<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>Actuator Controller</title><style>"
"*{box-sizing:border-box}body{font-family:system-ui,Segoe UI,Roboto,sans-serif;"
"margin:0;background:#0f1419;color:#e6e6e6}"
"header{background:#1a2129;padding:16px 20px;border-bottom:1px solid #2a333d}"
"h1{margin:0;font-size:18px}h2{font-size:14px;text-transform:uppercase;"
"letter-spacing:.05em;color:#8aa;margin:0 0 12px}"
".wrap{max-width:760px;margin:0 auto;padding:20px}"
".card{background:#1a2129;border:1px solid #2a333d;border-radius:10px;"
"padding:18px;margin-bottom:18px}"
".row{display:flex;gap:12px;flex-wrap:wrap}"
".fld{flex:1;min-width:120px;margin-bottom:10px}"
"label{display:block;font-size:12px;color:#9ab;margin-bottom:4px}"
"input{width:100%;padding:8px;border-radius:6px;border:1px solid #38424d;"
"background:#0f1419;color:#fff;font-size:14px}"
"button{background:#2d7ff9;color:#fff;border:0;border-radius:6px;"
"padding:9px 16px;font-size:14px;cursor:pointer}"
"button:hover{background:#1f6fe0}button.sec{background:#38424d}"
"button.danger{background:#b0413e}"
".btns{display:flex;gap:10px;flex-wrap:wrap;margin-top:6px}"
".tel{display:flex;gap:18px;flex-wrap:wrap}"
".tel div{flex:1;min-width:90px}.big{font-size:22px;font-weight:600}"
".muted{color:#8aa;font-size:12px}"
"table{width:100%;border-collapse:collapse;font-size:13px}"
"th,td{text-align:right;padding:6px 8px;border-bottom:1px solid #2a333d}"
"th:first-child,td:first-child{text-align:left}"
"#toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);"
"background:#2d7ff9;color:#fff;padding:10px 18px;border-radius:6px;"
"opacity:0;transition:.3s;pointer-events:none}#toast.show{opacity:1}"
".badge{display:inline-block;background:#2d7ff9;border-radius:5px;"
"padding:2px 8px;font-size:12px;font-weight:600}"
"</style></head><body>"
"<header><h1>PX4 Actuator Controller</h1>"
"<span class=\"muted\">live tuning &amp; debug</span></header>"
"<div class=\"wrap\">"

/* ---- Live telemetry ---- */
"<div class=\"card\"><h2>Live</h2><div class=\"tel\">"
"<div><div class=\"muted\">Mode</div><div class=\"big\"><span id=\"t_mode\">-</span> "
"<span class=\"badge\" id=\"t_bits\">--</span></div></div>"
"<div><div class=\"muted\">Target RPM</div><div class=\"big\" id=\"t_tgt\">-</div></div>"
"<div><div class=\"muted\">Measured RPM</div><div class=\"big\" id=\"t_meas\">-</div></div>"
"<div><div class=\"muted\">PWM out (us)</div><div class=\"big\" id=\"t_out\">-</div></div>"
"</div></div>"

/* ---- PID ---- */
"<div class=\"card\"><h2>PID gains</h2><div class=\"row\">"
"<div class=\"fld\"><label>Kp</label><input id=\"kp\" type=\"number\" step=\"0.001\"></div>"
"<div class=\"fld\"><label>Ki</label><input id=\"ki\" type=\"number\" step=\"0.001\"></div>"
"<div class=\"fld\"><label>Kd</label><input id=\"kd\" type=\"number\" step=\"0.001\"></div>"
"</div><div class=\"btns\"><button onclick=\"applyPid()\">Apply</button></div></div>"

/* ---- Setpoints ---- */
"<div class=\"card\"><h2>Mode setpoints (RPM)</h2><div class=\"row\">"
"<div class=\"fld\"><label>Mode 00</label><input id=\"sp0\" type=\"number\" step=\"1\"></div>"
"<div class=\"fld\"><label>Mode 01</label><input id=\"sp1\" type=\"number\" step=\"1\"></div>"
"<div class=\"fld\"><label>Mode 10</label><input id=\"sp2\" type=\"number\" step=\"1\"></div>"
"<div class=\"fld\"><label>Mode 11</label><input id=\"sp3\" type=\"number\" step=\"1\"></div>"
"</div><div class=\"btns\"><button onclick=\"applySp()\">Apply</button></div></div>"

/* ---- Stats ---- */
"<div class=\"card\"><h2>RPM statistics</h2>"
"<table><thead><tr><th>Bucket</th><th>Min</th><th>Max</th><th>Avg</th><th>Samples</th></tr>"
"</thead><tbody id=\"stats\"></tbody></table>"
"<div class=\"btns\"><button class=\"sec\" onclick=\"resetStats()\">Reset stats</button></div></div>"

/* ---- Persist ---- */
"<div class=\"card\"><h2>Storage</h2>"
"<p class=\"muted\">PID gains and setpoints are kept in RAM until saved. "
"Save writes them to flash so they survive a reboot.</p>"
"<div class=\"btns\"><button onclick=\"save()\">Save to flash</button>"
"<button class=\"danger\" onclick=\"resetDefaults()\">Restore defaults</button></div></div>"

"</div><div id=\"toast\"></div>"

/* ---- Script ---- */
"<script>"
"function $(i){return document.getElementById(i)}"
"function toast(m){var t=$('toast');t.textContent=m;t.classList.add('show');"
"setTimeout(function(){t.classList.remove('show')},1500)}"
"function post(u,b){return fetch(u,{method:'POST',headers:{'Content-Type':'application/json'},"
"body:b?JSON.stringify(b):null})}"
"var dirty={};"
"['kp','ki','kd','sp0','sp1','sp2','sp3'].forEach(function(id){"
"$(id).addEventListener('focus',function(){dirty[id]=1});"
"$(id).addEventListener('blur',function(){dirty[id]=0})});"
"function setIf(id,v){if(!dirty[id]&&document.activeElement!==$(id))$(id).value=v}"
"function modeBits(m){return ('0'+m.toString(2)).slice(-2)}"
"function refresh(){fetch('/api/state').then(function(r){return r.json()}).then(function(s){"
"$('t_mode').textContent=s.mode;$('t_bits').textContent=modeBits(s.mode);"
"$('t_tgt').textContent=s.target_rpm.toFixed(0);"
"$('t_meas').textContent=s.measured_rpm.toFixed(0);"
"$('t_out').textContent=s.output_us.toFixed(0);"
"setIf('kp',s.kp);setIf('ki',s.ki);setIf('kd',s.kd);"
"for(var i=0;i<4;i++)setIf('sp'+i,s.sp[i]);"
"var rows='';function row(n,b){return '<tr><td>'+n+'</td><td>'+b.min.toFixed(0)+"
"'</td><td>'+b.max.toFixed(0)+'</td><td>'+b.avg.toFixed(0)+'</td><td>'+b.samples+'</td></tr>'}"
"rows+=row('Overall',s.stats.overall);"
"for(var j=0;j<4;j++)rows+=row('Mode '+modeBits(j),s.stats.mode[j]);"
"$('stats').innerHTML=rows;"
"}).catch(function(){})}"
"function applyPid(){post('/api/pid',{kp:+$('kp').value,ki:+$('ki').value,kd:+$('kd').value})"
".then(function(){toast('PID applied')})}"
"function applySp(){post('/api/setpoints',{sp:[+$('sp0').value,+$('sp1').value,"
"+$('sp2').value,+$('sp3').value]}).then(function(){toast('Setpoints applied')})}"
"function save(){post('/api/save').then(function(){toast('Saved to flash')})}"
"function resetStats(){post('/api/reset_stats').then(function(){toast('Stats reset')})}"
"function resetDefaults(){if(confirm('Restore default gains and setpoints?'))"
"post('/api/reset_defaults').then(function(){toast('Defaults restored')})}"
"refresh();setInterval(refresh,500);"
"</script></body></html>";

#endif /* WEB_PAGE_H */
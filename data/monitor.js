function setText(id, value) {
  const el = document.getElementById(id);
  if (!el) return;
  el.textContent = value;
}

function updateStatus() {
  fetch('/state')
    .then((res) => res.json())
    .then((data) => {
      setText('modo', data.modo || '--');
      setText('ui', data.interfaceMode || '--');
      setText('distancia', `${data.distance ?? '--'} cm`);
      setText('estado', data.distance != null && data.distance >= 0 ? 'OK' : 'N/A');
      setText('ultra-status', data.ultra?.status || '--');
      setText('ultra-filtro', data.ultra?.filter || '--');
      setText('enc-esq', data.encoderLeft ?? '--');
      setText('enc-dir', data.encoderRight ?? '--');

      setText('ax', data.accelX ?? '--');
      setText('ay', data.accelY ?? '--');
      setText('az', data.accelZ ?? '--');
      setText('gx', data.gyroX ?? '--');
      setText('gy', data.gyroY ?? '--');
      setText('gz', data.gyroZ ?? '--');

      setText('ultra-hz', `${data.ultra?.hz ?? '--'} Hz`);
      setText('ultra-cal', data.ultra?.calibration ?? '--');
      setText('ultra-age', `${data.ultra?.ageMs ?? '--'} ms`);
      setText('ultra-valid', data.ultra?.stats?.validReads ?? '--');
      setText('ultra-reads', data.ultra?.stats?.reads ?? '--');

      const reads = Number(data.ultra?.stats?.reads ?? 0);
      const valids = Number(data.ultra?.stats?.validReads ?? 0);
      const quality = reads > 0 ? Math.round((valids / reads) * 100) : 0;
      setText('ultra-quality', `${quality} %`);
    })
    .catch(() => {
      setText('estado', 'Sem resposta');
    });
}

fetch('/controle?cmd=UI MONITOR');
updateStatus();
setInterval(updateStatus, 500);

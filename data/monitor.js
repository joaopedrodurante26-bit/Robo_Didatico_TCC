function setText(id, value) {
  const el = document.getElementById(id);
  if (!el) return;
  el.textContent = value;
}

function updateStatus() {
  fetch('/status')
    .then((res) => res.json())
    .then((data) => {
      setText('modo', data.modo || '--');
      setText('ui', data.ui || '--');
      setText('distancia', `${data.distancia ?? '--'} cm`);
      setText('estado', data.estado || '--');
      setText('enc-esq', data.encoder_esq ?? '--');
      setText('enc-dir', data.encoder_dir ?? '--');

      setText('ax', data.accel?.x ?? '--');
      setText('ay', data.accel?.y ?? '--');
      setText('az', data.accel?.z ?? '--');
      setText('gx', data.gyro?.x ?? '--');
      setText('gy', data.gyro?.y ?? '--');
      setText('gz', data.gyro?.z ?? '--');
    })
    .catch(() => {
      setText('estado', 'Sem resposta');
    });
}

fetch('/controle?cmd=UI MONITOR');
updateStatus();
setInterval(updateStatus, 500);

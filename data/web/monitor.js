function setText(id, value) {
  const el = document.getElementById(id);
  if (!el) return;
  el.textContent = value;
}

function asNumber(value, fallback = 0) {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function formatSigned(value) {
  return value >= 0 ? `+${value}` : `${value}`;
}

const encoderState = {
  lastTs: 0,
  left: null,
  right: null,
};

function updateStatus() {
  fetch('/state', { cache: 'no-store' })
    .then((res) => res.json())
    .then((data) => {
      setText('modo', data.mode || '--');
      setText('ui', data.interfaceMode || '--');
      setText('distancia', `${data.distance ?? '--'} cm`);
      setText('estado', data.ultra?.status || (data.distance != null && data.distance >= 0 ? 'OK' : 'N/A'));
      setText('ultra-status', data.ultra?.status || '--');
      setText('ultra-filtro', data.ultra?.filter || '--');

      const now = performance.now();
      const left = asNumber(data.encoderLeft, 0);
      const right = asNumber(data.encoderRight, 0);

      setText('enc-esq', left);
      setText('enc-dir', right);
      setText('enc-diff', Math.abs(left - right));

      if (encoderState.left != null && encoderState.right != null && encoderState.lastTs > 0) {
        const elapsedSec = Math.max((now - encoderState.lastTs) / 1000, 0.001);
        const deltaLeft = left - encoderState.left;
        const deltaRight = right - encoderState.right;
        const rateLeft = (deltaLeft / elapsedSec).toFixed(1);
        const rateRight = (deltaRight / elapsedSec).toFixed(1);

        setText('enc-esq-rate', `${rateLeft} p/s`);
        setText('enc-dir-rate', `${rateRight} p/s`);
        setText('enc-esq-delta', `Δ ${formatSigned(deltaLeft)}`);
        setText('enc-dir-delta', `Δ ${formatSigned(deltaRight)}`);
      } else {
        setText('enc-esq-rate', '-- p/s');
        setText('enc-dir-rate', '-- p/s');
        setText('enc-esq-delta', 'Δ --');
        setText('enc-dir-delta', 'Δ --');
      }

      encoderState.left = left;
      encoderState.right = right;
      encoderState.lastTs = now;

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
setInterval(updateStatus, 120);

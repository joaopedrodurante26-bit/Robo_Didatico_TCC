function sendCmd(cmd) {
  return fetch(`/controle?cmd=${encodeURIComponent(cmd)}`);
}

const speed = document.getElementById('speed');
const speedValue = document.getElementById('speed-value');
let holdTimer = null;
let activeMove = '';

function v() {
  return Number(speed.value);
}

speed.addEventListener('input', () => {
  speedValue.textContent = speed.value;
});

function startHold(moveToken) {
  stopHold();
  activeMove = moveToken;

  // Garante que o robô esteja em controle manual.
  sendCmd('MODE MANUAL');
  sendCmd(`${activeMove} ${v()}`);

  holdTimer = setInterval(() => {
    sendCmd(`${activeMove} ${v()}`);
  }, 120);
}

function stopHold() {
  if (holdTimer) {
    clearInterval(holdTimer);
    holdTimer = null;
  }

  if (activeMove) {
    activeMove = '';
    sendCmd('STOP');
  }
}

function bindPressButton(id, moveToken) {
  const el = document.getElementById(id);

  const press = (e) => {
    e.preventDefault();
    startHold(moveToken);
  };

  const release = (e) => {
    e.preventDefault();
    stopHold();
  };

  el.addEventListener('pointerdown', press);
  el.addEventListener('pointerup', release);
  el.addEventListener('pointercancel', release);
  el.addEventListener('pointerleave', release);
}

bindPressButton('btn-front', 'F');
bindPressButton('btn-back', 'T');
bindPressButton('btn-left', 'VE');
bindPressButton('btn-right', 'VD');

const stopBtn = document.getElementById('btn-stop');
stopBtn.addEventListener('click', (e) => {
  e.preventDefault();
  stopHold();
  sendCmd('STOP');
});

sendCmd('UI CONTROL');
sendCmd('MODE MANUAL');
sendCmd('MOTOR');

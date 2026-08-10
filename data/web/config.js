const msg = document.getElementById('cfg-msg');
const modeSelect = document.getElementById('mode-select');
const uiSelect = document.getElementById('ui-select');
const btnSave = document.getElementById('btn-save');

function line(text) {
  const el = document.createElement('div');
  el.className = 'line info';
  el.textContent = text;
  msg.appendChild(el);
}

function sendCmd(cmd) {
  return fetch(`/controle?cmd=${encodeURIComponent(cmd)}`);
}

btnSave.addEventListener('click', async () => {
  msg.innerHTML = '';
  try {
    await sendCmd(modeSelect.value);
    await sendCmd(uiSelect.value);
    line('[OK] Configuração aplicada');
  } catch (e) {
    line('[ERRO] Falha ao aplicar configuração');
  }
});

sendCmd('UI CONFIG');
line('Ajuste os parâmetros e clique em Aplicar.');

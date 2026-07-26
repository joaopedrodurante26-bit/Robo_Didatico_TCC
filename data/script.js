const output = document.getElementById('output');
const cmdInput = document.getElementById('cmd');
const btnEnviar = document.getElementById('btnEnviar');

function appendLine(text, type = 'info') {
  const line = document.createElement('div');
  line.className = `line ${type}`;
  line.textContent = text;
  output.appendChild(line);
  output.scrollTop = output.scrollHeight;
}

function atualizarStatus() {
  fetch('/status')
    .then((res) => res.json())
    .then((data) => {
      document.getElementById('distancia').textContent = `${data.distancia ?? '--'} cm`;
      document.getElementById('estado').textContent = data.estado ?? 'Aguardando';
      document.getElementById('modo').textContent = data.modo ?? 'IDLE';
      document.getElementById('wifi').textContent = 'ROBO_VESPA';
    })
    .catch(() => {
      document.getElementById('estado').textContent = 'Sem resposta';
    });
}

function enviarComando() {
  const cmd = cmdInput.value.trim();
  if (!cmd) return;

  appendLine(`> ${cmd}`, 'cmd');
  cmdInput.value = '';

  fetch(`/controle?x=0&y=0`) 
    .then(() => {
      appendLine('Comando enviado para o robô.', 'ok');
    })
    .catch(() => {
      appendLine('Falha ao enviar comando.', 'error');
    });
}

btnEnviar.addEventListener('click', enviarComando);
cmdInput.addEventListener('keydown', (e) => {
  if (e.key === 'Enter') enviarComando();
});

appendLine('ROBO> Sistema pronto', 'info');
appendLine('ROBO> Digite um comando para interagir', 'info');
setInterval(atualizarStatus, 1000);
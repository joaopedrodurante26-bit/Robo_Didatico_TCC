const fsPathInput = document.getElementById('fs-path');
const fsList = document.getElementById('fs-list');
const fsPreview = document.getElementById('fs-preview');
const fsMeta = document.getElementById('fs-meta');
const fsBreadcrumb = document.getElementById('fs-breadcrumb');
const fsCount = document.getElementById('fs-count');
const fsStatus = document.getElementById('fs-status');
const fsDownload = document.getElementById('fs-download');
const fsCopy = document.getElementById('fs-copy');
const fsMove = document.getElementById('fs-move');
const fsDelete = document.getElementById('fs-delete');
const fsRefresh = document.getElementById('fs-refresh');
const fsOpen = document.getElementById('fs-open');
const fsUp = document.getElementById('fs-up');
const fsUploadInput = document.getElementById('fs-upload-input');
const fsUploadBtn = document.getElementById('fs-upload-btn');

let currentPath = '/';
let selectedItem = null;

function normalizePath(path) {
  const raw = String(path || '').trim();
  if (!raw) return '/';
  if (raw === '/') return '/';
  const withSlash = raw.startsWith('/') ? raw : `/${raw}`;
  return withSlash.replace(/\/+/g, '/').replace(/\/+$/g, '') || '/';
}

function joinPath(base, child) {
  const a = normalizePath(base);
  const b = String(child || '').trim();
  if (!b) return a;
  if (b.startsWith('/')) return normalizePath(b);
  if (a === '/') return normalizePath(`/${b}`);
  return normalizePath(`${a}/${b}`);
}

function setStatus(text) {
  fsStatus.textContent = text;
}

function renderBreadcrumb(path) {
  const normalized = normalizePath(path);
  const parts = normalized === '/' ? [] : normalized.split('/').filter(Boolean);
  fsBreadcrumb.innerHTML = '';

  const root = document.createElement('button');
  root.className = 'crumb';
  root.textContent = '/';
  root.addEventListener('click', () => openPath('/'));
  fsBreadcrumb.appendChild(root);

  let acc = '';
  parts.forEach((part) => {
    acc += `/${part}`;
    const segmentPath = acc;
    const sep = document.createElement('span');
    sep.className = 'crumb-sep';
    sep.textContent = '/';
    fsBreadcrumb.appendChild(sep);

    const item = document.createElement('button');
    item.className = 'crumb';
    item.textContent = part;
    item.addEventListener('click', () => openPath(segmentPath));
    fsBreadcrumb.appendChild(item);
  });
}

function renderList(items) {
  fsList.innerHTML = '';

  if (!items.length) {
    const empty = document.createElement('div');
    empty.className = 'fs-empty';
    empty.textContent = 'Diretorio vazio.';
    fsList.appendChild(empty);
    return;
  }

  items.forEach((item) => {
    const row = document.createElement('button');
    row.className = `fs-item ${item.type}`;
    row.type = 'button';

    const label = document.createElement('div');
    label.className = 'fs-item-label';
    label.textContent = item.name || item.path;

    const meta = document.createElement('div');
    meta.className = 'fs-item-meta';
    meta.textContent = item.type === 'directory' ? 'Pasta' : `${item.size || 0} bytes`;

    const actions = document.createElement('div');
    actions.className = 'fs-item-actions';

    if (item.type === 'directory') {
      const openBtn = document.createElement('span');
      openBtn.className = 'fs-pill';
      openBtn.textContent = 'Abrir';
      actions.appendChild(openBtn);
      row.addEventListener('click', () => openPath(item.path));
    } else {
      const previewBtn = document.createElement('span');
      previewBtn.className = 'fs-pill';
      previewBtn.textContent = 'Ver';
      actions.appendChild(previewBtn);
      row.addEventListener('click', () => openFile(item));
    }

    row.appendChild(label);
    row.appendChild(meta);
    row.appendChild(actions);
    fsList.appendChild(row);
  });
}

function updateActionButtons() {
  const enabled = Boolean(selectedItem && selectedItem.type === 'file');
  fsDownload.disabled = !enabled;
  fsCopy.disabled = !enabled;
  fsMove.disabled = !enabled;
  fsDelete.disabled = !enabled;
}

async function loadDirectory(path) {
  const target = normalizePath(path);
  currentPath = target;
  fsPathInput.value = target;
  renderBreadcrumb(target);
  selectedItem = null;
  updateActionButtons();
  fsMeta.textContent = 'Selecione um arquivo para visualizar.';
  fsPreview.textContent = 'Nada selecionado.';
  setStatus('Carregando...');

  const response = await fetch(`/fs-api?action=list&path=${encodeURIComponent(target)}`);
  const data = await response.json();

  if (!response.ok || !data.ok) {
    throw new Error(data.error || 'Falha ao listar');
  }

  if (data.type === 'file') {
    const fileItem = {
      name: target.split('/').filter(Boolean).pop() || target,
      path: target,
      type: 'file',
      size: data.size || 0,
    };

    fsCount.textContent = '1 item';
    renderList([fileItem]);
    await openFile(fileItem);
    return;
  }

  const items = Array.isArray(data.items) ? data.items.slice() : [];
  items.sort((a, b) => {
    if (a.type !== b.type) return a.type === 'directory' ? -1 : 1;
    return String(a.name || '').localeCompare(String(b.name || ''));
  });

  fsCount.textContent = `${items.length} item${items.length === 1 ? '' : 's'}`;
  renderList(items);
  setStatus(`Aberto: ${target}`);
}

async function openPath(path) {
  try {
    await loadDirectory(path);
  } catch (error) {
    setStatus('Erro');
    fsList.innerHTML = '';
    fsPreview.textContent = error.message || 'Falha ao abrir caminho.';
  }
}

async function openFile(item) {
  selectedItem = item;
  updateActionButtons();
  fsMeta.textContent = `${item.path} | ${item.size || 0} bytes`;
  fsPreview.textContent = 'Carregando arquivo...';
  setStatus('Lendo arquivo...');

  try {
    const response = await fetch(`/fs-api?action=read&path=${encodeURIComponent(item.path)}`);
    const text = await response.text();

    if (!response.ok) {
      throw new Error(text || 'Falha ao ler arquivo');
    }

    fsPreview.textContent = text || '(arquivo vazio)';
    setStatus(`Arquivo aberto: ${item.path}`);
  } catch (error) {
    fsPreview.textContent = error.message || 'Falha ao ler arquivo.';
    setStatus('Erro ao ler');
  }
}

function downloadSelected() {
  if (!selectedItem) return;
  const url = `/fs-api?action=download&path=${encodeURIComponent(selectedItem.path)}`;
  window.open(url, '_blank', 'noopener');
}

async function copyOrMoveSelected(action) {
  if (!selectedItem) return;

  const defaultName = selectedItem.path.split('/').filter(Boolean).pop() || 'arquivo';
  const suggested = joinPath(fsPathInput.value || currentPath, defaultName);
  const target = window.prompt(`Destino para ${action === 'copy' ? 'copiar' : 'mover'}:`, suggested);

  if (!target) {
    return;
  }

  const response = await fetch(
    `/fs-api?action=${action}&source=${encodeURIComponent(selectedItem.path)}&dest=${encodeURIComponent(target)}`
  );
  const data = await response.json();

  if (!response.ok || !data.ok) {
    throw new Error(data.error || 'Falha na operacao');
  }

  setStatus(action === 'copy' ? `Copiado para ${data.dest}` : `Movido para ${data.dest}`);
  await openPath(currentPath);
}

async function deleteSelected() {
  if (!selectedItem) return;

  const deletedPath = selectedItem.path;
  const confirmText = `Digite CONFIRMAR para apagar ${selectedItem.path}`;
  if (window.prompt(confirmText) !== 'CONFIRMAR') {
    return;
  }

  setStatus('Excluindo...');

  const response = await fetch(`/fs-api?action=delete&path=${encodeURIComponent(deletedPath)}&confirm=1`);
  const data = await response.json();

  if (!response.ok || !data.ok) {
    throw new Error(data.error || 'Falha ao excluir');
  }

  await loadDirectory(currentPath);
  setStatus(`Removido: ${deletedPath}`);
}

async function uploadSelectedFile() {
  const file = fsUploadInput.files && fsUploadInput.files[0];
  if (!file) {
    setStatus('Selecione um arquivo para enviar.');
    return;
  }

  const destination = fsPathInput.value || currentPath;
  const formData = new FormData();
  formData.append('file', file, file.name);

  setStatus('Enviando arquivo...');

  const response = await fetch(`/fs-upload?path=${encodeURIComponent(destination)}`, {
    method: 'POST',
    body: formData,
  });

  const data = await response.json();
  if (!response.ok || !data.ok) {
    throw new Error(data.error || 'Falha ao enviar arquivo');
  }

  fsUploadInput.value = '';
  setStatus(`Enviado: ${data.path}`);
  await openPath(currentPath);
}

fsOpen.addEventListener('click', () => openPath(fsPathInput.value));
fsRefresh.addEventListener('click', () => openPath(currentPath));
fsUp.addEventListener('click', () => {
  if (currentPath === '/') return;
  const parts = normalizePath(currentPath).split('/').filter(Boolean);
  parts.pop();
  openPath(parts.length ? `/${parts.join('/')}` : '/');
});
fsDelete.addEventListener('click', () => {
  deleteSelected().catch((error) => {
    setStatus('Erro');
    fsPreview.textContent = error.message || 'Falha ao excluir.';
  });
});
fsDownload.addEventListener('click', downloadSelected);
fsCopy.addEventListener('click', () => {
  copyOrMoveSelected('copy').catch((error) => {
    setStatus('Erro');
    fsPreview.textContent = error.message || 'Falha ao copiar.';
  });
});
fsMove.addEventListener('click', () => {
  copyOrMoveSelected('move').catch((error) => {
    setStatus('Erro');
    fsPreview.textContent = error.message || 'Falha ao mover.';
  });
});
fsUploadBtn.addEventListener('click', () => {
  uploadSelectedFile().catch((error) => {
    setStatus('Erro');
    fsPreview.textContent = error.message || 'Falha ao enviar arquivo.';
  });
});
fsPathInput.addEventListener('keydown', (event) => {
  if (event.key === 'Enter') {
    openPath(fsPathInput.value);
  }
});

document.addEventListener('keydown', (event) => {
  if (event.key === 'Escape') {
    fsPathInput.blur();
  }
});

openPath('/');
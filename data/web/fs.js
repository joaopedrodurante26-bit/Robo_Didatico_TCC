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
const fsStorageSummary = document.getElementById('fs-storage-summary');
const fsTotalBytes = document.getElementById('fs-total-bytes');
const fsUsedBytes = document.getElementById('fs-used-bytes');
const fsSafeFreeBytes = document.getElementById('fs-safe-free-bytes');
const fsStorageBarUsed = document.getElementById('fs-storage-bar-used');
const fsUploadHint = document.getElementById('fs-upload-hint');

let currentPath = '/';
let selectedItem = null;
let fsStats = null;

function isProtectedItem(item) {
  return Boolean(item && item.protected);
}

function formatBytes(bytes) {
  const value = Number(bytes || 0);
  if (value < 1024) return `${value} B`;
  if (value < 1024 * 1024) return `${(value / 1024).toFixed(1)} KB`;
  return `${(value / (1024 * 1024)).toFixed(2)} MB`;
}

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

function updateStoragePanel(stats) {
  fsStats = stats || null;

  if (!fsStats) {
    fsStorageSummary.textContent = 'Metricas indisponiveis.';
    fsTotalBytes.textContent = '-';
    fsUsedBytes.textContent = '-';
    fsSafeFreeBytes.textContent = '-';
    fsStorageBarUsed.style.width = '0%';
    updateUploadEligibility();
    return;
  }

  const total = Number(fsStats.totalBytes || 0);
  const used = Number(fsStats.usedBytes || 0);
  const free = Number(fsStats.freeBytes || 0);
  const safeFree = Number(fsStats.safeFreeBytes || 0);
  const reserved = Number(fsStats.reservedBytes || 0);
  const usedPercent = total > 0 ? Math.min(100, Math.round((used / total) * 100)) : 0;

  fsTotalBytes.textContent = formatBytes(total);
  fsUsedBytes.textContent = `${formatBytes(used)} (${usedPercent}%)`;
  fsSafeFreeBytes.textContent = formatBytes(safeFree);
  fsStorageSummary.textContent = `Livre atual: ${formatBytes(free)}. Reserva de seguranca: ${formatBytes(reserved)}.`;
  fsStorageBarUsed.style.width = `${usedPercent}%`;

  updateUploadEligibility();
}

function updateUploadEligibility() {
  const file = fsUploadInput.files && fsUploadInput.files[0];
  const safeFree = Number(fsStats && fsStats.safeFreeBytes ? fsStats.safeFreeBytes : 0);

  if (!file) {
    fsUploadBtn.disabled = false;
    fsUploadHint.textContent = fsStats
      ? `Espaco seguro disponivel para upload: ${formatBytes(safeFree)}.`
      : 'Selecione um arquivo para validar o upload.';
    return;
  }

  const fits = !fsStats || file.size <= safeFree;
  fsUploadBtn.disabled = !fits;
  fsUploadHint.textContent = fits
    ? `Arquivo selecionado: ${file.name} (${formatBytes(file.size)}).`
    : `Arquivo muito grande para o espaco seguro restante: ${formatBytes(file.size)} de ${formatBytes(safeFree)}.`;
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
  fsMove.disabled = !enabled || isProtectedItem(selectedItem);
  fsDelete.disabled = !enabled || isProtectedItem(selectedItem);
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
    updateStoragePanel(data.stats || null);

    const fileItem = {
      name: target.split('/').filter(Boolean).pop() || target,
      path: target,
      type: 'file',
      size: data.size || 0,
      protected: Boolean(data.protected),
    };

    fsCount.textContent = '1 item';
    renderList([fileItem]);
    await openFile(fileItem);
    return;
  }

  const items = Array.isArray(data.items) ? data.items.slice() : [];
  updateStoragePanel(data.stats || null);
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
  if (isProtectedItem(item)) {
    fsMeta.textContent += ' | protegido';
  }
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
  if (isProtectedItem(selectedItem)) {
    throw new Error('Arquivo protegido contra exclusao.');
  }

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

  if (fsStats && file.size > Number(fsStats.safeFreeBytes || 0)) {
    throw new Error('Arquivo excede o espaco seguro disponivel na particao.');
  }

  const destination = fsPathInput.value || currentPath;
  if (/^\/(web|help)(\/|$)/.test(normalizePath(destination)) || normalizePath(destination) === '/boot_log.json' || normalizePath(destination) === '/ultra_config.json') {
    throw new Error('Destino protegido contra escrita.');
  }
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
fsUploadInput.addEventListener('change', updateUploadEligibility);
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
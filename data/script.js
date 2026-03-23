function atualizarStatus() {
    fetch('/status')
        .then(response => response.json())
        .then(data => {
            document.getElementById('distancia').innerText = data.distancia;
            document.getElementById('bateria').innerText = data.bateria;
            document.getElementById('estado').innerText = data.estado;
        })
        .catch(err => console.log("Erro:", err));
}

// Atualiza a cada 1 segundo
setInterval(atualizarStatus, 1000);
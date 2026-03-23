const joystick = document.getElementById("joystick");
const stick = document.getElementById("stick");

let centerX = 100;
let centerY = 100;

let active = false;

joystick.addEventListener("mousedown", () => active = true);
document.addEventListener("mouseup", () => {
    active = false;
    resetStick();
    enviar(0, 0);
});

document.addEventListener("mousemove", (e) => {
    if (!active) return;

    const rect = joystick.getBoundingClientRect();

    let x = e.clientX - rect.left - centerX;
    let y = e.clientY - rect.top - centerY;

    // Limita ao círculo
    let max = 80;
    x = Math.max(-max, Math.min(max, x));
    y = Math.max(-max, Math.min(max, y));

    stick.style.left = (x + centerX - 30) + "px";
    stick.style.top = (y + centerY - 30) + "px";

    // Normaliza (-1 a 1)
    let normX = x / max;
    let normY = -y / max;

    enviar(normX, normY);
});

function resetStick() {
    stick.style.left = "70px";
    stick.style.top = "70px";
}

function enviar(x, y) {
    fetch(`/controle?x=${x}&y=${y}`);
}

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
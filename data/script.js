const joystick = document.getElementById("joystick");
const stick = document.getElementById("stick");

let active = false;

const max = 80;

joystick.addEventListener("mousedown", (e) => {
    active = true;
    mover(e);
});

document.addEventListener("mousemove", (e) => {
    if (!active) return;
    mover(e);
});

document.addEventListener("mouseup", () => {
    active = false;
    resetStick();
    enviar(0, 0);
});

joystick.addEventListener("touchstart", (e) => {
    active = true;
    mover(e.touches[0]);
});

joystick.addEventListener("touchmove", (e) => {
    e.preventDefault();
    if (!active) return;
    mover(e.touches[0]);
}, { passive: false });

joystick.addEventListener("touchend", () => {
    active = false;
    resetStick();
    enviar(0, 0);
});

function mover(e) {
    const rect = joystick.getBoundingClientRect();

    let x = e.clientX - rect.left - rect.width / 2;
    let y = e.clientY - rect.top - rect.height / 2;

    // Limita ao círculo
    const dist = Math.sqrt(x*x + y*y);
    if (dist > max) {
        x = (x / dist) * max;
        y = (y / dist) * max;
    }

    // Move o stick visualmente
    stick.style.left = (x + rect.width / 2 - 30) + "px";
    stick.style.top  = (y + rect.height / 2 - 30) + "px";

    // Normaliza (-1 a 1)
    const normX = x / max;
    const normY = -y / max;

    enviar(normX, normY);
}

function resetStick() {
    stick.style.left = "70px";
    stick.style.top  = "70px";
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
const connectBtn = document.getElementById('connectBtn');
const statusDisplay = document.getElementById('status');

const targetAngleDisplay = document.getElementById('targetAngle');
const currentAngleDisplay = document.getElementById('currentAngle');

const updateButtons = document.querySelectorAll('.updateBtn');
const saveBtn = document.getElementById('saveBtn');

const SERVICE_UUID = "0000ff10-0000-1000-8000-00805f9b34fb";
const CHARACTERISTIC_UUID = "0000ff11-0000-1000-8000-00805f9b34fb";

let bluetoothDevice = null;
let characteristic = null;

async function connect() {
    try {
        connectBtn.disabled = true;
        statusDisplay.textContent = 'Requesting...';

        bluetoothDevice = await navigator.bluetooth.requestDevice({
            filters: [{ services: [SERVICE_UUID] }]
        });
        bluetoothDevice.addEventListener('gattserverdisconnected', onDisconnected);

        statusDisplay.textContent = 'Connecting...';
        const server = await bluetoothDevice.gatt.connect();
        const service = await server.getPrimaryService(SERVICE_UUID);
        characteristic = await service.getCharacteristic(CHARACTERISTIC_UUID);

        await characteristic.startNotifications();
        characteristic.addEventListener('characteristicvaluechanged', handleCharacteristicValueChanged);

        statusDisplay.textContent = 'Connected';
        connectBtn.textContent = 'Disconnect';
        connectBtn.disabled = false;
    } catch (error) {
        console.error('Connection error', error);
        statusDisplay.textContent = 'Error';
        connectBtn.disabled = false;
    }
}

function disconnect() {
    if (bluetoothDevice && bluetoothDevice.gatt.connected) {
        bluetoothDevice.gatt.disconnect();
    }
}

function onDisconnected() {
    characteristic = null;

    statusDisplay.textContent = 'Disconnected';
    connectBtn.textContent = 'Connect';
    connectBtn.disabled = false;

    targetAngleDisplay.textContent = '-';
    currentAngleDisplay.textContent = '-';
    voltageDisplay.textContent = '-';
}

function handleCharacteristicValueChanged(event) {
    const value = event.target.value;
    const decoder = new TextDecoder('utf-8');
    const message = decoder.decode(value);

    const targetMatch = message.match(/T:([\d.-]+)/);
    const currentMatch = message.match(/C:([\d.-]+)/);

    if (targetMatch && targetMatch[1]) {
        targetAngleDisplay.textContent = targetMatch[1];
    }
    if (currentMatch && currentMatch[1]) {
        currentAngleDisplay.textContent = currentMatch[1];
    }
}

async function sendCommand(command) {
    if (!characteristic) {
        alert('Not connected to device');
        return;
    }

    const encoder = new TextEncoder();
    const data = encoder.encode(command);

    try {
        if (characteristic.writeValueWithResponse) {
            await characteristic.writeValueWithResponse(data);
        } else {
            await characteristic.writeValue(data);
        }
    } catch (error) {
        alert('Failed to send command: ' + error);
    }
}

connectBtn.addEventListener('click', () => {
    if (bluetoothDevice && bluetoothDevice.gatt.connected) {
        disconnect();
    } else {
        connect();
    }
});

updateButtons.forEach(btn => {
    btn.addEventListener('click', () => {
        const paramName = btn.getAttribute('data-input');
        const inputElement = document.getElementById(paramName);
        const value = inputElement.value;

        if (value === '' || isNaN(Number(value))) {
            alert('Please enter a valid numeric value');
            return;
        }

        sendCommand(`${paramName}=${value}`);
    });
});

saveBtn.addEventListener('click', () => {
    sendCommand('save');
});

if ('serviceWorker' in navigator) {
    navigator.serviceWorker.register('sw.js')
}

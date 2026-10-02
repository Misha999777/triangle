const connectBtn = document.getElementById('connectBtn');
const statusDisplay = document.getElementById('status');

const targetAngleDisplay = document.getElementById('targetAngle');
const currentAngleDisplay = document.getElementById('currentAngle');

const updateButtons = document.querySelectorAll('.updateBtn');

const SERVICE_UUID = "0000ff10-0000-1000-8000-00805f9b34fb";
const CHARACTERISTIC_UUID = "0000ff11-0000-1000-8000-00805f9b34fb";

let bluetoothDevice = null;
let characteristic = null;
let pollInterval = null;
let isFetching = false;

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

        statusDisplay.textContent = 'Connected';
        connectBtn.textContent = 'Disconnect';
        connectBtn.disabled = false;

        pollInterval = setInterval(fetchStatus, 500);
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
    isFetching = false;

    statusDisplay.textContent = 'Disconnected';
    connectBtn.textContent = 'Connect';
    connectBtn.disabled = false;

    targetAngleDisplay.textContent = '-';
    currentAngleDisplay.textContent = '-';

    if (pollInterval) {
        clearInterval(pollInterval);
        pollInterval = null;
    }
}

async function fetchStatus() {
    if (!characteristic || isFetching) {
        return;
    }
    
    isFetching = true;
    try {
        const value = await characteristic.readValue();
        const decoder = new TextDecoder('utf-8');
        const message = decoder.decode(value);

        const targetMatch = message.match(/Target angle:\s*([\d.-]+)/);
        const currentMatch = message.match(/Current angle:\s*([\d.-]+)/);

        if (targetMatch && targetMatch[1]) {
            targetAngleDisplay.textContent = targetMatch[1];
        }
        if (currentMatch && currentMatch[1]) {
            currentAngleDisplay.textContent = currentMatch[1];
        }
    } catch (error) {
        console.error("Failed to read status", error);
    } finally {
        isFetching = false;
    }
}

async function sendParameterUpdate(index, value) {
    if (!characteristic) {
        alert('Not connected to device');
        return;
    }

    const message = `${index}:${value}`;
    const encoder = new TextEncoder();
    const data = encoder.encode(message);

    try {
        if (characteristic.writeValueWithResponse) {
            await characteristic.writeValueWithResponse(data);
        } else {
            await characteristic.writeValue(data);
        }
    } catch (error) {
        alert('Failed to update parameter: ' + error);
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
        const paramIndex = btn.getAttribute('data-index');
        const inputId = btn.getAttribute('data-input');
        const inputElement = document.getElementById(inputId);
        const value = inputElement.value;

        if (value === '' || isNaN(Number(value))) {
            alert('Please enter a valid numeric value');
            return;
        }

        sendParameterUpdate(paramIndex, value);
    });
});

if ('serviceWorker' in navigator) {
    navigator.serviceWorker.register('sw.js')
}

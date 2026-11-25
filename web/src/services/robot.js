// Must match src/bluetooth/config/ble_config.gatt and src/bluetooth/Protocol.h in the firmware.
const SERVICE_UUID = '0000ff10-0000-1000-8000-00805f9b34fb'
const PARAMETERS_UUID = '0000ff11-0000-1000-8000-00805f9b34fb'
const TELEMETRY_UUID = '0000ff12-0000-1000-8000-00805f9b34fb'
const COMMAND_UUID = '0000ff13-0000-1000-8000-00805f9b34fb'

const COMMAND_SAVE_PARAMETERS = 0x01

const TELEMETRY_VERSION = 1
const HEADER_SIZE = 4
const SAMPLE_SIZE = 13
const FLAG_BALANCING = 0x01

const MAX_RECONNECT_DELAY_MS = 5000

const listeners = {
  samples: new Set(),
  state: new Set(),
  stats: new Set(),
}

let state = 'disconnected'
let device = null
let characteristics = null
let userDisconnected = false
let operationQueue = Promise.resolve()

const telemetry = {
  expectedSequence: null,
  firstTime: null,
  lastRawTime: 0,
  timeOffset: 0,
  received: 0,
  dropped: 0,
}
let statsTimer = 0

function subscribe(kind, listener) {
  listeners[kind].add(listener)
  return () => {
    listeners[kind].delete(listener)
  }
}

function emit(kind, value) {
  listeners[kind].forEach(listener => listener(value))
}

function setState(next) {
  if (state === next) return
  state = next
  emit('state', state)
}

function resetTelemetry() {
  Object.assign(telemetry, { expectedSequence: null, firstTime: null, lastRawTime: 0, timeOffset: 0, received: 0, dropped: 0 })
}

function decodeTelemetry(view) {
  if (view.byteLength < HEADER_SIZE || view.getUint8(0) !== TELEMETRY_VERSION) return []

  const count = view.getUint8(1)
  const sequence = view.getUint16(2, true)
  if (view.byteLength < HEADER_SIZE + count * SAMPLE_SIZE) return []

  if (telemetry.expectedSequence !== null) {
    telemetry.dropped += (sequence - telemetry.expectedSequence) & 0xffff
  }
  telemetry.expectedSequence = (sequence + count) & 0xffff
  telemetry.received += count

  const samples = []
  for (let i = 0; i < count; i++) {
    const offset = HEADER_SIZE + i * SAMPLE_SIZE

    // The firmware sends the low 16 bits of its millisecond clock; unwrap it into a continuous time.
    const rawTime = view.getUint16(offset, true)
    if (rawTime < telemetry.lastRawTime) telemetry.timeOffset += 0x10000
    telemetry.lastRawTime = rawTime
    const timeMs = telemetry.timeOffset + rawTime
    telemetry.firstTime ??= timeMs

    samples.push({
      time: (timeMs - telemetry.firstTime) / 1000,
      angle: view.getInt16(offset + 2, true) / 100,
      angularVelocity: view.getInt16(offset + 4, true) / 10,
      wheelVelocity: view.getInt16(offset + 6, true) / 10,
      torque: view.getInt16(offset + 8, true) / 1000,
      target: view.getInt16(offset + 10, true) / 100,
      balancing: (view.getUint8(offset + 12) & FLAG_BALANCING) !== 0,
    })
  }
  return samples
}

function handleTelemetry(event) {
  const samples = decodeTelemetry(event.target.value)
  if (samples.length) emit('samples', samples)
}

function startStats() {
  clearInterval(statsTimer)
  let lastReceived = telemetry.received
  statsTimer = setInterval(() => {
    emit('stats', { rate: telemetry.received - lastReceived, dropped: telemetry.dropped })
    lastReceived = telemetry.received
  }, 1000)
}

function stopStats() {
  clearInterval(statsTimer)
  statsTimer = 0
  emit('stats', null)
}

async function openLink() {
  const server = await device.gatt.connect()
  const service = await server.getPrimaryService(SERVICE_UUID)
  characteristics = {
    parameters: await service.getCharacteristic(PARAMETERS_UUID),
    telemetry: await service.getCharacteristic(TELEMETRY_UUID),
    command: await service.getCharacteristic(COMMAND_UUID),
  }

  // The firmware restarts the stream on subscription, so sequence numbers start over too.
  telemetry.expectedSequence = null
  characteristics.telemetry.addEventListener('characteristicvaluechanged', handleTelemetry)
  await characteristics.telemetry.startNotifications()
}

async function handleDisconnected() {
  characteristics = null
  if (userDisconnected || !device) {
    stopStats()
    setState('disconnected')
    return
  }

  setState('reconnecting')
  for (let attempt = 1; device && !userDisconnected; attempt++) {
    try {
      await openLink()
      setState('connected')
      return
    } catch {
      await new Promise(resolve => setTimeout(resolve, Math.min(attempt * 1000, MAX_RECONNECT_DELAY_MS)))
    }
  }
}

// Chrome rejects a GATT operation while another one is in progress, so run them one at a time.
function enqueue(operation) {
  const run = async () => {
    if (state !== 'connected' || !characteristics) {
      throw 'Not connected'
    }
    return operation(characteristics)
  }
  const result = operationQueue.then(run, run)
  operationQueue = result.catch(() => {})
  return result
}

/**
 * @param {(samples: {time: number, angle: number, target: number, angularVelocity: number,
 *   wheelVelocity: number, torque: number, balancing: boolean}[]) => void} listener
 */
export const onSamples = listener => subscribe('samples', listener)

/** @param {(state: 'disconnected' | 'connecting' | 'connected' | 'reconnecting') => void} listener */
export const onState = listener => subscribe('state', listener)

/** @param {(stats: {rate: number, dropped: number} | null) => void} listener Called once per second while connected. */
export const onStats = listener => subscribe('stats', listener)

export const getState = () => state

export const deviceName = () => device?.name ?? null

export const isSupported = () => 'bluetooth' in navigator

/** Opens the browser's device picker and connects. Resolves to false if the user closed the picker. */
export async function connect() {
  if (!isSupported()) {
    throw 'This browser does not support Web Bluetooth. Use Chrome or Edge.'
  }

  let picked
  try {
    picked = await navigator.bluetooth.requestDevice({ filters: [{ services: [SERVICE_UUID] }] })
  } catch (e) {
    if (e?.name === 'NotFoundError') return false
    throw e
  }

  device?.removeEventListener('gattserverdisconnected', handleDisconnected)
  device = picked
  userDisconnected = false
  device.addEventListener('gattserverdisconnected', handleDisconnected)

  resetTelemetry()
  setState('connecting')
  try {
    await openLink()
  } catch (e) {
    userDisconnected = true
    device.gatt.disconnect()
    setState('disconnected')
    throw `Connection failed: ${errorMessage(e)}. If the robot's firmware was just updated, turn Bluetooth off and on to clear the cached services.`
  }
  startStats()
  setState('connected')
  return true
}

export async function disconnect() {
  userDisconnected = true
  if (device?.gatt.connected) {
    device.gatt.disconnect()
  }
  characteristics = null
  stopStats()
  setState('disconnected')
}

/** @returns {Promise<{k1: number, k2: number, k3: number, targetAngle: number}>} */
export function readParameters() {
  return enqueue(async ({ parameters }) => {
    const view = await parameters.readValue()
    return {
      k1: view.getFloat32(0, true),
      k2: view.getFloat32(4, true),
      k3: view.getFloat32(8, true),
      targetAngle: view.getFloat32(12, true),
    }
  })
}

/** @param {{k1: number, k2: number, k3: number, targetAngle: number}} values */
export function writeParameters({ k1, k2, k3, targetAngle }) {
  const view = new DataView(new ArrayBuffer(16))
  view.setFloat32(0, k1, true)
  view.setFloat32(4, k2, true)
  view.setFloat32(8, k3, true)
  view.setFloat32(12, targetAngle, true)

  return enqueue(async ({ parameters }) => {
    try {
      await parameters.writeValueWithResponse(view)
    } catch (e) {
      throw `The robot rejected the values (${errorMessage(e)})`
    }
  })
}

export function saveParameters() {
  return enqueue(async ({ command }) => {
    try {
      await command.writeValueWithResponse(new Uint8Array([COMMAND_SAVE_PARAMETERS]))
    } catch {
      // The browser doesn't expose the ATT error code; the only reason the firmware refuses is balancing.
      throw "The robot rejected the save. It can't save while it is balancing. Lay it down and try again."
    }
  })
}

export function errorMessage(error) {
  return typeof error === 'string' ? error : error?.message ?? String(error)
}

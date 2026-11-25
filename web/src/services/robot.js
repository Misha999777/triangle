import { create, fromBinary, toBinary } from '@bufbuild/protobuf'

import { CommandSchema, CommandType, ParametersSchema, TelemetryBatchSchema } from '../gen/triangle_pb.js'

// Must match src/bluetooth/config/ble_config.gatt in the firmware. The payloads are defined in proto/triangle.proto.
const SERVICE_UUID = '0000ff10-0000-1000-8000-00805f9b34fb'
const PARAMETERS_UUID = '0000ff11-0000-1000-8000-00805f9b34fb'
const TELEMETRY_UUID = '0000ff12-0000-1000-8000-00805f9b34fb'
const COMMAND_UUID = '0000ff13-0000-1000-8000-00805f9b34fb'

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
  lastTimeMs: null,
  elapsedMs: 0,
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
  Object.assign(telemetry, { expectedSequence: null, lastTimeMs: null, elapsedMs: 0, received: 0, dropped: 0 })
}

const toBytes = view => new Uint8Array(view.buffer, view.byteOffset, view.byteLength)

function decodeTelemetry(view) {
  let batch
  try {
    batch = fromBinary(TelemetryBatchSchema, toBytes(view))
  } catch {
    return []
  }

  const count = batch.angle.length
  if (telemetry.expectedSequence !== null) {
    telemetry.dropped += Math.max(0, batch.sequence - telemetry.expectedSequence)
  }
  telemetry.expectedSequence = batch.sequence + count
  telemetry.received += count

  // The robot's millisecond clock is 32-bit and wraps after ~49 days, so add up unsigned differences.
  if (telemetry.lastTimeMs !== null) {
    telemetry.elapsedMs += (batch.timeMs - telemetry.lastTimeMs) >>> 0
  }
  telemetry.lastTimeMs = batch.timeMs

  const samples = []
  for (let i = 0; i < count; i++) {
    const deltaMs = i === 0 ? 0 : batch.timeDeltaMs[i] ?? 0
    telemetry.elapsedMs += deltaMs
    telemetry.lastTimeMs = (telemetry.lastTimeMs + deltaMs) >>> 0
    samples.push({
      time: telemetry.elapsedMs / 1000,
      angle: batch.angle[i],
      angularVelocity: batch.angularVelocity[i],
      wheelVelocity: batch.wheelVelocity[i],
      torque: batch.torque[i],
      target: batch.targetAngle[i],
      balancing: batch.balancing[i] ?? false,
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
    const { k1, k2, k3, targetAngle } = fromBinary(ParametersSchema, toBytes(await parameters.readValue()))
    return { k1, k2, k3, targetAngle }
  })
}

/**
 * Sends only the given parameters; the robot keeps its current value for the rest.
 * @param {{k1?: number, k2?: number, k3?: number, targetAngle?: number}} values
 */
export function writeParameters(values) {
  const bytes = toBinary(ParametersSchema, create(ParametersSchema, values))
  return enqueue(async ({ parameters }) => {
    try {
      await parameters.writeValueWithResponse(bytes)
    } catch (e) {
      throw `The robot rejected the values (${errorMessage(e)})`
    }
  })
}

export function saveParameters() {
  const bytes = toBinary(CommandSchema, create(CommandSchema, { type: CommandType.SAVE_PARAMETERS }))
  return enqueue(async ({ command }) => {
    try {
      await command.writeValueWithResponse(bytes)
    } catch {
      // The browser doesn't expose the ATT error code; the only reason the firmware refuses is balancing.
      throw "The robot rejected the save. It can't save while it is balancing. Lay it down and try again."
    }
  })
}

export function errorMessage(error) {
  return typeof error === 'string' ? error : error?.message ?? String(error)
}

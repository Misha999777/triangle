import { create, fromBinary, toBinary } from '@bufbuild/protobuf'

import { CommandSchema, CommandType, ParametersSchema, TelemetryBatchSchema } from '../gen/triangle_pb.js'

// Must match src/bluetooth/config/ble_config.gatt in the firmware.
const SERVICE_UUID = '0000ff10-0000-1000-8000-00805f9b34fb'
const PARAMETERS_UUID = '0000ff11-0000-1000-8000-00805f9b34fb'
const TELEMETRY_UUID = '0000ff12-0000-1000-8000-00805f9b34fb'
const COMMAND_UUID = '0000ff13-0000-1000-8000-00805f9b34fb'

const MAX_RECONNECT_DELAY_MS = 5000

const listeners = { samples: new Set(), state: new Set(), stats: new Set() }
const freshTelemetry = () => ({ expectedSequence: null, lastTimeMs: null, elapsedMs: 0, received: 0, dropped: 0 })

let state = 'disconnected'
let device = null
let characteristics = null
let session = 0
let reconnectingSession = null
let operationQueue = Promise.resolve()
let telemetry = freshTelemetry()
let statsTimer = 0

function subscribe(kind, listener) {
  listeners[kind].add(listener)
  return () => listeners[kind].delete(listener)
}

function emit(kind, value) {
  listeners[kind].forEach(listener => listener(value))
}

function setState(next) {
  if (state === next) return
  state = next
  emit('state', state)
}

const toBytes = view => new Uint8Array(view.buffer, view.byteOffset, view.byteLength)

function handleTelemetry(event) {
  const batch = fromBinary(TelemetryBatchSchema, toBytes(event.target.value))
  const samples = batch.samples.map(sample => {
    if (telemetry.expectedSequence !== null) {
      telemetry.dropped += Math.max(0, sample.sequence - telemetry.expectedSequence)
    }
    telemetry.expectedSequence = sample.sequence + 1

    // The robot's millisecond clock is 32-bit and wraps after ~49 days, so add up unsigned differences.
    if (telemetry.lastTimeMs !== null) {
      telemetry.elapsedMs += (sample.timeMs - telemetry.lastTimeMs) >>> 0
    }
    telemetry.lastTimeMs = sample.timeMs

    return { ...sample, time: telemetry.elapsedMs / 1000 }
  })
  telemetry.received += samples.length
  emit('samples', samples)
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
  emit('stats', null)
}

async function openLink(target) {
  const server = await target.gatt.connect()
  const service = await server.getPrimaryService(SERVICE_UUID)
  characteristics = {
    parameters: await service.getCharacteristic(PARAMETERS_UUID),
    telemetry: await service.getCharacteristic(TELEMETRY_UUID),
    command: await service.getCharacteristic(COMMAND_UUID),
  }

  telemetry.expectedSequence = null
  telemetry.lastTimeMs = null
  characteristics.telemetry.addEventListener('characteristicvaluechanged', handleTelemetry)
  await characteristics.telemetry.startNotifications()
}

async function handleDisconnected() {
  characteristics = null
  if (state === 'disconnected' || reconnectingSession === session) return

  const current = session
  reconnectingSession = current
  const target = device
  setState('reconnecting')
  for (let attempt = 1; session === current; attempt++) {
    try {
      await openLink(target)
      if (session === current) {
        setState('connected')
      } else if (target !== device || state === 'disconnected') {
        target.gatt.disconnect()
      }
      break
    } catch {
      await new Promise(resolve => setTimeout(resolve, Math.min(attempt * 1000, MAX_RECONNECT_DELAY_MS)))
    }
  }
  if (reconnectingSession === current) reconnectingSession = null
}

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

function write(name, bytes, rejected) {
  return enqueue(async current => {
    try {
      await current[name].writeValueWithResponse(bytes)
    } catch (e) {
      throw rejected(e)
    }
  })
}

export const onSamples = listener => subscribe('samples', listener)
export const onState = listener => subscribe('state', listener)
export const onStats = listener => subscribe('stats', listener)
export const getState = () => state
export const deviceName = () => device?.name ?? null
export const isSupported = () => 'bluetooth' in navigator

export async function connect() {
  let picked
  try {
    picked = await navigator.bluetooth.requestDevice({ filters: [{ services: [SERVICE_UUID] }] })
  } catch (e) {
    if (e?.name === 'NotFoundError') return false
    throw e
  }

  device?.removeEventListener('gattserverdisconnected', handleDisconnected)
  device = picked
  device.addEventListener('gattserverdisconnected', handleDisconnected)
  const current = ++session

  telemetry = freshTelemetry()
  setState('connecting')
  try {
    await openLink(picked)
  } catch (e) {
    if (session !== current) return false
    setState('disconnected')
    picked.gatt.disconnect()
    throw `Connection failed: ${errorMessage(e)}.`
  }
  if (session !== current) return false

  startStats()
  setState('connected')
  return true
}

export async function disconnect() {
  session++
  characteristics = null
  stopStats()
  setState('disconnected')
  device?.gatt.disconnect()
}

export function readParameters() {
  return enqueue(async ({ parameters }) => fromBinary(ParametersSchema, toBytes(await parameters.readValue())))
}

export function writeParameters(values) {
  const bytes = toBinary(ParametersSchema, create(ParametersSchema, values))
  return write('parameters', bytes, e => `The robot rejected the values (${errorMessage(e)})`)
}

export function saveParameters() {
  const bytes = toBinary(CommandSchema, create(CommandSchema, { type: CommandType.SAVE_PARAMETERS }))
  // The browser doesn't expose the ATT error code; the only reason the firmware refuses is balancing.
  return write('command', bytes, () => "The robot rejected the save. It can't save while it is balancing. Lay it down and try again.")
}

export function errorMessage(error) {
  return typeof error === 'string' ? error : error?.message ?? String(error)
}

if (import.meta.hot) {
  import.meta.hot.accept(() => location.reload())
}

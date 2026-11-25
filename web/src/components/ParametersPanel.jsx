import { useCallback, useEffect, useState } from 'react'

import {
  Button,
  Field,
  Input,
  MessageBar,
  MessageBarBody,
  Text,
} from '@fluentui/react-components'
import { ArrowClockwiseRegular, CheckmarkRegular, SaveRegular } from '@fluentui/react-icons'

import { errorMessage, onSamples, readParameters, saveParameters, writeParameters } from '../services/robot'

import styles from '../styles/ParametersPanel.module.css'

const PARAMETERS = [
  { name: 'k1', label: 'k1 · Angle gain', hint: 'Stiffness around the balance point' },
  { name: 'k2', label: 'k2 · Angular velocity gain', hint: 'Damping against oscillation' },
  { name: 'k3', label: 'k3 · Wheel speed gain', hint: 'Keeps the wheel below its speed limit' },
  { name: 'targetAngle', label: 'Target angle (°)', hint: 'Balance point; adjusts itself while balancing' },
]

const format = value => String(Number(value.toPrecision(6)))
const toFields = parameters => Object.fromEntries(PARAMETERS.map(({ name }) => [name, format(parameters[name])]))
const isValid = value => value.trim() !== '' && Number.isFinite(Number(value))

function useBalancing() {
  const [balancing, setBalancing] = useState(false)
  useEffect(() => onSamples(samples => setBalancing(samples.at(-1).balancing)), [])
  return balancing
}

export default function ParametersPanel({ linkState }) {
  const [device, setDevice] = useState(null)
  const [values, setValues] = useState(null)
  const [unsaved, setUnsaved] = useState(false)
  const [busy, setBusy] = useState(false)
  const [message, setMessage] = useState(null)
  const balancing = useBalancing()

  const isConnected = linkState === 'connected'

  const run = useCallback(async (action, success) => {
    setBusy(true)
    setMessage(null)
    try {
      await action()
      if (success) setMessage({ intent: 'success', text: success })
    } catch (e) {
      setMessage({ intent: 'error', text: errorMessage(e) })
    } finally {
      setBusy(false)
    }
  }, [])

  const load = useCallback(() => run(async () => {
    const parameters = await readParameters()
    setDevice(parameters)
    setValues(toFields(parameters))
  }), [run])

  useEffect(() => {
    if (isConnected) load()
  }, [isConnected, load])

  useEffect(() => {
    if (linkState === 'connecting') setUnsaved(false)
  }, [linkState])

  const invalid = values ? PARAMETERS.filter(({ name }) => !isValid(values[name])) : []
  const changed = values && device
    ? PARAMETERS.filter(({ name }) => isValid(values[name]) && Number(values[name]) !== Number(format(device[name])))
    : []

  const handleApply = event => {
    event.preventDefault()
    run(async () => {
      // Only the edited fields: the robot adjusts the target angle itself while balancing.
      await writeParameters(Object.fromEntries(changed.map(({ name }) => [name, Number(values[name])])))
      const parameters = await readParameters()
      setDevice(parameters)
      setValues(toFields(parameters))
      setUnsaved(true)
    }, 'Applied. Not saved to flash yet.')
  }

  const handleSave = () => run(async () => {
    await saveParameters()
    setUnsaved(false)
  }, 'Saved to flash.')

  const editable = isConnected && values && !busy

  return (
    <aside className={styles.panel}>
      <div className={styles.header}>
        <Text weight="semibold" size={400}>Parameters</Text>
        <Button
          appearance="subtle"
          size="small"
          icon={<ArrowClockwiseRegular />}
          onClick={load}
          disabled={!isConnected || busy}
          title="Reload the values from the robot"
        >
          Reload
        </Button>
      </div>

      <form className={styles.form} onSubmit={handleApply}>
        {PARAMETERS.map(({ name, label, hint }) => {
          const isInvalid = invalid.some(p => p.name === name)
          return (
            <Field
              key={name}
              label={label}
              hint={hint}
              validationState={isInvalid ? 'error' : 'none'}
              validationMessage={isInvalid ? 'Not a number' : undefined}
            >
              <Input
                value={values?.[name] ?? ''}
                inputMode="decimal"
                onChange={(_, data) => setValues(current => ({ ...current, [name]: data.value }))}
                disabled={!editable}
              />
            </Field>
          )
        })}

        <div className={styles.actions}>
          <Button
            type="submit"
            appearance="primary"
            icon={<CheckmarkRegular />}
            disabled={!editable || changed.length === 0 || invalid.length > 0}
          >
            Apply
          </Button>
          <Button icon={<SaveRegular />} onClick={handleSave} disabled={!isConnected || busy || balancing}>
            Save to flash
          </Button>
        </div>

        {isConnected && balancing && (
          <Text size={200} className={styles.note}>Lay the robot down to save: writing flash would stop the motor.</Text>
        )}
        {isConnected && !balancing && unsaved && (
          <Text size={200} className={styles.note}>Applied values are lost on restart until you save them.</Text>
        )}
      </form>

      {message && (
        <MessageBar intent={message.intent}>
          <MessageBarBody>{message.text}</MessageBarBody>
        </MessageBar>
      )}
    </aside>
  )
}

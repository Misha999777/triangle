import { useEffect, useReducer, useRef, useState } from 'react'

import { Badge, Text, ToggleButton } from '@fluentui/react-components'
import uPlot from 'uplot'

import { onSamples, onStats } from '../services/robot'

import styles from '../styles/TelemetryPanel.module.css'

const WINDOW_SECONDS = 30
const READOUT_INTERVAL_MS = 100
const Y_AXIS_WIDTH = 56
const DEFAULT_SELECTION = ['angle', 'targetAngle']

// Categorical palette slots validated for colour-blind separation on the light and dark surfaces.
const COLORS = {
  light: ['#2a78d6', '#eb6834', '#1baf7a'],
  dark: ['#3987e5', '#d95926', '#199e70'],
}

const SIGNALS = [
  { key: 'angle', label: 'Angle', unit: '°', digits: 2, chart: 'angle', slot: 1 },
  { key: 'targetAngle', label: 'Target', unit: '°', digits: 2, chart: 'angle', slot: 2, dashed: true },
  { key: 'error', label: 'Error', unit: '°', digits: 2, chart: 'angle', slot: 3 },
  { key: 'angularVelocity', label: 'Gyro', unit: '°/s', digits: 1, chart: 'velocity', slot: 1 },
  { key: 'wheelVelocity', label: 'Wheel', unit: '°/s', digits: 0, chart: 'velocity', slot: 2 },
  { key: 'torque', label: 'Torque', unit: ' V', digits: 2, chart: 'torque', slot: 1 },
]

const CHARTS = [
  { id: 'angle', title: 'Angle (°)' },
  { id: 'velocity', title: 'Angular velocity (°/s)' },
  { id: 'torque', title: 'Torque (V)' },
]

const emptyData = () => Object.fromEntries(['time', 'balancing', ...SIGNALS.map(s => s.key)].map(key => [key, []]))

function append(data, samples) {
  for (const sample of samples) {
    const values = { ...sample, error: sample.angle - sample.targetAngle, wheelVelocity: sample.wheelVelocity * 180 / Math.PI }
    for (const key of Object.keys(data)) data[key].push(values[key])
  }

  const firstKept = data.time.findIndex(t => t >= data.time.at(-1) - WINDOW_SECONDS)
  if (firstKept > 0) {
    for (const values of Object.values(data)) values.splice(0, firstKept)
  }
}

export default function TelemetryPanel({ theme, isDark, linkState }) {
  const elementsRef = useRef({})
  const plotsRef = useRef([])
  const dataRef = useRef(emptyData())
  const frameRef = useRef(0)
  const readoutTimeRef = useRef(0)
  const [, refresh] = useReducer(n => n + 1, 0)
  const [stats, setStats] = useState(null)
  const [selection, setSelection] = useState(DEFAULT_SELECTION)

  const colors = isDark ? COLORS.dark : COLORS.light
  const charts = CHARTS
    .map(chart => ({ ...chart, series: SIGNALS.filter(s => s.chart === chart.id && selection.includes(s.key)) }))
    .filter(chart => chart.series.length > 0)

  useEffect(() => onStats(setStats), [])

  useEffect(() => {
    const font = `12px ${theme.fontFamilyBase}`
    const axis = {
      stroke: theme.colorNeutralForeground2,
      font,
      labelFont: font,
      grid: { stroke: theme.colorNeutralStroke2, width: 1 },
      ticks: { stroke: theme.colorNeutralStroke2, width: 1 },
    }

    const plots = charts.map(({ id, series }, i) => {
      const element = elementsRef.current[id]
      const size = () => ({ width: element.clientWidth, height: element.clientHeight })
      const toData = data => [data.time, ...series.map(s => data[s.key])]
      const plot = new uPlot({
        ...size(),
        scales: { x: { time: false } },
        axes: [
          i === charts.length - 1
            ? { ...axis, label: 'Time (s)' }
            : { ...axis, size: 8, values: (_, splits) => splits.map(() => '') },
          { ...axis, size: Y_AXIS_WIDTH },
        ],
        series: [
          {},
          ...series.map(signal => ({
            label: signal.label,
            stroke: colors[signal.slot - 1],
            width: 2,
            dash: signal.dashed ? [6, 4] : undefined,
            points: { show: false },
          })),
        ],
        legend: { show: false },
        cursor: { y: false, drag: { x: false, y: false }, sync: { key: 'telemetry' } },
      }, toData(dataRef.current), element)
      const observer = new ResizeObserver(() => plot.setSize(size()))
      observer.observe(element)
      return { plot, observer, toData }
    })
    plotsRef.current = plots

    return () => {
      for (const { plot, observer } of plots) {
        observer.disconnect()
        plot.destroy()
      }
      plotsRef.current = []
    }
  }, [theme, selection])

  useEffect(() => {
    if (linkState === 'connecting') {
      dataRef.current = emptyData()
      for (const { plot, toData } of plotsRef.current) plot.setData(toData(dataRef.current))
      refresh()
    }
  }, [linkState])

  useEffect(() => {
    const unsubscribe = onSamples(samples => {
      append(dataRef.current, samples)
      if (frameRef.current) return

      frameRef.current = requestAnimationFrame(() => {
        frameRef.current = 0
        for (const { plot, toData } of plotsRef.current) plot.setData(toData(dataRef.current))
        const now = performance.now()
        if (now - readoutTimeRef.current >= READOUT_INTERVAL_MS) {
          readoutTimeRef.current = now
          refresh()
        }
      })
    })

    return () => {
      unsubscribe()
      cancelAnimationFrame(frameRef.current)
      frameRef.current = 0
    }
  }, [])

  const toggle = key => setSelection(current =>
    current.includes(key) ? current.filter(k => k !== key) : [...current, key])

  const data = dataRef.current
  const placeholder = linkState === 'disconnected'
    ? 'Connect to the robot to see live data'
    : data.time.length === 0 ? 'Waiting for data…'
    : charts.length === 0 ? 'Select a value above to plot it' : null

  return (
    <section className={styles.panel}>
      <div className={styles.status}>
        {data.balancing.at(-1) && <Badge appearance="filled" color="success">Balancing</Badge>}
        {stats && <Text size={200} className={styles.rate}>{stats.rate} Hz · {stats.dropped} dropped</Text>}
      </div>

      <div className={styles.readouts}>
        {SIGNALS.map(signal => {
          const plotted = selection.includes(signal.key)
          const value = data[signal.key].at(-1)
          return (
            <ToggleButton
              key={signal.key}
              appearance="subtle"
              checked={plotted}
              className={`${styles.readout} ${plotted ? styles.plotted : ''}`}
              title={plotted ? 'Hide from the chart' : 'Show on the chart'}
              onClick={() => toggle(signal.key)}
            >
              <span className={styles.readoutContent}>
                <Text size={200} className={styles.label}>
                  <span
                    className={`${styles.key} ${signal.dashed ? styles.dashed : ''}`}
                    style={{ '--key-color': colors[signal.slot - 1] }}
                  />
                  {signal.label}
                </Text>
                <Text size={500} weight="semibold" className={styles.value}>
                  {value == null ? '—' : `${value.toFixed(signal.digits)}${signal.unit}`}
                </Text>
              </span>
            </ToggleButton>
          )
        })}
      </div>

      <div className={styles.charts} style={{ '--chart-count': Math.max(charts.length, 1) }}>
        {charts.map(chart => (
          <div key={chart.id} className={styles.chartBlock}>
            <Text size={200} className={styles.chartTitle}>{chart.title}</Text>
            <div ref={element => { elementsRef.current[chart.id] = element }} className={styles.plot} />
          </div>
        ))}
        {placeholder && <div className={styles.placeholder}><Text>{placeholder}</Text></div>}
      </div>
    </section>
  )
}

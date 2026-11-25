import { useEffect, useRef, useState } from 'react'

import { Badge, Text } from '@fluentui/react-components'
import uPlot from 'uplot'

import { onSamples, onStats } from '../services/robot'

import styles from '../styles/TelemetryPanel.module.css'

const WINDOW_SECONDS = 30
const READOUT_INTERVAL_MS = 100

const emptyData = () => ({ time: [], target: [], angle: [] })

function append(data, samples) {
  for (const { time, target, angle } of samples) {
    data.time.push(time)
    data.target.push(target)
    data.angle.push(angle)
  }

  const cutoff = data.time.at(-1) - WINDOW_SECONDS
  const firstKept = data.time.findIndex(t => t >= cutoff)
  if (firstKept > 0) {
    data.time.splice(0, firstKept)
    data.target.splice(0, firstKept)
    data.angle.splice(0, firstKept)
  }
}

const toPlotData = data => [data.time, data.target, data.angle]

const sizeOf = element => ({ width: element.clientWidth, height: element.clientHeight })

function chartOptions(theme, element) {
  const font = `12px ${theme.fontFamilyBase}`
  const axis = {
    stroke: theme.colorNeutralForeground2,
    font,
    labelFont: font,
    grid: { stroke: theme.colorNeutralStroke2, width: 1 },
    ticks: { stroke: theme.colorNeutralStroke2, width: 1 },
  }

  return {
    ...sizeOf(element),
    scales: { x: { time: false } },
    axes: [
      { ...axis, label: 'Time (s)' },
      { ...axis, label: 'Angle (°)' },
    ],
    series: [
      {},
      { label: 'Target', stroke: theme.colorPaletteDarkOrangeForeground1, width: 2, dash: [6, 4] },
      { label: 'Angle', stroke: theme.colorBrandForeground1, width: 2 },
    ],
    legend: { show: false },
    cursor: { drag: { x: false, y: false } },
  }
}

function Stat({ label, value, unit = '°', digits = 2, color }) {
  return (
    <div className={styles.stat}>
      <Text size={200} className={styles.statLabel}>
        {color && <span className={styles.swatch} style={{ background: color }} />}
        {label}
      </Text>
      <Text size={600} weight="semibold" className={styles.statValue}>
        {value == null ? '—' : `${value.toFixed(digits)}${unit}`}
      </Text>
    </div>
  )
}

export default function TelemetryPanel({ theme, linkState }) {
  const chartRef = useRef(null)
  const plotRef = useRef(null)
  const dataRef = useRef(emptyData())
  const frameRef = useRef(0)
  const latestSampleRef = useRef(null)
  const readoutTimeRef = useRef(0)
  const [latest, setLatest] = useState(null)
  const [stats, setStats] = useState(null)

  useEffect(() => onStats(setStats), [])

  useEffect(() => {
    const element = chartRef.current
    const plot = new uPlot(chartOptions(theme, element), toPlotData(dataRef.current), element)
    plotRef.current = plot

    const observer = new ResizeObserver(() => plot.setSize(sizeOf(element)))
    observer.observe(element)

    return () => {
      observer.disconnect()
      plot.destroy()
      plotRef.current = null
    }
  }, [theme])

  useEffect(() => {
    if (linkState === 'connecting') {
      dataRef.current = emptyData()
      latestSampleRef.current = null
      plotRef.current?.setData(toPlotData(dataRef.current))
      setLatest(null)
    }
  }, [linkState])

  useEffect(() => {
    const unsubscribe = onSamples(samples => {
      append(dataRef.current, samples)
      latestSampleRef.current = samples.at(-1)
      if (frameRef.current) return

      frameRef.current = requestAnimationFrame(() => {
        frameRef.current = 0
        plotRef.current?.setData(toPlotData(dataRef.current))
        const now = performance.now()
        if (now - readoutTimeRef.current >= READOUT_INTERVAL_MS) {
          readoutTimeRef.current = now
          setLatest(latestSampleRef.current)
        }
      })
    })

    return () => {
      unsubscribe()
      cancelAnimationFrame(frameRef.current)
      frameRef.current = 0
    }
  }, [])

  const placeholder = linkState === 'disconnected'
    ? 'Connect to the robot to see live data'
    : !latest ? 'Waiting for data…' : null

  return (
    <section className={styles.panel}>
      <div className={styles.header}>
        <div className={styles.stats}>
          <Stat label="Angle" value={latest?.angle} color={theme.colorBrandForeground1} />
          <Stat label="Target" value={latest?.target} color={theme.colorPaletteDarkOrangeForeground1} />
          <Stat label="Error" value={latest ? latest.angle - latest.target : null} />
          <Stat label="Torque" value={latest?.torque} unit=" V" />
          <Stat label="Wheel" value={latest?.wheelVelocity} unit=" rad/s" digits={1} />
        </div>
        <div className={styles.status}>
          {latest?.balancing && <Badge appearance="filled" color="success">Balancing</Badge>}
          {stats && (
            <Text size={200} className={styles.rate}>
              {stats.rate} Hz · {stats.dropped} dropped
            </Text>
          )}
        </div>
      </div>

      <div className={styles.chartArea}>
        <div ref={chartRef} className={styles.chart} />
        {placeholder && <div className={styles.placeholder}><Text>{placeholder}</Text></div>}
      </div>
    </section>
  )
}

import { FluentProvider } from '@fluentui/react-components'

import useSystemTheme from './hooks/useSystemTheme'
import useLinkState from './hooks/useLinkState'

import ConnectionBar from './components/ConnectionBar'
import TelemetryPanel from './components/TelemetryPanel'
import ParametersPanel from './components/ParametersPanel'

import styles from './styles/App.module.css'

export default function App() {
  const { theme, isDark } = useSystemTheme()
  const linkState = useLinkState()

  return (
    <FluentProvider theme={theme} className={styles.root}>
      <ConnectionBar linkState={linkState} />
      <main className={styles.content}>
        <TelemetryPanel theme={theme} isDark={isDark} linkState={linkState} />
        <ParametersPanel linkState={linkState} />
      </main>
    </FluentProvider>
  )
}

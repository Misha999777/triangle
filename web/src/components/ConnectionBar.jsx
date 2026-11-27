import {
  Badge,
  Button,
  MessageBar,
  MessageBarBody,
  Spinner,
  Text,
} from '@fluentui/react-components'
import { PlugConnectedRegular, PlugDisconnectedRegular } from '@fluentui/react-icons'

import useAction from '../hooks/useAction'
import { connect, deviceName, disconnect, isSupported } from '../services/robot'

import styles from '../styles/ConnectionBar.module.css'

const STATE_BADGES = {
  disconnected: { color: 'subtle', label: 'Disconnected' },
  connecting: { color: 'warning', label: 'Connecting…' },
  connected: { color: 'success', label: 'Connected' },
  reconnecting: { color: 'warning', label: 'Reconnecting…' },
}

export default function ConnectionBar({ linkState }) {
  const { busy, message, run } = useAction()

  const supported = isSupported()
  const isLinked = linkState !== 'disconnected'
  const badge = STATE_BADGES[linkState] ?? STATE_BADGES.disconnected

  return (
    <>
      <header className={styles.bar}>
        <Text weight="semibold" size={500} className={styles.title}>Triangle</Text>

        {isLinked ? (
          <>
            <Text className={styles.device}>{deviceName() ?? 'Unknown device'}</Text>
            <Button icon={<PlugDisconnectedRegular />} onClick={() => run(disconnect)} disabled={busy && linkState !== 'connecting'}>
              Disconnect
            </Button>
          </>
        ) : (
          <Button appearance="primary" icon={<PlugConnectedRegular />} onClick={() => run(connect)} disabled={busy || !supported}>
            Connect…
          </Button>
        )}

        <div className={styles.status}>
          {(busy || linkState === 'connecting' || linkState === 'reconnecting') && <Spinner size="tiny" />}
          <Badge appearance="tint" color={badge.color}>{badge.label}</Badge>
        </div>
      </header>

      {!supported && (
        <MessageBar intent="warning" className={styles.error}>
          <MessageBarBody>
            This browser doesn't support Web Bluetooth. Open this page in Chrome or Edge on desktop or Android.
          </MessageBarBody>
        </MessageBar>
      )}

      {message && (
        <MessageBar intent={message.intent} className={styles.error}>
          <MessageBarBody>{message.text}</MessageBarBody>
        </MessageBar>
      )}
    </>
  )
}

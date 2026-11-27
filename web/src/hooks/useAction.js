import { useCallback, useState } from 'react'

import { errorMessage } from '../services/robot'

export default function useAction() {
  const [busy, setBusy] = useState(false)
  const [message, setMessage] = useState(null)

  const run = useCallback(async (action, success) => {
    setBusy(true)
    setMessage(null)
    try {
      await action()
      if (success) {
        setMessage({ intent: 'success', text: success })
      }
    } catch (e) {
      setMessage({ intent: 'error', text: errorMessage(e) })
    } finally {
      setBusy(false)
    }
  }, [])

  return { busy, message, run }
}

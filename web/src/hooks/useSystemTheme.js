import { useState, useEffect } from 'react'

import { webLightTheme, webDarkTheme } from '@fluentui/react-components'

const query = '(prefers-color-scheme: dark)'

export default function useSystemTheme() {
  const [theme, setTheme] = useState(() => window.matchMedia(query).matches ? webDarkTheme : webLightTheme)

  useEffect(() => {
    const mq = window.matchMedia(query)
    const handleChange = e => setTheme(e.matches ? webDarkTheme : webLightTheme)
    mq.addEventListener('change', handleChange)

    return () => mq.removeEventListener('change', handleChange)
  }, [])

  return theme
}

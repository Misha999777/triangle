import { useState, useEffect } from 'react'

import { webLightTheme, webDarkTheme } from '@fluentui/react-components'

const query = '(prefers-color-scheme: dark)'

export default function useSystemTheme() {
  const [isDark, setIsDark] = useState(() => window.matchMedia(query).matches)

  useEffect(() => {
    const mq = window.matchMedia(query)
    const handleChange = e => setIsDark(e.matches)
    mq.addEventListener('change', handleChange)

    return () => mq.removeEventListener('change', handleChange)
  }, [])

  return { theme: isDark ? webDarkTheme : webLightTheme, isDark }
}

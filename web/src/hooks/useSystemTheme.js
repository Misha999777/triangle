import { useState, useEffect } from 'react'

import { webLightTheme, webDarkTheme } from '@fluentui/react-components'

const darkQuery = window.matchMedia('(prefers-color-scheme: dark)')

export default function useSystemTheme() {
  const [isDark, setIsDark] = useState(darkQuery.matches)

  useEffect(() => {
    const handleChange = e => setIsDark(e.matches)
    darkQuery.addEventListener('change', handleChange)

    return () => darkQuery.removeEventListener('change', handleChange)
  }, [])

  return { theme: isDark ? webDarkTheme : webLightTheme, isDark }
}

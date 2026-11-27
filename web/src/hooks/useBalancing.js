import { useState, useEffect } from 'react'

import { onSamples } from '../services/robot'

export default function useBalancing() {
  const [balancing, setBalancing] = useState(false)

  useEffect(() => onSamples(samples => setBalancing(samples.at(-1).balancing)), [])

  return balancing
}

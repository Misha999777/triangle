import { useState, useEffect } from 'react'

import { getState, onState } from '../services/robot'

export default function useLinkState() {
  const [state, setState] = useState(getState)

  useEffect(() => onState(setState), [])

  return state
}

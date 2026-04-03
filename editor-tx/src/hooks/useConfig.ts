import { useContext } from 'react'
import ConfigContext from '@/contexts/ConfigContext'
import type { ConfigContextValue } from '@/types/midi'

export function useConfig(): ConfigContextValue {
  const ctx = useContext(ConfigContext)
  if (!ctx) throw new Error('useConfig must be used within ConfigProvider')
  return ctx
}

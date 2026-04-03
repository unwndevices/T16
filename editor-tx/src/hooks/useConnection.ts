import { useContext } from 'react'
import ConnectionContext from '@/contexts/ConnectionContext'
import type { ConnectionContextValue } from '@/types/midi'

export function useConnection(): ConnectionContextValue {
  const ctx = useContext(ConnectionContext)
  if (!ctx) throw new Error('useConnection must be used within ConnectionProvider')
  return ctx
}

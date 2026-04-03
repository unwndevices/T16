import '@fontsource/inter/400.css'
import '@fontsource/inter/600.css'
import '@fontsource/poppins/600.css'
import './design-system/reset.css'
import './design-system/tokens.css'
import './design-system/globals.css'

import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { ToastProvider } from '@/contexts/ToastContext'
import { ConnectionProvider } from '@/contexts/ConnectionContext'
import { ConfigProvider } from '@/contexts/ConfigContext'
import App from './App'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <ToastProvider>
      <ConnectionProvider>
        <ConfigProvider>
          <App />
        </ConfigProvider>
      </ConnectionProvider>
    </ToastProvider>
  </StrictMode>
)

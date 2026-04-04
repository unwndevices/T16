import { createBrowserRouter, RouterProvider } from 'react-router'
import { Layout } from '@/pages/Layout'
import { Dashboard } from '@/pages/Dashboard'
import { Upload } from '@/pages/Upload'
import { Monitor } from '@/pages/Monitor'
import { Manual } from '@/pages/Manual'

const router = createBrowserRouter([
  {
    element: <Layout />,
    children: [
      { index: true, element: <Dashboard /> },
      { path: 'upload', element: <Upload /> },
      { path: 'monitor', element: <Monitor /> },
      { path: 'manual', element: <Manual /> },
    ],
  },
])

export default function App() {
  return <RouterProvider router={router} />
}

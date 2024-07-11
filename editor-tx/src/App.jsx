import {
    createBrowserRouter,
    createRoutesFromElements,
    Route,
    RouterProvider,
} from 'react-router-dom'

// layouts and pages
import RootLayout from './layouts/RootLayout'
import Dashboard from './pages/Dashboard'
import QuickStart from './pages/QuickStart'
import Upload from './pages/Upload'

// router and routes
const router = createBrowserRouter(
    createRoutesFromElements(
        <Route path="/" element={<RootLayout />}>
            <Route index element={<Dashboard />} />
            <Route path="upload" element={<Upload />} />
            <Route path="manual" element={<QuickStart />} />
        </Route>
    )
)

function App() {
    return <RouterProvider router={router} />
}

export default App

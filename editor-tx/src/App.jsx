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
// router and routes
const router = createBrowserRouter(
    createRoutesFromElements(
        <Route path="/" element={<RootLayout />}>
            <Route index element={<Dashboard />} />
            <Route path="quickstart" element={<QuickStart />} />
        </Route>
    )
)

function App() {
    return <RouterProvider router={router} />
}

export default App

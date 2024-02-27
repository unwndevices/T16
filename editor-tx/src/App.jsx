import {
    createBrowserRouter,
    createRoutesFromElements,
    Route,
    RouterProvider,
} from 'react-router-dom'

// layouts and pages
import RootLayout from './layouts/RootLayout'
import Dashboard from './pages/Dashboard'
import General from './pages/General'
import ControlChange from './pages/ControlChange'

// router and routes
const router = createBrowserRouter(
    createRoutesFromElements(
        <Route path="/" element={<RootLayout />}>
            <Route index element={<Dashboard />} />
            <Route path="general" element={<General />} />
            <Route path="cc" element={<ControlChange />} />
        </Route>
    )
)

function App() {
    return <RouterProvider router={router} />
}

export default App

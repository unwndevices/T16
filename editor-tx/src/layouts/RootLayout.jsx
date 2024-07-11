import { Outlet } from 'react-router-dom'
import NavBar from '../components/NavBar'

const RootLayout = () => {
    return (
        <>
            <NavBar />
            <Outlet />
        </>
    )
}

export default RootLayout

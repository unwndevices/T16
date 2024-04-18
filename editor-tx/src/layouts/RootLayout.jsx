import { Outlet } from 'react-router-dom'
import { Grid, GridItem } from '@chakra-ui/react'
// components
import NavBar from '../components/NavBar'
import Footer from '../components/Footer'

export default function RootLayout() {
    return (
        <Grid
            templateColumns="repeat(6, 1fr)"
            backgroundColor="background.main"
            height="100vh"
        >
            <GridItem as="main" colSpan={6}>
                <NavBar />
                <Outlet />
            </GridItem>

            <Footer />
        </Grid>
    )
}

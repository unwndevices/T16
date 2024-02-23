import { Outlet } from 'react-router-dom'
import { Grid, GridItem } from '@chakra-ui/react'
// components
import NavBar from '../components/NavBar'
import SideBar from '../components/SideBar'

export default function RootLayout() {
    return (
        <Grid templateColumns="repeat(6, 1fr)" bg="gray.100">
            <GridItem
                as="aside"
                colSpan={{ base: 6, md: 1 }}
                bg="gray.600"
                minH={{ md: '100vh' }}
                p={4}
            >
                <SideBar />
            </GridItem>
            <GridItem as="main" colSpan={{ base: 6, md: 5 }}>
                <NavBar />
                <Outlet />
            </GridItem>
        </Grid>
    )
}

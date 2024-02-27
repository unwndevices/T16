import { Heading, List, ListIcon, ListItem, VStack } from '@chakra-ui/react'
import { NavLink } from 'react-router-dom'
import { MdDashboard, MdPiano } from 'react-icons/md'
import { IoMdGrid } from 'react-icons/io'

export default function SideBar() {
    return (
        <VStack align="stretch">
            <Heading fontWeight={500} fontStyle="italic" color="white">
                Topo ctrl
            </Heading>
            <List fontSize="1.6em" fontWeight={300} color="white">
                <ListItem>
                    <NavLink to="/">
                        <ListIcon as={MdDashboard} /> Dashboard
                    </NavLink>
                </ListItem>
                <ListItem>
                    <NavLink to="/general">
                        <ListIcon as={MdPiano} /> General
                    </NavLink>
                </ListItem>
                <ListItem>
                    <NavLink to="/cc">
                        <ListIcon as={IoMdGrid} /> Control Change
                    </NavLink>
                </ListItem>
            </List>
        </VStack>
    )
}

import { List, ListIcon, ListItem } from '@chakra-ui/react'
import { NavLink } from 'react-router-dom'
import { MdDashboard, MdPiano } from 'react-icons/md'
import { IoMdGrid } from 'react-icons/io'

export default function SideBar() {
    return (
        <List fontSize="1.6em" fontWeight={300} color="white">
            <ListItem>
                <NavLink to="/">
                    <ListIcon as={MdDashboard} /> Dashboard
                </NavLink>
            </ListItem>
            <ListItem>
                <NavLink to="/keyboard">
                    <ListIcon as={MdPiano} /> Keyboard
                </NavLink>
            </ListItem>
            <ListItem>
                <NavLink to="/cc">
                    <ListIcon as={IoMdGrid} /> Control Change
                </NavLink>
            </ListItem>
        </List>
    )
}

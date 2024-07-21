import {
    Heading,
    Button,
    Flex,
    Spacer,
    Box,
    HStack,
    IconButton,
    Drawer,
    DrawerOverlay,
    DrawerContent,
    DrawerCloseButton,
    DrawerBody,
    DrawerFooter,
    useDisclosure,
    DrawerHeader,
    Switch,
    Text,
} from '@chakra-ui/react'
import MidiContext from './MidiProvider'
import React, { useContext } from 'react'
import {
    MdOutlineMenuBook,
    MdOutlineDisplaySettings,
    MdOutlineSystemUpdateAlt,
} from 'react-icons/md'
import Settings from '../pages/Settings'
import { Link } from 'react-router-dom'
import { FaEdit, FaUpload } from 'react-icons/fa'
import { useLocation } from 'react-router-dom'

export default function NavBar() {
    const { isConnected, connect, sendConfig, isDemo, setDemo } =
        useContext(MidiContext)
    const { isOpen, onOpen, onClose } = useDisclosure()
    const location = useLocation()
    const btnRef = React.useRef()

    // Function to check if current route is the editor page
    const isEditorPage = () => location.pathname === '/'

    // Function to get the appropriate title based on the current route
    const getPageTitle = () => {
        switch (location.pathname) {
            case '/':
                return 'Topo Editor'
            case '/upload':
                return 'Firmware Update'
            case '/manual':
                return 'Manual'
            default:
                return 'Topo Editor'
        }
    }

    // Function to determine if a route is active
    const isActive = (path) => location.pathname === path

    return (
        <Flex
            as="nav"
            p={2}
            px={6}
            alignItems="center"
            position="sticky"
            top={0}
            zIndex={100}
            bg="#ffffffcf"
        >
            <Flex maxW="container.lg" mx="auto" alignItems="center" mr={4}>
                <IconButton
                    as={Link}
                    to="/"
                    icon={<MdOutlineDisplaySettings />}
                    aria-label="Editor"
                    mr={2}
                    bg={isActive('/') ? 'green.600' : 'transparent'}
                    color={isActive('/') ? 'white' : 'gray.600'}
                    size="lg"
                />
                <IconButton
                    as={Link}
                    to="/upload"
                    icon={<MdOutlineSystemUpdateAlt />}
                    aria-label="Upload"
                    mr={2}
                    bg={isActive('/upload') ? 'green.600' : 'transparent'}
                    color={isActive('/upload') ? 'white' : 'gray.600'}
                    size="lg"
                />
                <IconButton
                    as={Link}
                    to="/manual"
                    icon={<MdOutlineMenuBook />}
                    aria-label="Manual"
                    bg={isActive('/manual') ? 'green.600' : 'transparent'}
                    color={isActive('/manual') ? 'white' : 'gray.600'}
                    size="lg"
                />
                <Spacer />
            </Flex>
            <HStack spacing={4}>
                <Heading as="h1" size="lg" letterSpacing={1}>
                    {getPageTitle()}
                </Heading>
            </HStack>
            <Spacer />
            {isEditorPage() && (
                <HStack spacing={4}>
                    <HStack>
                        <Text>Demo Mode</Text>
                        <Switch
                            id="demo-mode"
                            isChecked={isDemo}
                            onChange={(e) => setDemo(e.target.checked)}
                        />
                    </HStack>

                    <Box>{isConnected ? '🟢' : '🔴'}</Box>
                    <Button
                        colorScheme="secondary"
                        onClick={isConnected ? sendConfig : connect}
                    >
                        {isConnected ? 'Save' : 'Connect'}
                    </Button>
                </HStack>
            )}
        </Flex>
    )
}

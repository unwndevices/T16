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
import { MdOutlineMenuBook, MdSettings } from 'react-icons/md'
import Settings from '../pages/Settings'

export default function NavBar() {
    const { isConnected, connect, sendConfig, isDemo, setDemo } =
        useContext(MidiContext)
    const { isOpen, onOpen, onClose } = useDisclosure()

    const btnRef = React.useRef()
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
            <HStack spacing={4}>
                <Heading as="h1" size="lg" letterSpacing={1}>
                    Topo Editor
                </Heading>
                <Button
                    as="a"
                    href="/quickstart"
                    rightIcon={<MdOutlineMenuBook />}
                    colorScheme="primary"
                    variant="outline"
                    target="_blank"
                >
                    Guide
                </Button>
            </HStack>
            <Spacer />
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

                <IconButton
                    onClick={onOpen}
                    aria-label="Settings"
                    colorScheme="secondary"
                    icon={<MdSettings />}
                />
            </HStack>
            <Drawer
                isOpen={isOpen}
                placement="right"
                onClose={onClose}
                finalFocusRef={btnRef}
                size="md"
            >
                <DrawerOverlay />
                <DrawerContent>
                    <DrawerCloseButton size="lg" right="25px" />
                    <DrawerHeader>Settings</DrawerHeader>
                    <DrawerBody>
                        <Settings />
                    </DrawerBody>
                    <DrawerFooter />
                </DrawerContent>
            </Drawer>
        </Flex>
    )
}

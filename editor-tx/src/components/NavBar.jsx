import { Heading, Button, Flex, Spacer, Box, HStack } from '@chakra-ui/react'
import MidiContext from './MidiProvider'
import { useContext } from 'react'

export default function NavBar() {
    const { isConnected, connect, sendConfig } = useContext(MidiContext)
    return (
        <Flex as="nav" p={4} pb={1} alignItems="center">
            <Heading as="h1" size="lg" letterSpacing={'tighter'}>
                Editor TX
            </Heading>
            <Spacer />
            <HStack spacing={4}>
                <Box>{isConnected ? '🟢' : '🔴'}</Box>
                <Button
                    colorScheme="green"
                    onClick={isConnected ? sendConfig : connect}
                >
                    {isConnected ? 'Save' : 'Connect'}
                </Button>
            </HStack>
        </Flex>
    )
}

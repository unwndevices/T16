import { Box, HStack, IconButton, Text } from '@chakra-ui/react'
import BankSelector from './BankSelector'
import MidiContext from './MidiProvider'
import { useContext } from 'react'
import { MdDownload, MdUpload } from 'react-icons/md'
import MidiMonitor from './MidiMonitor'

export default function Footer() {
    const { isConnected, downloadConfig, uploadConfig } =
        useContext(MidiContext)
    return (
        <HStack
            as="footer"
            alignItems="center"
            w="full"
            bg="#ffffffcf"
            justifyContent="space-between"
            p={2}
            px={6}
            position="fixed"
            bottom={0}
            spacing={6}
        >
            <BankSelector />
            <MidiMonitor />
            <Box display="flex" justifyContent="center" alignItems="center">
                <Text fontWeight="bold" mr={1}>
                    Download
                </Text>
                <IconButton
                    isRound={true}
                    onClick={downloadConfig}
                    aria-label="Download Config"
                    colorScheme="primary"
                    icon={<MdDownload />}
                    isDisabled={!isConnected}
                    size="sm"
                    mx={1}
                />
                <Text fontWeight="bold" ml={4} mr={1}>
                    Upload
                </Text>
                <IconButton
                    isRound={true}
                    onClick={uploadConfig}
                    aria-label="Upload Config"
                    colorScheme="primary"
                    icon={<MdUpload />}
                    isDisabled={!isConnected}
                    size="sm"
                    mx={1}
                />
            </Box>
        </HStack>
    )
}

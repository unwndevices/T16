import { useState, useRef, useCallback, useEffect } from 'react'
import {
    Box,
    Button,
    VStack,
    Heading,
    useToast,
    Progress,
    Select,
    Text,
    UnorderedList,
    ListItem,
    Alert,
    AlertIcon,
    AlertDescription,
} from '@chakra-ui/react'
import { ESPLoader } from 'esptool-js'
import releaseNotes from '../assets/firmwares/release_notes.json'
import PropTypes from 'prop-types'

const ReleaseNotesCard = ({ selectedFirmware }) => {
    if (!selectedFirmware) return null

    return (
        <Box borderWidth="1px" borderRadius="lg" p={4} mt={4} w="50vw">
            {selectedFirmware.highlights && (
                <Box mb={2}>
                    <Text>{selectedFirmware.highlights}</Text>
                </Box>
            )}
            {selectedFirmware.bugfixes &&
                selectedFirmware.bugfixes.length > 0 && (
                    <Box mb={2}>
                        <Heading size="sm">Bugfixes</Heading>
                        <UnorderedList>
                            {selectedFirmware.bugfixes.map((fix, index) => (
                                <ListItem key={index}>{fix}</ListItem>
                            ))}
                        </UnorderedList>
                    </Box>
                )}
            {selectedFirmware.improvements &&
                selectedFirmware.improvements.length > 0 && (
                    <Box mb={2}>
                        <Heading size="sm">Improvements</Heading>
                        <UnorderedList>
                            {selectedFirmware.improvements.map(
                                (improvement, index) => (
                                    <ListItem key={index}>
                                        {improvement}
                                    </ListItem>
                                )
                            )}
                        </UnorderedList>
                    </Box>
                )}
        </Box>
    )
}

ReleaseNotesCard.propTypes = {
    selectedFirmware: PropTypes.shape({
        highlights: PropTypes.string,
        bugfixes: PropTypes.arrayOf(PropTypes.string),
        improvements: PropTypes.arrayOf(PropTypes.string),
    }),
}

export default function Upload() {
    const [firmwares, setFirmwares] = useState([])
    const [selectedFirmware, setSelectedFirmware] = useState(null)
    const [isConnected, setIsConnected] = useState(false)
    const [isUploading, setIsUploading] = useState(false)
    const [espLoader, setEspLoader] = useState(null)
    const toastIdRef = useRef()
    const toast = useToast()

    useEffect(() => {
        // Parse the release notes and set the firmwares state
        const parsedFirmwares = Object.entries(releaseNotes).map(
            ([version, data]) => ({
                version,
                fileName: data.fileName,
                releaseDate: data.releaseDate,
                highlights: data.releaseNotes.Highlights,
                bugfixes: data.releaseNotes.Bugfixes,
                improvements: data.releaseNotes.Improvements,
            })
        )
        setFirmwares(parsedFirmwares)
        // Set the latest firmware as the default selected firmware
        if (parsedFirmwares.length > 0) {
            setSelectedFirmware(parsedFirmwares[0])
        }
    }, [])

    const handleFirmwareChange = (event) => {
        const selected = firmwares.find(
            (fw) => fw.version === event.target.value
        )
        setSelectedFirmware(selected)
    }

    const validateFirmwareSelection = () => {
        if (!selectedFirmware) {
            toast({
                title: 'No firmware selected',
                description:
                    'Please select a firmware version before uploading.',
                status: 'warning',
                duration: 3000,
                isClosable: true,
            })
            return false
        }
        return true
    }

    const updateProgressToast = useCallback(
        (progress) => {
            if (toastIdRef.current) {
                toast.update(toastIdRef.current, {
                    description: (
                        <Box>
                            <Progress
                                value={progress}
                                size="sm"
                                colorScheme="blue"
                                mb={2}
                            />
                            Uploading: {progress}%
                        </Box>
                    ),
                    duration: null,
                })
            }
        },
        [toast]
    )

    const handleConnectUpload = async () => {
        if (!isConnected) {
            try {
                const port = await navigator.serial.requestPort()
                const esploader = new ESPLoader({
                    transport: 'web-serial',
                    baudrate: 921600,
                    port: port,
                })

                await esploader.main()
                await esploader.flashId()
                setEspLoader(esploader)
                setIsConnected(true)
                toast({
                    title: 'Connected successfully',
                    status: 'success',
                    duration: 3000,
                    isClosable: true,
                })
            } catch (error) {
                console.error('Connection failed:', error)
                toast({
                    title: 'Connection failed',
                    description: 'Please check your device and try again.',
                    status: 'error',
                    duration: 5000,
                    isClosable: true,
                })
            }
        } else if (selectedFirmware && espLoader) {
            if (!validateFirmwareSelection()) return

            setIsUploading(true)
            toastIdRef.current = toast({
                title: 'Uploading Firmware',
                description: (
                    <Box>
                        <Progress
                            value={0}
                            size="sm"
                            colorScheme="blue"
                            mb={2}
                        />
                        Uploading: 0%
                    </Box>
                ),
                status: 'info',
                duration: null,
                isClosable: false,
            })

            try {
                // Fetch the firmware file
                const firmwareUrl = new URL(
                    `../assets/firmwares/${selectedFirmware.fileName}`,
                    import.meta.url
                ).href
                console.log('Attempting to fetch firmware from:', firmwareUrl)
                const response = await fetch(firmwareUrl)

                if (!response.ok) {
                    throw new Error(`HTTP error! status: ${response.status}`)
                }

                const blob = await response.blob()
                const reader = new FileReader()
                const firmwareData = await new Promise((resolve, reject) => {
                    reader.onload = (e) => resolve(e.target.result)
                    reader.onerror = (e) => reject(e)
                    reader.readAsBinaryString(blob)
                })

                console.log(
                    'Firmware data fetched, size:',
                    firmwareData.length,
                    'bytes'
                )
                const fileArray = [
                    {
                        data: firmwareData,
                        address: 0x10000, // Adjust this address if needed
                    },
                ]

                const flashOptions = {
                    fileArray: fileArray,
                    flashSize: 'keep',
                    eraseAll: false,
                    compress: true,
                    reportProgress: (fileIndex, written, total) => {
                        const progress = Math.floor((written / total) * 100)
                        updateProgressToast(progress)
                    },
                }

                await espLoader.writeFlash(flashOptions)
                toast.close(toastIdRef.current)
                toast({
                    title: 'Firmware uploaded successfully',
                    status: 'success',
                    duration: 3000,
                    isClosable: true,
                })
            } catch (error) {
                console.error('Upload failed:', error)
                toast.close(toastIdRef.current)
                toast({
                    title: 'Upload failed',
                    description: 'Please try again.',
                    status: 'error',
                    duration: 5000,
                    isClosable: true,
                })
            } finally {
                setIsUploading(false)
            }
        } else {
            toast({
                title: 'No firmware selected',
                description:
                    'Please select a firmware version before uploading.',
                status: 'warning',
                duration: 3000,
                isClosable: true,
            })
        }
    }

    return (
        <Box display="flex" justifyContent="center">
            <Box p={4} w="50vw">
                <Alert status="info" variant="solid" rounded="lg">
                    <AlertIcon size="lg" />
                    <AlertDescription>
                        <Text>
                            - To start your device in Update mode, hold the
                            <strong> MODE button</strong> down while plugging it
                            in. All the LEDs will stay off during the procedure
                        </Text>
                        <Text>
                            - Once the device is in Update mode, you can click
                            <strong> &quot;Connect Device&quot;</strong>, a
                            popup will appear, select
                            <strong>
                                {' '}
                                &quot;USB JTAG/Serial debug unit&quot;{' '}
                            </strong>
                            from the list and click
                            <strong> &quot;Connect&quot;</strong>.
                        </Text>
                        <Text>
                            - You can now select a firmware version from the
                            dropdown and click on{' '}
                            <strong> &quot;Update Firmware&quot;</strong> to
                            start the process
                        </Text>
                        <Text>
                            - Once the firmware is uploaded,
                            <strong> unplug</strong> the device to exit Update
                            mode
                        </Text>
                        <Text>
                            - Enter the Web configurator and sync the device (connect, then save) to update
                        </Text>
                    </AlertDescription>
                </Alert>
                <VStack spacing={4} align="stretch">
                    <Box></Box>
                    <Button
                        onClick={handleConnectUpload}
                        colorScheme={isConnected ? 'green' : 'purple'}
                        isLoading={isUploading}
                        loadingText="Uploading..."
                    >
                        {isConnected ? 'Upload Firmware' : 'Connect Device'}
                    </Button>
                    <Select
                        placeholder="Select firmware version"
                        onChange={handleFirmwareChange}
                        value={selectedFirmware?.version || ''}
                    >
                        {firmwares.map((fw) => (
                            <option key={fw.version} value={fw.version}>
                                {fw.version} - {fw.releaseDate}
                            </option>
                        ))}
                    </Select>
                    <ReleaseNotesCard selectedFirmware={selectedFirmware} />
                </VStack>
            </Box>
        </Box>
    )
}

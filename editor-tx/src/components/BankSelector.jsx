import { useContext } from 'react'
import { Box, Button, Text } from '@chakra-ui/react'
import MidiContext from './MidiProvider'

export default function BankSelector() {
    const { selectedBank, setSelectedBank } = useContext(MidiContext)

    const handleButtonClick = (index) => {
        setSelectedBank(index)
    }

    const buttons = Array.from({ length: 4 }, (_, index) => index + 1)
    return (
        <Box display="flex" justifyContent="center" alignItems="center">
            <Text fontWeight="bold" mr={2}>
                Banks:
            </Text>
            {buttons.map((state, index) => (
                <Button
                    key={index}
                    bg={selectedBank === index ? 'primary.500' : 'gray.300'}
                    color="white"
                    borderRadius="full"
                    size="sm"
                    mx={1}
                    onClick={() => handleButtonClick(index)}
                >
                    {index + 1}
                </Button>
            ))}
        </Box>
    )
}

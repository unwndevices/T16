import {
    Card,
    CardHeader,
    CardBody,
    Flex,
    Text,
    Button,
    Spacer,
    Progress,
    Stack,
    StackDivider,
} from '@chakra-ui/react'
import { NumberInput } from '../components/CustomInputs'
import { PropTypes } from 'prop-types'
import { useContext, useMemo } from 'react'
import MidiContext from './MidiProvider'
import SkeletonLoader from './SkeletonLoader'

export function CcCard({ name, channel, cc, value, onChange, isDuplicate }) {
    const { ccMessages } = useContext(MidiContext)

    // Assuming onChange updates the state or props that control channel or cc,
    // ensure it's correctly implemented to reflect changes.
    const handleSelectChannel = (newChannel) => {
        // Update the channel state or props here
        onChange({ channel: newChannel, cc, value })
    }

    const handleSelectCc = (newCc) => {
        // Update the cc state or props here
        onChange({ channel, cc: newCc, value })
    }

    const updatedValue = useMemo(() => {
        const ccMessage = ccMessages.find((message) => message.index === cc)
        return ccMessage ? Math.round(ccMessage.value * 100) : value
    }, [ccMessages, cc, value])

    return (
        <Card
            as={Flex}
            justifyContent="space-between"
            borderRadius={20}
            borderColor="gray.100"
            borderBottomWidth={4}
            height="250px"
        >
            <CardHeader pt={4} pb={1}>
                <Flex
                    justifyContent="space-between"
                    alignItems="center"
                    alignContent="center"
                    wrap="wrap"
                >
                    <Button
                        colorScheme="primary"
                        flexGrow="0"
                        size="lg"
                        px={3}
                        borderRadius="full"
                    >
                        {name}
                    </Button>
                    <Spacer />
                    <Progress
                        flexGrow="5"
                        colorScheme="primary"
                        height={3}
                        value={updatedValue}
                    />
                </Flex>
            </CardHeader>
            <Stack divider={<StackDivider />} spacing="3" pb={2}>
                <CardBody py={3}>
                    <Flex justifyContent="space-between" align="center">
                        <Text mr="10px">CH</Text>
                        <SkeletonLoader>
                            <NumberInput
                                def={channel}
                                min={1}
                                max={16}
                                value={channel} // Set the value of the input component
                                onSelect={handleSelectChannel}
                            />
                        </SkeletonLoader>
                    </Flex>
                </CardBody>
                <CardBody py={3}>
                    <Flex justifyContent="space-between" align="center">
                        <Text mr="4px">CC</Text>
                        <SkeletonLoader>
                            <NumberInput
                                def={cc}
                                min={0}
                                max={127}
                                value={cc} // Set the value of the input component
                                onSelect={handleSelectCc}
                                isHighlighted={isDuplicate}
                            />
                        </SkeletonLoader>
                    </Flex>
                </CardBody>
            </Stack>
        </Card>
    )
}

CcCard.propTypes = {
    name: PropTypes.string,
    channel: PropTypes.number,
    cc: PropTypes.number,
    value: PropTypes.number,
    onChange: PropTypes.func,
    isDuplicate: PropTypes.bool,
}

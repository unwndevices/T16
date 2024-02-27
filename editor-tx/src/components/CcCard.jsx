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

export function CcCard({ name, channel, cc, value }) {
    const { ccMessages } = useContext(MidiContext)

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
                        flexGrow="0"
                        colorScheme="green"
                        size="lg"
                        px={3}
                        borderRadius="full"
                    >
                        {name}
                    </Button>
                    <Spacer />
                    <Progress
                        flexGrow="5"
                        colorScheme="green"
                        height={3}
                        value={updatedValue}
                    />
                </Flex>
            </CardHeader>
            <Stack divider={<StackDivider />} spacing="3" pb={2}>
                <CardBody py={3}>
                    <Flex justifyContent="space-between" align="center">
                        <Text mr="10px">CH</Text>
                        <NumberInput def={channel} min={1} max={16} />
                    </Flex>
                </CardBody>
                <CardBody py={3}>
                    <Flex justifyContent="space-between" align="center">
                        <Text mr="4px">CC</Text>
                        <NumberInput def={cc} min={0} max={127} />
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
}

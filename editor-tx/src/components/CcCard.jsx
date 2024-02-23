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

export function CcCard({ name, channel, cc, value }) {
    return (
        <Card borderRadius={20} borderColor="gray.100" borderBottomWidth={4}>
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
                        size="md"
                        borderRadius="full"
                    >
                        {name}
                    </Button>
                    <Spacer />
                    <Progress
                        flexGrow="5"
                        colorScheme="green"
                        height={3}
                        value={value}
                    />
                </Flex>
            </CardHeader>
            <Stack divider={<StackDivider />} spacing="0">
                <CardBody py={3}>
                    <Flex justifyContent="space-between" align="center">
                        <Text mr="10px">CH</Text>
                        <NumberInput
                            def_value={channel}
                            min_value={1}
                            max_value={16}
                        />
                    </Flex>
                </CardBody>
                <CardBody py={3}>
                    <Flex justifyContent="space-between" align="center">
                        <Text mr="10px">CC</Text>
                        <NumberInput
                            def_value={cc}
                            min_value={0}
                            max_value={127}
                        />
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

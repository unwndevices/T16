import {
    Card,
    CardHeader,
    CardBody,
    Flex,
    Button,
    Spacer,
    Stack,
    StackDivider,
    Slider,
    SliderTrack,
    SliderFilledTrack,
    SliderThumb,
    Text,
    SliderMark,
} from '@chakra-ui/react'

import { PropTypes } from 'prop-types'

export function SliderCard({
    name,
    value,
    min = 0,
    max = 127,
}) {

    return (
        <Card borderRadius={20} borderColor="gray.100" borderBottomWidth={4}>
            <Flex flexDirection="column" height="150px">
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
                        <Text>{value}</Text>
                    </Flex>
                </CardHeader>
                <Stack divider={<StackDivider />} spacing="0">
                    <CardBody py={3} pt={10}>
                        <Flex justifyContent="space-between" align="center">
                                <Slider
                                    aria-label={name}
                                    defaultValue={value}
                                    min={min}
                                    max={max}
                                    size="lg"
                                    colorScheme="green"
                                >
                                    <SliderTrack height={2} rounded={3}>
                                        <SliderFilledTrack />
                                    </SliderTrack>
                                    <SliderThumb
                                        boxSize={8}
                                        backgroundColor={'green.300'}
                                    />
                                </Slider>
                        </Flex>
                    </CardBody>
                </Stack>
            </Flex>
        </Card>
    )
}

SliderCard.propTypes = {
    name: PropTypes.string,
    value: PropTypes.number,
    max: PropTypes.number,
    min: PropTypes.number,
    stepped: PropTypes.bool,
    step: PropTypes.number,
}
